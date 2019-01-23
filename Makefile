cc = gcc -g
prom = containerc
obj = main.o netlink.o veth.o

$(prom): $(obj)
	$(cc) -o $(prom) $(obj)

veth.o: lib/veth.c
	$(cc) -c $< -o $@

netlink.o: lib/netlink.c lib/netlink.h
	$(cc) -c $< -o $@

main.o: main.c lib/netlink.h
	$(cc) -c $< -o $@

clean:
	rm -rf containerc main.o netlink.o veth.o