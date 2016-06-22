
#include <efuse.h>
#include "cloner.h"
#ifdef CONFIG_CMD_EFUSE
int efuse_program(struct cloner *cloner)
{
	static int enabled = 0;
	if(!enabled) {
		efuse_init(cloner->args->efuse_gpio);
		enabled = 1;
	}
	u32 partation = cloner->cmd->write.partation;
	u32 length = cloner->cmd->write.length;
	void *addr = (void *)cloner->write_req->buf;
	u32 r = 0;

	if (!!(r = efuse_write(addr, length, partation))) {
		printf("efuse write error\n");
		return r;
	}
	return r;
}
#endif
