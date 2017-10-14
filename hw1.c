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
#include <linux/list.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define EXITING_HIGHER_BOUND 100

#define BITS 3
#define BY_S 1000000000
#define BY_MS 1000000
#define BY_M 1000

#define MESSAGE_LENGTH 10

static void print_bar(struct seq_file *s){
	int i;
	for(i = 0;i < 160;i++)
		seq_printf(s, "-");
	seq_printf(s, "\n");
}

static int write_process_info(struct seq_file *s)
{
	int task_count=0, running_count=0, sleeping_count=0, stopped_count=0, zombie_count=0;
	struct task_struct *task;

	bool is_kernel;
	
	char* command;
	int pid, ppid;
	int msec_user_time, msec_kernel_time, msec_total_time;
	int start_time, total_time, user_time, kernel_time;
	int start_time_ms, total_time_ms, user_time_ms, kernel_time_ms;
	long unsigned voluntary, involuntary;
	char process_state;
	char* scheduler_type;
	long long unsigned vruntime;

	struct timespec64 boottime;

	// systemwide info
	for_each_process(task)
	{
		task_count++;
		if(task->state == TASK_RUNNING) {
			// running
			running_count++;	
		} else if (task->state == TASK_UNINTERRUPTIBLE || task->state == TASK_INTERRUPTIBLE) {
			// sleeping
			sleeping_count++;
		} else if (task->state == __TASK_STOPPED) {
			// stopped
			stopped_count++;
		} else if (task->exit_state == EXIT_ZOMBIE) {
			// zombie
			zombie_count++;
		} else {
			// undefined
		}
	}


	print_bar(s);
	seq_printf(s, "%76s %-78s\n", "2014117007", "Ji-Wan Chung");
	print_bar(s);
	seq_printf(s, "CURRENT SYSTEM INFORMATION >\n");
	seq_printf(s, "Total %d task, %d running, %d sleeping, %d stopped, %d zombies\n", 
	task_count, running_count, sleeping_count, stopped_count, zombie_count);
	get_monotonic_boottime64(&boottime);
	seq_printf(s, "%dHz, %llu ms after system boot time\n", HZ, 
		(timespec_to_ns(&boottime))/BY_MS);

	print_bar(s);
	
	//write column names	
	seq_printf(s, "%19s%8s%8s%13s%13s%13s%13s%14s%16s%8s", 
	"command","pid","ppid","start(s)","total(s)",
	"user(s)","kernel(s)","voluntary","involuntary", "state");
	seq_printf(s, "%12s", "scheduler");
	seq_printf(s, "%15s", "vruntime");
	seq_printf(s, "\n");
	print_bar(s);
	
	// write data
	// user process
	for_each_process(task)
	{
		is_kernel = ( task->mm == NULL ? true : false );
		
		if ( task->mm != NULL )
		{
			command = task->comm;
			pid = task->pid;
			ppid = task->real_parent->pid;

			start_time = task->real_start_time/BY_S;
			start_time_ms = -start_time*1000 + task->real_start_time/BY_MS; 		

			// jiffy -> nsec 
			msec_user_time = jiffies_to_msecs(task->utime);
			user_time = msec_user_time/1000;
			user_time_ms = -user_time*1000 + msec_user_time;
			msec_kernel_time = jiffies_to_msecs(task->stime);
			kernel_time = msec_kernel_time/1000;
			kernel_time_ms = -kernel_time*1000 + msec_kernel_time;
			msec_total_time = msec_user_time + msec_kernel_time;	
			total_time = msec_total_time/1000; 
			total_time_ms =	-total_time*1000 + msec_total_time;
			voluntary = task->nvcsw;
			involuntary = task->nivcsw;

			process_state = 'U';	
 			if(task->state == TASK_RUNNING) {
				// running
				process_state = 'R';
			} else if (task->state == TASK_UNINTERRUPTIBLE || task->state == TASK_INTERRUPTIBLE) {
				// sleeping
				process_state = 'S';
			} else if (task->state == __TASK_STOPPED) {
				// stopped
				process_state = 'T';
			} else if (task->exit_state == EXIT_ZOMBIE) {
				// zombie
				process_state = 'Z';
			} else {
				// undefined
			}

			scheduler_type = "none";
			switch(task->policy) {
				case 0: scheduler_type = "CFS";break;		
				case 1: scheduler_type = "REALTIME";break;
				case 2: scheduler_type = "REALTIME";break;	
				case 3: scheduler_type = "REALTIME";break;	
				case 5: scheduler_type = "IDLE";break;	
				case 6: scheduler_type = "DEADLINE";break;	
			}
			vruntime = task->se.vruntime;		

			seq_printf(s, "%19s%8d%8d%9d.%03d%9d.%03d%9d.%03d%9d.%03d%14lu%16lu%8c", 
				command, pid, ppid,
				start_time,start_time_ms,
				total_time,total_time_ms,
				user_time,user_time_ms,
				kernel_time,kernel_time_ms,
				voluntary,involuntary,
				process_state);
			seq_printf(s, "%12s", scheduler_type);
			if (task->state == 0)
				seq_printf(s, "%15llu", vruntime);
			seq_printf(s, "\n");		
		}

	}

	print_bar(s);	
	
	for_each_process(task)
	{
		is_kernel = ( task->mm == NULL ? true : false );
		
		if ( task->mm == NULL )
		{
			command = task->comm;
			pid = task->pid;
			ppid = task->real_parent->pid;

			start_time = task->real_start_time/BY_S;
			start_time_ms = -start_time*1000 + task->real_start_time/BY_MS; 		

			// jiffy -> nsec 
			msec_user_time = jiffies_to_msecs(task->utime);
			user_time = msec_user_time/1000;
			user_time_ms = -user_time*1000 + msec_user_time;
			msec_kernel_time = jiffies_to_msecs(task->stime);
			kernel_time = msec_kernel_time/1000;
			kernel_time_ms = -kernel_time*1000 + msec_kernel_time;
			msec_total_time = msec_user_time + msec_kernel_time;	
			total_time = msec_total_time/1000; 
			total_time_ms =	-total_time*1000 + msec_total_time;
			voluntary = task->nvcsw;
			involuntary = task->nivcsw;

			process_state = 'U';	
 			if(task->state == TASK_RUNNING) {
				// running
				process_state = 'R';
			} else if (task->state == TASK_UNINTERRUPTIBLE || task->state == TASK_INTERRUPTIBLE) {
				// sleeping
				process_state = 'S';
			} else if (task->state == __TASK_STOPPED) {
				// stopped
				process_state = 'T';
			} else if (task->exit_state == EXIT_ZOMBIE) {
				// zombie
				process_state = 'Z';
			} else {
				// undefined
			}

			scheduler_type = "none";
			switch(task->policy) {
				case 0: scheduler_type = "CFS";break;		
				case 1: scheduler_type = "REALTIME";break;
				case 2: scheduler_type = "REALTIME";break;	
				case 3: scheduler_type = "REALTIME";break;	
				case 5: scheduler_type = "IDLE";break;	
				case 6: scheduler_type = "DEADLINE";break;	
			}
			vruntime = task->se.vruntime;		

			seq_printf(s, "%19s%8d%8d%9d.%03d%9d.%03d%9d.%03d%9d.%03d%14lu%16lu%8c", 
				command, pid, ppid,
				start_time,start_time_ms,
				total_time,total_time_ms,
				user_time,user_time_ms,
				kernel_time,kernel_time_ms,
				voluntary,involuntary,
				process_state);
			seq_printf(s, "%12s", scheduler_type);
			if (task->state == 0)
				seq_printf(s, "%15llu", vruntime);
			seq_printf(s, "\n");		
		}

	}

	print_bar(s);
	return 0;
}

typedef struct proc_exit_item
{
	char comm[TASK_COMM_LEN];
	pid_t pid;
	pid_t ppid;
	unsigned int cpu;
	u64 start_time;
	u64 dead_time;
	struct list_head exit_head;
} exit_it_t;	

static LIST_HEAD(exit_list);

char emit_flag;
bool probe_flag;

static int flush_list(void)
{
	exit_it_t *tmp, *n;

	//flush list
	list_for_each_entry_safe(tmp, n, &exit_list, exit_head)
	{
		list_del(&tmp->exit_head);
		kfree(tmp);
	}

	return list_empty(&exit_list);
}

void jdo_task_dead(void)
{
	struct task_struct *tsk = current;
	struct timespec64 ts;
	
	exit_it_t *it1;

	if (probe_flag)
	{
		// allocate mem for list node
		it1 = kmalloc(sizeof *it1, GFP_KERNEL);
		if (!it1) {
			printk("Can't allocate mem");
			return;
		}
		//save info in data structure
		it1->pid = tsk->pid;
		it1->ppid = tsk->real_parent->pid;
		it1->cpu = tsk->cpu;
		it1->start_time = tsk->real_start_time;	
		get_monotonic_boottime64(&ts);
		it1->dead_time = timespec_to_ns(&ts);
		// copy command name
		memcpy(it1->comm, tsk->comm, TASK_COMM_LEN);
		// init list head
		INIT_LIST_HEAD(&it1->exit_head);

		//add in the list
		list_add_tail(&it1->exit_head, &exit_list);		
	}

	/* Always end with a call to jprobe_return(). */
	jprobe_return();
	/*NOTREACHED*/
}

static int write_dead_info(struct seq_file *s)
{
	// tmp list node pointer
	exit_it_t *tmp;
	// time vars
	unsigned int start_s, start_ms, dead_s, dead_ms;

	// print column names
	print_bar(s);
	seq_printf(s, "%19s%8s%8s%8s%19s%19s\n", 
	"command","pid","ppid","cpu","start(s)", "exit(s)");
	print_bar(s);
	
	// iterate over hash table
	list_for_each_entry(tmp, &exit_list, exit_head)	
	{ 
		// time vars
		start_s = tmp->start_time/BY_S;
		start_ms = tmp->start_time/BY_MS - start_s*BY_M;
		dead_s = tmp->dead_time/BY_S;
		dead_ms = tmp->dead_time/BY_MS - dead_s*BY_M;			

		// print
		seq_printf(s, "%19s%8d%8d", tmp->comm, tmp->pid, tmp->ppid);
		seq_printf(s, "%8d", tmp->cpu);
		seq_printf(s, "%15d.%03d", start_s, start_ms);
		seq_printf(s, "%15d.%03d", dead_s, dead_ms);
		seq_printf(s, "\n");
	}
	
	flush_list();	
	return 0;
}

static struct jprobe my_jdead = {
        .entry = JPROBE_ENTRY(jdo_task_dead)
};

static int write_mode_a(struct seq_file *s)
{
	return write_process_info(s);
};

static int write_err_info(struct seq_file *s, char flag)
{
	seq_printf(s, "Err: Incorrect Input: %c\n", flag);
	return 0;
}
 
static int h1_show(struct seq_file *m, void *v)
{
	if ( emit_flag == 'T')
		return write_mode_a(m);
	else if ( emit_flag == 'E')
		return (write_dead_info(m));
	write_err_info(m, emit_flag);
	printk(KERN_ALERT "Incorrect input: %c\n", emit_flag);
	return 0;
}

static int hw1_open(struct inode *inode, struct file *file)
{
	return single_open(file, h1_show, NULL);
}

static int start_jprobing(void)
{
	int ret;

	// regitster jprobe for do_task_dead
	my_jdead.kp.symbol_name = "do_task_dead";
	
	if((ret = register_jprobe(&my_jdead)) <0) {
		printk("register_jdead failed, returned %d\n", ret);
                return -1;
	}
	printk("Planted jdead at %p, handler addr %p\n",
               my_jdead.kp.addr, my_jdead.entry);

	return 0;	
}

static int stop_jprobing(void)
{
	probe_flag = false;
		
	return 0;
}

static int init_mod_B(void)
{
	probe_flag = true;
	
	return 0;
}

static int stop_mod_B(void)
{
	flush_list();
	return stop_jprobing();
}

static ssize_t hw1_write(struct file *file, const char __user *buf,
	size_t length, loff_t *offset)
{
	char Message[MESSAGE_LENGTH];
	int i;

	for(i=0; i<MESSAGE_LENGTH-1 && i<length; i++)
		get_user(Message[i], buf+i);
	Message[i] = '\0';

	if( Message[0] == '\0')
		printk(KERN_ALERT "ERR: no message");
	else
	{
		emit_flag = Message[0];
		if( emit_flag == 'E')
			init_mod_B();
		else
			stop_mod_B();
	}
		
	return i;	
}

static const struct file_operations
hw1_fops = {
	.owner = THIS_MODULE,
	.open = hw1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = hw1_write,
};

static int __init hw1_init(void)
{
	
	// init with mode B	
	emit_flag = 'E';
	init_mod_B();

	// start probing
	start_jprobing();

	//create proc file
	proc_create("hw1", 0, NULL, &hw1_fops);
	return 0;
}

static void __exit hw1_exit(void)
{
	if ( emit_flag == 'E') {
		stop_mod_B();
	}
	// unregister jprobe
	unregister_jprobe(&my_jdead);
	printk("unregistered jdead");

	//remove proc file
	remove_proc_entry("hw1", NULL);
}

MODULE_AUTHOR("Jiwan Chung");
MODULE_LICENSE("GPL");

module_init(hw1_init);
module_exit(hw1_exit);
