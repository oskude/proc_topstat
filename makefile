KBDIR ?= /lib/modules/`uname -r`/build

default:
	make -C $(KBDIR) M=$(PWD) modules

clean:
	make -C $(KBDIR) M=$(PWD) clean
	rm -rf distro/archlinux/proc_topstat || true
	rm distro/archlinux/*.pkg.*

test:
	insmod proc_topstat.ko
	cat /proc/topstat
	rmmod proc_topstat.ko

archlinux:
	cd distro/archlinux;\
		makepkg -cf
