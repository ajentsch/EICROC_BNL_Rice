obj-m  := resmem.o

### For diskless nodes 
#KERNELDIR := /RTS/JEFF/kerneldev/linux-2.6.32.63-aufs

### For local
KERNELDIR := /lib/modules/$(shell uname -r)/build

all::
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules
clean:
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean
    
save:
	@/RTS/bin/install.sh resmem.ko /RTS/bin/TPX64

insmod:
	insmod /RTS/bin/TPX64/resmem.ko resmem_hwaddr=0x80000000 resmem_length=0x20000000
	chmod 666 /dev/resmem

insmod_here_512:
	insmod resmem.ko resmem_hwaddr=0x80000000 resmem_length=0x20000000
	chmod 666 /dev/resmem

insmod_here_128:
	insmod resmem.ko resmem_hwaddr=0x80000000 resmem_length=0x08000000
	chmod 666 /dev/resmem
