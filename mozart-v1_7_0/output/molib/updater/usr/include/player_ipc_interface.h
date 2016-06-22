#ifndef __PLAYER_IPC_INTERFACE_HEAD__
#define __PLAYER_IPC_INTERFACE_HEAD__

#include <stdint.h>

#define URL_SIZE	256

#define PLAYER_MSG_KEY		0x01020304
#define PLAYER_RESOURCE_TYPE	'R'
#define PLAYER_CONTROL_TYPE	'C'

#define SHAIRPORT_MSG_KEY	0x01020304
#define SHAIRPORT_FADE_TYPE	'F'

#define PLAYER_SHM_KEY		0X00112233
#define PLAYER_SHM_SIZE		(URL_SIZE + 64)

#define PLAYER_SOCK	"/var/run/player-server/player_socket"
#define TINY_SOCK	"/var/run/player-server/tiny_socket"

enum sOPT {
	OPTION_MASK	= ~((uint16_t)0),
	SO_STOPABLE	= (1 << 0),
	SO_SYNC		= (1 << 1),
	SO_STOP		= (1 << 2),
};

typedef struct {
	/* url[0] equal 0, means url is empty */
	char url[URL_SIZE];
	uint32_t opt;

	uint32_t fade_msec;
	uint32_t fade_volume;
} simple_request_t;

typedef struct {
	short	ok;
} simple_respond_t;

/************************************************/
typedef enum {
	FADE_IN = 0,
	FADE_OUT,
} fade_opt_t;

typedef struct {
	fade_opt_t fade_opt;
	uint32_t fade_msec;
	uint32_t fade_volume;
} shairport_fade_t;

typedef struct {
	long msg_type;
	shairport_fade_t fade;
} shairport_fade_msg_t;

#endif /* __PLAYER_IPC_INTERFACE_HEAD__ */
