cc = gcc -g
prom = containerc
obj = main.o netlink.o veth.o utils.o

$(prom): $(obj)
	@set -e
	@if [ -f ./containerc_roots.tar.gz ]; then \
		tar -zxf ./containerc_roots.tar.gz; \
	else \
		echo "ROOTFS Is Not Exist"; \
		exit 1; \
	fi
	$(cc) -o $(prom) $(obj)

utils.o: lib/utils.c
	$(cc) -c $< -o $@

veth.o: lib/veth.c
	$(cc) -c $< -o $@

netlink.o: lib/netlink.c lib/netlink.h
	$(cc) -c $< -o $@

main.o: main.c lib/netlink.h
	$(cc) -c $< -o $@

clean:
	rm -rf containerc main.o netlink.o veth.o utils.o containerc_roots
