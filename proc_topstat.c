// for /proc stuff
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* cpu data *******************************************************************
- copied from fs/proc/stat.c
*/
#include <linux/kernel_stat.h>
#include <linux/tick.h>

// TODO ask upstream if `nsec_to_clock_t` can be EXPORT_SYMBOL
u64 nsec_to_clock_t(u64 x)
{
#if (NSEC_PER_SEC % USER_HZ) == 0
	return div_u64(x, NSEC_PER_SEC / USER_HZ);
#elif (USER_HZ % 512) == 0
	return div_u64(x * USER_HZ / 512, NSEC_PER_SEC / 512);
#else
	return div_u64(x * 9, (9ull * NSEC_PER_SEC + (USER_HZ / 2)) / USER_HZ);
#endif
}

#ifdef arch_idle_time

static u64 get_idle_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 idle;

	idle = kcs->cpustat[CPUTIME_IDLE];
	if (cpu_online(cpu) && !nr_iowait_cpu(cpu))
		idle += arch_idle_time(cpu);
	return idle;
}

static u64 get_iowait_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 iowait;

	iowait = kcs->cpustat[CPUTIME_IOWAIT];
	if (cpu_online(cpu) && nr_iowait_cpu(cpu))
		iowait += arch_idle_time(cpu);
	return iowait;
}

#else

static u64 get_idle_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 idle, idle_usecs = -1ULL;

	if (cpu_online(cpu))
		idle_usecs = get_cpu_idle_time_us(cpu, NULL);

	if (idle_usecs == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.idle */
		idle = kcs->cpustat[CPUTIME_IDLE];
	else
		idle = idle_usecs * NSEC_PER_USEC;

	return idle;
}

static u64 get_iowait_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 iowait, iowait_usecs = -1ULL;

	if (cpu_online(cpu))
		iowait_usecs = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_usecs == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
		iowait = kcs->cpustat[CPUTIME_IOWAIT];
	else
		iowait = iowait_usecs * NSEC_PER_USEC;

	return iowait;
}

#endif

static int me_show_cpu(struct seq_file *p, void *v)
{
	int i;
	u64 used;
	u64 total;

	for_each_online_cpu(i) {
		struct kernel_cpustat *kcs = &kcpustat_cpu(i);
		total = 0;
		used = 0;

		total += kcs->cpustat[CPUTIME_USER];
		total += kcs->cpustat[CPUTIME_NICE];
		total += kcs->cpustat[CPUTIME_SYSTEM];

		used = total;

		total += get_idle_time(kcs, i);
		total += get_iowait_time(kcs, i);
		total += kcs->cpustat[CPUTIME_IRQ];
		total += kcs->cpustat[CPUTIME_SOFTIRQ];
		total += kcs->cpustat[CPUTIME_STEAL];
		total += kcs->cpustat[CPUTIME_GUEST];
		total += kcs->cpustat[CPUTIME_GUEST_NICE];

		seq_printf(p, "cpu");
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(total));
		seq_put_decimal_ull(p, " ", nsec_to_clock_t(used));
		seq_putc(p, '\n');
	}

	return 0;
}

/* mem data *******************************************************************
- copied from fs/proc/meminfo.c
*/
#include <linux/mm.h>

// TODO-swap ask upstream if `si_swapinfo` can be EXPORT_SYMBOL

static void show_val_kb(struct seq_file *m, unsigned long num)
{
	seq_put_decimal_ull(m, " ", num << (PAGE_SHIFT - 10));
}

static int me_show_mem(struct seq_file *p, void *v)
{
	struct sysinfo i;
	long cached;

	si_meminfo(&i);
	// TODO-swap si_swapinfo(&i);

	// TODO: cached is still not 0 after `sysctl vm.drop_caches=3`
	cached = global_node_page_state(NR_FILE_PAGES) - i.bufferram;
	if (cached < 0)
		cached = 0;

	seq_printf(p, "mem");
	show_val_kb(p, i.totalram);
	show_val_kb(p, i.totalram - i.freeram);
	show_val_kb(p, cached);
	seq_putc(p, '\n');

	/* TODO-swap
	seq_printf(p, "swap");
	show_val_kb(p, i.totalswap);
	show_val_kb(p, i.totalswap - i.freeswap);
	seq_putc(p, '\n');
	*/

	return 0;
}

/* /proc stuff ***************************************************************/

static int me_show(struct seq_file *p, void *v)
{
	me_show_cpu(p, v);
	me_show_mem(p, v);
	return 0;
}

static int me_open(struct inode *inodep, struct file *filep)
{
	return single_open(filep, me_show, NULL);
}

static const struct proc_ops fops = {
	.proc_open    = me_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* kernel module *************************************************************/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int __init me_init(void)
{
	printk(KERN_INFO "proc_topstat: init.\n");
	proc_create("topstat", 0, NULL, &fops);
	return 0;
}

static void __exit me_exit(void)
{
	printk(KERN_INFO "proc_topstat: exit.\n");
	remove_proc_entry("topstat", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andre Schmidt");
MODULE_DESCRIPTION("provide simple system stats in /proc/topstat");
MODULE_VERSION("0.0.1");

module_init(me_init);
module_exit(me_exit);
