#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
#include <linux/jiffies.h>
#include <linux/types.h>

#define BY_S 1000000000
#define BY_MS 1000000

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
	struct timespec64 currenttime = current_kernel_time64();

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
static int h1_show(struct seq_file *m, void *v)
{
	return (write_process_info(m));
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
