#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
#include <linux/jiffies.h>
#include <linux/types.h>
#include <linux/kprobes.h>
#include <linux/hashtable.h>
#include <linux/slab.h>

#define EXITING_HIGHER_BOUND 100
#define BITS 3

typedef struct proc_exit_item
{
	char comm[TASK_COMM_LEN];
	pid_t pid;
	pid_t ppid;
	unsigned int cpu;
	struct timespec64 exit_time;
	struct timespec64 dead_time;
	struct hlist_node exit_hash_list;
} exit_it_t;	

DEFINE_HASHTABLE(exit_hash, BITS);

void jdo_task_dead(void)
{
	struct task_struct *tsk = current;
	
	exit_it_t *tmp;

        printk("jdead pid:%d\n", tsk->pid);
	
	// update dead_time
	hash_for_each_possible(exit_hash, tmp, exit_hash_list, tsk->pid)
	{
		tmp->dead_time = current_kernel_timespec64();	
	}
	/* Always end with a call to jprobe_return(). */
	jprobe_return();
	/*NOTREACHED*/
}

void jdo_exit(long code)
{
	struct task_struct *tsk = current;

	exit_it_t *it1;

	printk("jexit pid:%d\n", tsk->pid);

	it1 = kmalloc(sizeof *it1, GFP_KERNEL);
	if (!it1) {
		printk("Can't allocate mem");
		return;
	}
	//save info in data structure
	it1->pid = tsk->pid;
	it1->ppid = tsk->real_parent->pid;
	it1->cpu = tsk->cpu;
	it1->exit_time = current_kernel_time64();
	// copy command name
	memcpy(it1->comm, tsk->comm, TASK_COMM_LEN);

	hash_add(exit_hash, &it1->exit_hash_list, it1->pid);
	
	/* Always end with a call to jprobe_return(). */
        jprobe_return();
        /*NOTREACHED*/	
}

static int write_dead_info(struct seq_file *s)
{
	return 0;
}

static struct jprobe my_jexit = {
        .entry = JPROBE_ENTRY(jdo_exit)
};

static struct jprobe my_jdead = {
        .entry = JPROBE_ENTRY(jdo_task_dead)
};

static int h1_show(struct seq_file *m, void *v)
{
	return (write_dead_info(m));
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
	//register jprobe for do_exit
	int ret;
	
	my_jexit.kp.symbol_name = "do_exit";
	
	if((ret = register_jprobe(&my_jexit)) <0) {
		printk("register_jexit failed, returned %d\n", ret);
                return -1;
	}
	printk("Planted jprobe at %p, handler addr %p\n",
               my_jexit.kp.addr, my_jexit.entry);

	my_jdead.kp.symbol_name = "do_task_dead";
	
	if((ret = register_jprobe(&my_jdead)) <0) {
		printk("register_jdead failed, returned %d\n", ret);
                return -1;
	}
	printk("Planted jdead at %p, handler addr %p\n",
               my_jdead.kp.addr, my_jdead.entry);

	//init hash table
	hash_init(exit_hash, BITS);
	
	//create proc file
	proc_create("hw1", 0, NULL, &hw1_fops);
	return 0;
}

static void __exit hw1_exit(void)
{
	//unregister jprobe
	unregister_jprobe(&my_jexit);
        printk("jexit unregistered\n");
	//unregister jprobe
	unregister_jprobe(&my_jdead);
        printk("jdead unregistered\n");


	//remove proc file
	remove_proc_entry("hw1", NULL);
}

MODULE_AUTHOR("Jiwan Chung");
MODULE_LICENSE("GPL");

module_init(hw1_init);
module_exit(hw1_exit);
