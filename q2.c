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
	struct timespec64 currenttime;

	getnstimeofday64(&currenttime);
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
	getboottime64(&boottime);
	seq_printf(s, "%dHz, %llu ms after system boot time\n", HZ, 
		(timespec_to_ns(&currenttime) - timespec_to_ns(&boottime))/BY_MS);

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
	struct timespec64 exit_time;
	struct timespec64 dead_time;
	bool if_exited;
	struct hlist_node exit_hash_list;
} exit_it_t;	

DEFINE_HASHTABLE(exit_hash, BITS);

char emit_flag;

void jdo_task_dead(void)
{
	struct task_struct *tsk = current;
	
	exit_it_t *tmp;

	// update dead_time
	hash_for_each_possible(exit_hash, tmp, exit_hash_list, tsk->pid)
	{
		if(!tmp->if_exited)
		{
			getnstimeofday64(&(tmp->dead_time));
			tmp->if_exited = true;
		}
	}
	/* Always end with a call to jprobe_return(). */
	jprobe_return();
	/*NOTREACHED*/
}

void jdo_exit(long code)
{
	struct task_struct *tsk = current;

	exit_it_t *it1;

	// allocate mem for hash node
	it1 = kmalloc(sizeof *it1, GFP_KERNEL);
	if (!it1) {
		printk("Can't allocate mem");
		return;
	}
	//save info in data structure
	it1->pid = tsk->pid;
	it1->ppid = tsk->real_parent->pid;
	it1->cpu = tsk->cpu;
	getnstimeofday64(&(it1->exit_time));
	// copy command name
	memcpy(it1->comm, tsk->comm, TASK_COMM_LEN);
	
	it1->if_exited = false;
	// add node
	hash_add(exit_hash, &it1->exit_hash_list, it1->pid);
	
	/* Always end with a call to jprobe_return(). */
        jprobe_return();
        /*NOTREACHED*/	
}

static int write_dead_info(struct seq_file *s)
{
	// bucket integer for hash iteration
	int bkt = 0;
	// tmp hash node pointer
	exit_it_t *tmp;
	// time vars
	unsigned int nsec_exit, nsec_dead, exit_start_s, exit_start_ms, dead_start_s, dead_start_ms;

	// print column names
	print_bar(s);
	seq_printf(s, "%19s%8s%8s%8s%19s%19s\n", 
	"command","pid","ppid","cpu","start(s)", "exit(s)");
	print_bar(s);
	
	// iterate over hash table
	hash_for_each(exit_hash, bkt, tmp, exit_hash_list)
	{ 
		// time vars
		nsec_exit = timespec_to_ns(&(tmp->exit_time));
		exit_start_s = nsec_exit/BY_S;
		exit_start_ms = nsec_exit/BY_MS - exit_start_s*BY_M;
		nsec_dead = timespec_to_ns(&(tmp->dead_time));
		dead_start_s = nsec_dead/BY_S;
		dead_start_ms = nsec_dead/BY_MS - dead_start_s*BY_M;			

		// print
		seq_printf(s, "%19s%8d%8d", tmp->comm, tmp->pid, tmp->ppid);
		seq_printf(s, "%8d", tmp->cpu);
		seq_printf(s, "%15d.%03d", exit_start_s, exit_start_ms);
		seq_printf(s, "%15d.%03d", dead_start_s, dead_start_ms);
		seq_printf(s, "\n");
	}
	return 0;
}

static struct jprobe my_jexit = {
        .entry = JPROBE_ENTRY(jdo_exit)
};

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
	//register jprobe for do_exit
	int ret;

	my_jexit.kp.symbol_name = "do_exit";
	
	if((ret = register_jprobe(&my_jexit)) <0) {
		printk("register_jexit failed, returned %d\n", ret);
                return -1;
	}
	printk("Planted jexit at %p, handler addr %p\n",
               my_jexit.kp.addr, my_jexit.entry);

	// regitster jprobe for do_task_dead
	my_jdead.kp.symbol_name = "do_task_dead";
	
	if((ret = register_jprobe(&my_jdead)) <0) {
		printk("register_jdead failed, returned %d\n", ret);
                return -1;
	}
	printk("Planted jdead at %p, handler addr %p\n",
               my_jdead.kp.addr, my_jdead.entry);

	hash_init(exit_hash);	
	return 0;	
}

static int stop_jprobing(void)
{
	//unregister jprobe
	unregister_jprobe(&my_jexit);
        printk("jexit unregistered\n");
	//unregister jprobe
	unregister_jprobe(&my_jdead);
        printk("jdead unregistered\n");
	
	printk(KERN_ALERT "EMTPY? :%d\n", hash_empty(exit_hash));	
	return 0;
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
			start_jprobing();
		else
			stop_jprobing();
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
	start_jprobing();

	//init hash table
	hash_init(exit_hash);
	
	//create proc file
	proc_create("hw1", 0, NULL, &hw1_fops);
	return 0;
}

static void __exit hw1_exit(void)
{
	if ( emit_flag == 'E') {
		stop_jprobing();
	}
	//remove proc file
	remove_proc_entry("hw1", NULL);
}

MODULE_AUTHOR("Jiwan Chung");
MODULE_LICENSE("GPL");

module_init(hw1_init);
module_exit(hw1_exit);
