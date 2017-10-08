#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <asm/param.h>
#include <linux/time.h>
#include <linux/timekeeping.h>

static int write_summary(struct seq_file *m)
{
	seq_printf(m, "name of init: %s, HZ: %d, cu: %llu\n",
		init_task.comm, HZ, 
		(CURRENT_TIME.tv_nsec-init_task.real_start_time/1000000));	
	return 0;
}

static int write_process_info(struct seq_file *m)
{
	struct task_struct *task;
	
	for_each_process(task)
	{
		seq_printf(m, 
		"Name: %s PID: [%d]", task->comm, task->pid
		);
		seq_printf(m,
		"S: %llu, MS: %llu\n", task->real_start_time /1000000000,
		task->real_start_time/1000000);
	}
	seq_printf(m, "time: %llu\n", local_clock()/1000000);
	return 0;
}
static int h1_show(struct seq_file *m, void *v)
{
	return (write_summary(m) | write_process_info(m));
}

static int hw1_open(struct inode *inode, struct file *file)
{
	return single_open(file, h1_show, NULL);
}

static const struct file_operations
hw1_fops = {
	.owner = THIS_MODULE,
	.open = hw1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init hw1_init(void)
{
	proc_create("hw1", 0, NULL, &hw1_fops);
	return 0;
}

static void __exit hw1_exit(void)
{
	remove_proc_entry("hw1", NULL);
}

MODULE_LICENSE("GPL");
module_init(hw1_init);
module_exit(hw1_exit);
