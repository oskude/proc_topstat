obj-m += proc_topstat.o
KBDIR ?= /lib/modules/`uname -r`/build

default:
	make -C $(KBDIR) M=$(PWD) modules

clean:
	make -C $(KBDIR) M=$(PWD) clean

test:
	insmod proc_topstat.ko
	cat /proc/topstat
	rmmod proc_topstat.ko
