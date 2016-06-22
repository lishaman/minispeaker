#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/thread_info.h>
#include <asm/uaccess.h>

#define BUF_LEN         64

static int magic_read_proc(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
	struct thread_struct *mc_thread = &current->thread;

	/* sprintf auto add '\0' */
	if(mc_thread->mcflags == CPU_ARM)
		sprintf(page, "arm     \n");
	else if(mc_thread->mcflags == CPU_ARM_NEON)
		sprintf(page, "arm_neon\n");
	else 
		sprintf(page, "mips    \n");
		
	return 10;
}

static int magic_write_proc(struct file *file, const char __user *buffer,
                           unsigned long count, void *data)
{
        char buf[BUF_LEN];
	struct thread_struct *mc_thread = &current->thread;

        if (count > BUF_LEN)
                count = BUF_LEN;
        if (copy_from_user(buf, buffer, count))
                return -EFAULT;

	if (strncmp(buf, "arm_neon",8) == 0) {		
		mc_thread->mcflags = CPU_ARM_NEON;
        }
        else if (strncmp(buf, "arm",3) == 0) {
		mc_thread->mcflags = CPU_ARM;
        }
	else if(strncmp(buf, "mips",4) == 0) {
		mc_thread->mcflags = CPU_MIPS;
	}
        return count;
}

static int __init init_proc_magic(void)
{
        struct proc_dir_entry *res;

        res = create_proc_entry("magic", 0666, NULL);
        if (res) {
	    res->read_proc = magic_read_proc;
	    res->write_proc = magic_write_proc;
	}
        return 0;
}

module_init(init_proc_magic);


