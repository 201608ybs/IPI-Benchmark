
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ktime.h>

#define NTIMES 100000

#define POKE_ANY	0
#define DRY_RUN		1
#define POKE_SELF	2
#define POKE_ALL	3
#define POKE_ALL_LOCK	4

static void __init handle_ipi_spinlock(void *t)
{
	spinlock_t *lock = (spinlock_t *) t;

	spin_lock(lock);
	spin_unlock(lock);
}

static void __init handle_ipi(void *t)
{
	ktime_t *time = (ktime_t *) t;

	if (time)
		*time = ktime_get() - *time;
}

static ktime_t __init send_ipi(int flags)
{
	ktime_t time = 0;
	DEFINE_SPINLOCK(lock);
	unsigned int cpu = get_cpu();

	switch (flags) {
	case DRY_RUN:
		/* Do everything except actually sending IPI. */
		break;
	case POKE_ALL:
		/* If broadcasting, don't force all CPUs to update time. */
		smp_call_function_many(cpu_online_mask, handle_ipi, NULL, 1);
		break;
	case POKE_ALL_LOCK:
		smp_call_function_many(cpu_online_mask,
				handle_ipi_spinlock, &lock, 1);
		break;
	case POKE_ANY:
		cpu = cpumask_any_but(cpu_online_mask, cpu);
		if (cpu >= nr_cpu_ids) {
			time = -ENOENT;
			break;
		}
		/* Fall thru */
	case POKE_SELF:
		time = ktime_get();
		smp_call_function_single(cpu, handle_ipi, &time, 1);
		break;
	default:
		time = -EINVAL;
	}

	put_cpu();
	return time;
}

static int __init __bench_ipi(unsigned long i, ktime_t *time, int flags)
{
	ktime_t t;

	*time = 0;
	while (i--) {
		t = send_ipi(flags);
		if ((int) t < 0)
			return (int) t;

		*time += t;
	}

	return 0;
}

static int __init bench_ipi(unsigned long times, int flags,
				ktime_t *ipi, ktime_t *total)
{
	int ret;

	*total = ktime_get();
	ret = __bench_ipi(times, ipi, flags);
	if (unlikely(ret))
		return ret;

	*total = ktime_get() - *total;

	return 0;
}

static int __init init_bench_ipi(void)
{
	ktime_t ipi, total;
	int ret;

	ret = bench_ipi(NTIMES, DRY_RUN, &ipi, &total);
	if (ret)
		pr_info("IPI_BENCHMARK Dry-run FAILED: %d\n", ret);
	else
		pr_info("IPI_BENCHMARK Dry-run:        %18llu, %18llu ns\n", ipi, total);

	ret = bench_ipi(NTIMES, POKE_SELF, &ipi, &total);
	if (ret)
		pr_info("IPI_BENCHMARK Self-IPI FAILED: %d\n", ret);
	else
		pr_info("IPI_BENCHMARK Self-IPI:       %18llu, %18llu ns\n", ipi, total);

	ret = bench_ipi(NTIMES, POKE_ANY, &ipi, &total);
	if (ret)
		pr_info("IPI_BENCHMARK Mutual IPI FAILED: %d\n", ret);
	else
		pr_info("IPI_BENCHMARK Mutual IPI:     %18llu, %18llu ns\n", ipi, total);

	// ret = bench_ipi(NTIMES, POKE_ALL, &ipi, &total);
	// if (ret)
	// 	pr_info("IPI_BENCHMARK Broadcast IPI FAILED: %d\n", ret);
	// else
	// 	pr_info("IPI_BENCHMARK Broadcast IPI:  %18llu, %18llu ns\n", ipi, total);

	// ret = bench_ipi(NTIMES, POKE_ALL_LOCK, &ipi, &total);
	// if (ret)
	// 	pr_info("IPI_BENCHMARK Broadcast lock FAILED: %d\n", ret);
	// else
	// 	pr_info("IPI_BENCHMARK Broadcast lock: %18llu, %18llu ns\n", ipi, total);

	/* Return error to avoid annoying rmmod. */
	return -EINVAL;
}

static void __exit ipi_bench_exit(void){ }
module_init(init_bench_ipi);
module_exit(ipi_bench_exit);

MODULE_LICENSE("GPL");