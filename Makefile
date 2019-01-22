cc = gcc
prom = containerc
obj = main.o netlink.o

$(prom): $(obj)
	$(cc) -o $(prom) $(obj)

netlink.o: lib/netlink.c lib/netlink.h
	$(cc) -c $< -o $@

main.o: main.c lib/netlink.h
	$(cc) -c $< -o $@

clean:
	rm -rf containerc main.o netlink.o
