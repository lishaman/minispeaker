#include <linux/fb.h>

/* structures for frame buffer ioctl */
struct jzfb_fg_pos {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 x;
	__u32 y;
};

struct jzfb_fg_size {
	__u32 fg;
	__u32 w;
	__u32 h;
};

struct jzfb_fg_alpha {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode; /* 0:global alpha, 1:pixel alpha */
	__u32 value; /* 0x00-0xFF */
};

struct jzfb_bg {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct jzfb_color_key {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode; /* 0:color key, 1:mask color key */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct jzfb_mode_res {
	__u32 index; /* 1-64 */
	__u32 w;
	__u32 h;
};

/* ioctl commands base fb.h FBIO_XXX */
/* image_enh.h: 0x142 -- 0x162 */
#define JZFB_GET_MODENUM		_IOR('F', 0x100, int)
#define JZFB_GET_MODELIST		_IOR('F', 0x101, int)
#define JZFB_SET_VIDMEM			_IOW('F', 0x102, unsigned int *)
#define JZFB_SET_MODE			_IOW('F', 0x103, int)
#define JZFB_ENABLE			_IOW('F', 0x104, int)
#define JZFB_GET_RESOLUTION		_IOWR('F', 0x105, struct jzfb_mode_res)

/* Reserved for future extend */
#define JZFB_SET_FG_SIZE		_IOW('F', 0x116, struct jzfb_fg_size)
#define JZFB_GET_FG_SIZE		_IOWR('F', 0x117, struct jzfb_fg_size)
#define JZFB_SET_FG_POS			_IOW('F', 0x118, struct jzfb_fg_pos)
#define JZFB_GET_FG_POS			_IOWR('F', 0x119, struct jzfb_fg_pos)
#define JZFB_GET_BUFFER			_IOR('F', 0x120, int)
#define JZFB_GET_LCDC_ID		_IOR('F', 0x121, int)
#define JZFB_GET_LCDTYPE		_IOR('F', 0x122, int)
/* Reserved for future extend */
#define JZFB_SET_ALPHA			_IOW('F', 0x123, struct jzfb_fg_alpha)
#define JZFB_SET_BACKGROUND		_IOW('F', 0x124, struct jzfb_bg)
#define JZFB_SET_COLORKEY		_IOW('F', 0x125, struct jzfb_color_key)
#define JZFB_ENABLE_LCDC_CLK		_IOW('F', 0x130, int)
/* Reserved for future extend */
#define JZFB_ENABLE_FG0			_IOW('F', 0x139, int)
#define JZFB_ENABLE_FG1			_IOW('F', 0x140, int)

/* Reserved for future extend */
#define JZFB_SET_VSYNCINT		_IOW('F', 0x210, int)

#define JZFB_SET_PAN_SYNC		_IOW('F', 0x220, int)

#define JZFB_SET_NEED_SYSPAN	_IOR('F', 0x310, int)
#define NOGPU_PAN				_IOR('F', 0x311, int)

/* Support for multiple buffer, can be switched. */
#define JZFB_ACQUIRE_BUFFER		_IOR('F', 0x441, int)	//acquire new buffer and to display
#define JZFB_RELEASE_BUFFER		_IOR('F', 0x442, int)	//release the acquire buffer
#define JZFB_CHANGE_BUFFER		_IOR('F', 0x443, int)	//change the acquire buffer

