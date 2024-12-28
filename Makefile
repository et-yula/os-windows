obj-m += pci_info.o

all:
	make -C /lib/modules/`uname -r`/build M=`pwd` modules
	g++ -o pci pci_info_user.c

clean:
	make -C /lib/modules/`uname`/build M=`pwd` clean
	rm pci

