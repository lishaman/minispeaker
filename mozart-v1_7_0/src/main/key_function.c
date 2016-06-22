#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <fcntl.h>

#include "ini_interface.h"
#include "key_function.h"
#include "wifi_interface.h"
#include "volume_interface.h"
#include "tips_interface.h"
#include "utils_interface.h"
#include "app.h"
#include "airplay.h"
#include "linein.h"
#include "sharememory_interface.h"
#include "mozart_config.h"
#include "lcd_interface.h"
#if (SUPPORT_VR != VR_NULL)
#include "vr.h"
#endif

#if (SUPPORT_LAPSULE == 1)
#include "lapsule_control.h"
#endif
#include "ximalaya.h"

// keyvalue2str for EV_KEY, ONLY for debug.
char *keyvalue_str[] = {
	[0] = "Released",
	[1] = "Pressed",
};

// keycode2str for EV_KEY, ONLY for debug.
char *keycode_str[] = {
	[KEY_RESERVED] = "KEY_RESERVED",
	[KEY_ESC] = "KEY_ESC",
	[KEY_1] = "KEY_1",
	[KEY_2] = "KEY_2",
	[KEY_3] = "KEY_3",
	[KEY_4] = "KEY_4",
	[KEY_5] = "KEY_5",
	[KEY_6] = "KEY_6",
	[KEY_7] = "KEY_7",
	[KEY_8] = "KEY_8",
	[KEY_9] = "KEY_9",
	[KEY_0] = "KEY_0",
	[KEY_MINUS] = "KEY_MINUS",
	[KEY_EQUAL] = "KEY_EQUAL",
	[KEY_BACKSPACE] = "KEY_BACKSPACE",
	[KEY_TAB] = "KEY_TAB",
	[KEY_Q] = "KEY_Q",
	[KEY_W] = "KEY_W",
	[KEY_E] = "KEY_E",
	[KEY_R] = "KEY_R",
	[KEY_T] = "KEY_T",
	[KEY_Y] = "KEY_Y",
	[KEY_U] = "KEY_U",
	[KEY_I] = "KEY_I",
	[KEY_O] = "KEY_O",
	[KEY_P] = "KEY_P",
	[KEY_LEFTBRACE] = "KEY_LEFTBRACE",
	[KEY_RIGHTBRACE] = "KEY_RIGHTBRACE",
	[KEY_ENTER] = "KEY_ENTER",
	[KEY_LEFTCTRL] = "KEY_LEFTCTRL",
	[KEY_A] = "KEY_A",
	[KEY_S] = "KEY_S",
	[KEY_D] = "KEY_D",
	[KEY_F] = "KEY_F",
	[KEY_G] = "KEY_G",
	[KEY_H] = "KEY_H",
	[KEY_J] = "KEY_J",
	[KEY_K] = "KEY_K",
	[KEY_L] = "KEY_L",
	[KEY_SEMICOLON] = "KEY_SEMICOLON",
	[KEY_APOSTROPHE] = "KEY_APOSTROPHE",
	[KEY_GRAVE] = "KEY_GRAVE",
	[KEY_LEFTSHIFT] = "KEY_LEFTSHIFT",
	[KEY_BACKSLASH] = "KEY_BACKSLASH",
	[KEY_Z] = "KEY_Z",
	[KEY_X] = "KEY_X",
	[KEY_C] = "KEY_C",
	[KEY_V] = "KEY_V",
	[KEY_B] = "KEY_B",
	[KEY_N] = "KEY_N",
	[KEY_M] = "KEY_M",
	[KEY_COMMA] = "KEY_COMMA",
	[KEY_DOT] = "KEY_DOT",
	[KEY_SLASH] = "KEY_SLASH",
	[KEY_RIGHTSHIFT] = "KEY_RIGHTSHIFT",
	[KEY_KPASTERISK] = "KEY_KPASTERISK",
	[KEY_LEFTALT] = "KEY_LEFTALT",
	[KEY_SPACE] = "KEY_SPACE",
	[KEY_CAPSLOCK] = "KEY_CAPSLOCK",
	[KEY_F1] = "KEY_F1",
	[KEY_F2] = "KEY_F2",
	[KEY_F3] = "KEY_F3",
	[KEY_F4] = "KEY_F4",
	[KEY_F5] = "KEY_F5",
	[KEY_F6] = "KEY_F6",
	[KEY_F7] = "KEY_F7",
	[KEY_F8] = "KEY_F8",
	[KEY_F9] = "KEY_F9",
	[KEY_F10] = "KEY_F10",
	[KEY_NUMLOCK] = "KEY_NUMLOCK",
	[KEY_SCROLLLOCK] = "KEY_SCROLLLOCK",
	[KEY_KP7] = "KEY_KP7",
	[KEY_KP8] = "KEY_KP8",
	[KEY_KP9] = "KEY_KP9",
	[KEY_KPMINUS] = "KEY_KPMINUS",
	[KEY_KP4] = "KEY_KP4",
	[KEY_KP5] = "KEY_KP5",
	[KEY_KP6] = "KEY_KP6",
	[KEY_KPPLUS] = "KEY_KPPLUS",
	[KEY_KP1] = "KEY_KP1",
	[KEY_KP2] = "KEY_KP2",
	[KEY_KP3] = "KEY_KP3",
	[KEY_KP0] = "KEY_KP0",
	[KEY_KPDOT] = "KEY_KPDOT",
	[KEY_ZENKAKUHANKAKU] = "KEY_ZENKAKUHANKAKU",
	[KEY_102ND] = "KEY_102ND",
	[KEY_F11] = "KEY_F11",
	[KEY_F12] = "KEY_F12",
	[KEY_RO] = "KEY_RO",
	[KEY_KATAKANA] = "KEY_KATAKANA",
	[KEY_HIRAGANA] = "KEY_HIRAGANA",
	[KEY_HENKAN] = "KEY_HENKAN",
	[KEY_KATAKANAHIRAGANA] = "KEY_KATAKANAHIRAGANA",
	[KEY_MUHENKAN] = "KEY_MUHENKAN",
	[KEY_KPJPCOMMA] = "KEY_KPJPCOMMA",
	[KEY_KPENTER] = "KEY_KPENTER",
	[KEY_RIGHTCTRL] = "KEY_RIGHTCTRL",
	[KEY_KPSLASH] = "KEY_KPSLASH",
	[KEY_SYSRQ] = "KEY_SYSRQ",
	[KEY_RIGHTALT] = "KEY_RIGHTALT",
	[KEY_LINEFEED] = "KEY_LINEFEED",
	[KEY_HOME] = "KEY_HOME",
	[KEY_UP] = "KEY_UP",
	[KEY_PAGEUP] = "KEY_PAGEUP",
	[KEY_LEFT] = "KEY_LEFT",
	[KEY_RIGHT] = "KEY_RIGHT",
	[KEY_END] = "KEY_END",
	[KEY_DOWN] = "KEY_DOWN",
	[KEY_PAGEDOWN] = "KEY_PAGEDOWN",
	[KEY_INSERT] = "KEY_INSERT",
	[KEY_DELETE] = "KEY_DELETE",
	[KEY_MACRO] = "KEY_MACRO",
	[KEY_MUTE] = "KEY_MUTE",
	[KEY_VOLUMEDOWN] = "KEY_VOLUMEDOWN",
	[KEY_VOLUMEUP] = "KEY_VOLUMEUP",
	[KEY_POWER] = "KEY_POWER",
	[KEY_KPEQUAL] = "KEY_KPEQUAL",
	[KEY_KPPLUSMINUS] = "KEY_KPPLUSMINUS",
	[KEY_PAUSE] = "KEY_PAUSE",
	[KEY_SCALE] = "KEY_SCALE",
	[KEY_KPCOMMA] = "KEY_KPCOMMA",
	[KEY_HANGEUL] = "KEY_HANGEUL",
	[KEY_HANGUEL] = "KEY_HANGUEL",
	[KEY_HANJA] = "KEY_HANJA",
	[KEY_YEN] = "KEY_YEN",
	[KEY_LEFTMETA] = "KEY_LEFTMETA",
	[KEY_RIGHTMETA] = "KEY_RIGHTMETA",
	[KEY_COMPOSE] = "KEY_COMPOSE",
	[KEY_STOP] = "KEY_STOP",
	[KEY_AGAIN] = "KEY_AGAIN",
	[KEY_PROPS] = "KEY_PROPS",
	[KEY_UNDO] = "KEY_UNDO",
	[KEY_FRONT] = "KEY_FRONT",
	[KEY_COPY] = "KEY_COPY",
	[KEY_OPEN] = "KEY_OPEN",
	[KEY_PASTE] = "KEY_PASTE",
	[KEY_FIND] = "KEY_FIND",
	[KEY_CUT] = "KEY_CUT",
	[KEY_HELP] = "KEY_HELP",
	[KEY_MENU] = "KEY_MENU",
	[KEY_CALC] = "KEY_CALC",
	[KEY_SETUP] = "KEY_SETUP",
	[KEY_SLEEP] = "KEY_SLEEP",
	[KEY_WAKEUP] = "KEY_WAKEUP",
	[KEY_FILE] = "KEY_FILE",
	[KEY_SENDFILE] = "KEY_SENDFILE",
	[KEY_DELETEFILE] = "KEY_DELETEFILE",
	[KEY_XFER] = "KEY_XFER",
	[KEY_PROG1] = "KEY_PROG1",
	[KEY_PROG2] = "KEY_PROG2",
	[KEY_WWW] = "KEY_WWW",
	[KEY_MSDOS] = "KEY_MSDOS",
	[KEY_COFFEE] = "KEY_COFFEE",
	[KEY_SCREENLOCK] = "KEY_SCREENLOCK",
	[KEY_DIRECTION] = "KEY_DIRECTION",
	[KEY_CYCLEWINDOWS] = "KEY_CYCLEWINDOWS",
	[KEY_MAIL] = "KEY_MAIL",
	[KEY_BOOKMARKS] = "KEY_BOOKMARKS",
	[KEY_COMPUTER] = "KEY_COMPUTER",
	[KEY_BACK] = "KEY_BACK",
	[KEY_FORWARD] = "KEY_FORWARD",
	[KEY_CLOSECD] = "KEY_CLOSECD",
	[KEY_EJECTCD] = "KEY_EJECTCD",
	[KEY_EJECTCLOSECD] = "KEY_EJECTCLOSECD",
	[KEY_NEXTSONG] = "KEY_NEXTSONG",
	[KEY_PLAYPAUSE] = "KEY_PLAYPAUSE",
	[KEY_PREVIOUSSONG] = "KEY_PREVIOUSSONG",
	[KEY_STOPCD] = "KEY_STOPCD",
	[KEY_RECORD] = "KEY_RECORD",
	[KEY_REWIND] = "KEY_REWIND",
	[KEY_PHONE] = "KEY_PHONE",
	[KEY_ISO] = "KEY_ISO",
	[KEY_CONFIG] = "KEY_CONFIG",
	[KEY_HOMEPAGE] = "KEY_HOMEPAGE",
	[KEY_REFRESH] = "KEY_REFRESH",
	[KEY_EXIT] = "KEY_EXIT",
	[KEY_MOVE] = "KEY_MOVE",
	[KEY_EDIT] = "KEY_EDIT",
	[KEY_SCROLLUP] = "KEY_SCROLLUP",
	[KEY_SCROLLDOWN] = "KEY_SCROLLDOWN",
	[KEY_KPLEFTPAREN] = "KEY_KPLEFTPAREN",
	[KEY_KPRIGHTPAREN] = "KEY_KPRIGHTPAREN",
	[KEY_NEW] = "KEY_NEW",
	[KEY_REDO] = "KEY_REDO",
	[KEY_F13] = "KEY_F13",
	[KEY_F14] = "KEY_F14",
	[KEY_F15] = "KEY_F15",
	[KEY_F16] = "KEY_F16",
	[KEY_F17] = "KEY_F17",
	[KEY_F18] = "KEY_F18",
	[KEY_F19] = "KEY_F19",
	[KEY_F20] = "KEY_F20",
	[KEY_F21] = "KEY_F21",
	[KEY_F22] = "KEY_F22",
	[KEY_F23] = "KEY_F23",
	[KEY_F24] = "KEY_F24",
	[KEY_PLAYCD] = "KEY_PLAYCD",
	[KEY_PAUSECD] = "KEY_PAUSECD",
	[KEY_PROG3] = "KEY_PROG3",
	[KEY_PROG4] = "KEY_PROG4",
	[KEY_DASHBOARD] = "KEY_DASHBOARD",
	[KEY_SUSPEND] = "KEY_SUSPEND",
	[KEY_CLOSE] = "KEY_CLOSE",
	[KEY_PLAY] = "KEY_PLAY",
	[KEY_FASTFORWARD] = "KEY_FASTFORWARD",
	[KEY_BASSBOOST] = "KEY_BASSBOOST",
	[KEY_PRINT] = "KEY_PRINT",
	[KEY_HP] = "KEY_HP",
	[KEY_CAMERA] = "KEY_CAMERA",
	[KEY_SOUND] = "KEY_SOUND",
	[KEY_QUESTION] = "KEY_QUESTION",
	[KEY_EMAIL] = "KEY_EMAIL",
	[KEY_CHAT] = "KEY_CHAT",
	[KEY_SEARCH] = "KEY_SEARCH",
	[KEY_CONNECT] = "KEY_CONNECT",
	[KEY_FINANCE] = "KEY_FINANCE",
	[KEY_SPORT] = "KEY_SPORT",
	[KEY_SHOP] = "KEY_SHOP",
	[KEY_ALTERASE] = "KEY_ALTERASE",
	[KEY_CANCEL] = "KEY_CANCEL",
	[KEY_BRIGHTNESSDOWN] = "KEY_BRIGHTNESSDOWN",
	[KEY_BRIGHTNESSUP] = "KEY_BRIGHTNESSUP",
	[KEY_MEDIA] = "KEY_MEDIA",
	[KEY_SWITCHVIDEOMODE] = "KEY_SWITCHVIDEOMODE",
	[KEY_KBDILLUMTOGGLE] = "KEY_KBDILLUMTOGGLE",
	[KEY_KBDILLUMDOWN] = "KEY_KBDILLUMDOWN",
	[KEY_KBDILLUMUP] = "KEY_KBDILLUMUP",
	[KEY_SEND] = "KEY_SEND",
	[KEY_REPLY] = "KEY_REPLY",
	[KEY_FORWARDMAIL] = "KEY_FORWARDMAIL",
	[KEY_SAVE] = "KEY_SAVE",
	[KEY_DOCUMENTS] = "KEY_DOCUMENTS",
	[KEY_BATTERY] = "KEY_BATTERY",
	[KEY_BLUETOOTH] = "KEY_BLUETOOTH",
	[KEY_WLAN] = "KEY_WLAN",
	[KEY_UWB] = "KEY_UWB",
	[KEY_UNKNOWN] = "KEY_UNKNOWN",
	[KEY_VIDEO_NEXT] = "KEY_VIDEO_NEXT",
	[KEY_VIDEO_PREV] = "KEY_VIDEO_PREV",
	[KEY_BRIGHTNESS_CYCLE] = "KEY_BRIGHTNESS_CYCLE",
	[KEY_BRIGHTNESS_ZERO] = "KEY_BRIGHTNESS_ZERO",
	[KEY_DISPLAY_OFF] = "KEY_DISPLAY_OFF",
	[KEY_WIMAX] = "KEY_WIMAX",
	[KEY_RFKILL] = "KEY_RFKILL",
	[BTN_MISC] = "BTN_MISC",
	[BTN_0] = "BTN_0",
	[BTN_1] = "BTN_1",
	[BTN_2] = "BTN_2",
	[BTN_3] = "BTN_3",
	[BTN_4] = "BTN_4",
	[BTN_5] = "BTN_5",
	[BTN_6] = "BTN_6",
	[BTN_7] = "BTN_7",
	[BTN_8] = "BTN_8",
	[BTN_9] = "BTN_9",
	[BTN_MOUSE] = "BTN_MOUSE",
	[BTN_LEFT] = "BTN_LEFT",
	[BTN_RIGHT] = "BTN_RIGHT",
	[BTN_MIDDLE] = "BTN_MIDDLE",
	[BTN_SIDE] = "BTN_SIDE",
	[BTN_EXTRA] = "BTN_EXTRA",
	[BTN_FORWARD] = "BTN_FORWARD",
	[BTN_BACK] = "BTN_BACK",
	[BTN_TASK] = "BTN_TASK",
	[BTN_JOYSTICK] = "BTN_JOYSTICK",
	[BTN_TRIGGER] = "BTN_TRIGGER",
	[BTN_THUMB] = "BTN_THUMB",
	[BTN_THUMB2] = "BTN_THUMB2",
	[BTN_TOP] = "BTN_TOP",
	[BTN_TOP2] = "BTN_TOP2",
	[BTN_PINKIE] = "BTN_PINKIE",
	[BTN_BASE] = "BTN_BASE",
	[BTN_BASE2] = "BTN_BASE2",
	[BTN_BASE3] = "BTN_BASE3",
	[BTN_BASE4] = "BTN_BASE4",
	[BTN_BASE5] = "BTN_BASE5",
	[BTN_BASE6] = "BTN_BASE6",
	[BTN_DEAD] = "BTN_DEAD",
	[BTN_GAMEPAD] = "BTN_GAMEPAD",
	[BTN_A] = "BTN_A",
	[BTN_B] = "BTN_B",
	[BTN_C] = "BTN_C",
	[BTN_X] = "BTN_X",
	[BTN_Y] = "BTN_Y",
	[BTN_Z] = "BTN_Z",
	[BTN_TL] = "BTN_TL",
	[BTN_TR] = "BTN_TR",
	[BTN_TL2] = "BTN_TL2",
	[BTN_TR2] = "BTN_TR2",
	[BTN_SELECT] = "BTN_SELECT",
	[BTN_START] = "BTN_START",
	[BTN_MODE] = "BTN_MODE",
	[BTN_THUMBL] = "BTN_THUMBL",
	[BTN_THUMBR] = "BTN_THUMBR",
	[BTN_DIGI] = "BTN_DIGI",
	[BTN_TOOL_PEN] = "BTN_TOOL_PEN",
	[BTN_TOOL_RUBBER] = "BTN_TOOL_RUBBER",
	[BTN_TOOL_BRUSH] = "BTN_TOOL_BRUSH",
	[BTN_TOOL_PENCIL] = "BTN_TOOL_PENCIL",
	[BTN_TOOL_AIRBRUSH] = "BTN_TOOL_AIRBRUSH",
	[BTN_TOOL_FINGER] = "BTN_TOOL_FINGER",
	[BTN_TOOL_MOUSE] = "BTN_TOOL_MOUSE",
	[BTN_TOOL_LENS] = "BTN_TOOL_LENS",
	[BTN_TOUCH] = "BTN_TOUCH",
	[BTN_STYLUS] = "BTN_STYLUS",
	[BTN_STYLUS2] = "BTN_STYLUS2",
	[BTN_TOOL_DOUBLETAP] = "BTN_TOOL_DOUBLETAP",
	[BTN_TOOL_TRIPLETAP] = "BTN_TOOL_TRIPLETAP",
	[BTN_TOOL_QUADTAP] = "BTN_TOOL_QUADTAP",
	[BTN_WHEEL] = "BTN_WHEEL",
	[BTN_GEAR_DOWN] = "BTN_GEAR_DOWN",
	[BTN_GEAR_UP] = "BTN_GEAR_UP",
	[KEY_OK] = "KEY_OK",
	[KEY_SELECT] = "KEY_SELECT",
	[KEY_GOTO] = "KEY_GOTO",
	[KEY_CLEAR] = "KEY_CLEAR",
	[KEY_POWER2] = "KEY_POWER2",
	[KEY_OPTION] = "KEY_OPTION",
	[KEY_INFO] = "KEY_INFO",
	[KEY_TIME] = "KEY_TIME",
	[KEY_VENDOR] = "KEY_VENDOR",
	[KEY_ARCHIVE] = "KEY_ARCHIVE",
	[KEY_PROGRAM] = "KEY_PROGRAM",
	[KEY_CHANNEL] = "KEY_CHANNEL",
	[KEY_FAVORITES] = "KEY_FAVORITES",
	[KEY_EPG] = "KEY_EPG",
	[KEY_PVR] = "KEY_PVR",
	[KEY_MHP] = "KEY_MHP",
	[KEY_LANGUAGE] = "KEY_LANGUAGE",
	[KEY_TITLE] = "KEY_TITLE",
	[KEY_SUBTITLE] = "KEY_SUBTITLE",
	[KEY_ANGLE] = "KEY_ANGLE",
	[KEY_ZOOM] = "KEY_ZOOM",
	[KEY_MODE] = "KEY_MODE",
	[KEY_KEYBOARD] = "KEY_KEYBOARD",
	[KEY_SCREEN] = "KEY_SCREEN",
	[KEY_PC] = "KEY_PC",
	[KEY_TV] = "KEY_TV",
	[KEY_TV2] = "KEY_TV2",
	[KEY_VCR] = "KEY_VCR",
	[KEY_VCR2] = "KEY_VCR2",
	[KEY_SAT] = "KEY_SAT",
	[KEY_SAT2] = "KEY_SAT2",
	[KEY_CD] = "KEY_CD",
	[KEY_TAPE] = "KEY_TAPE",
	[KEY_RADIO] = "KEY_RADIO",
	[KEY_TUNER] = "KEY_TUNER",
	[KEY_PLAYER] = "KEY_PLAYER",
	[KEY_TEXT] = "KEY_TEXT",
	[KEY_DVD] = "KEY_DVD",
	[KEY_AUX] = "KEY_AUX",
	[KEY_MP3] = "KEY_MP3",
	[KEY_AUDIO] = "KEY_AUDIO",
	[KEY_VIDEO] = "KEY_VIDEO",
	[KEY_DIRECTORY] = "KEY_DIRECTORY",
	[KEY_LIST] = "KEY_LIST",
	[KEY_MEMO] = "KEY_MEMO",
	[KEY_CALENDAR] = "KEY_CALENDAR",
	[KEY_RED] = "KEY_RED",
	[KEY_GREEN] = "KEY_GREEN",
	[KEY_YELLOW] = "KEY_YELLOW",
	[KEY_BLUE] = "KEY_BLUE",
	[KEY_CHANNELUP] = "KEY_CHANNELUP",
	[KEY_CHANNELDOWN] = "KEY_CHANNELDOWN",
	[KEY_FIRST] = "KEY_FIRST",
	[KEY_LAST] = "KEY_LAST",
	[KEY_AB] = "KEY_AB",
	[KEY_NEXT] = "KEY_NEXT",
	[KEY_RESTART] = "KEY_RESTART",
	[KEY_SLOW] = "KEY_SLOW",
	[KEY_SHUFFLE] = "KEY_SHUFFLE",
	[KEY_BREAK] = "KEY_BREAK",
	[KEY_PREVIOUS] = "KEY_PREVIOUS",
	[KEY_DIGITS] = "KEY_DIGITS",
	[KEY_TEEN] = "KEY_TEEN",
	[KEY_TWEN] = "KEY_TWEN",
	[KEY_VIDEOPHONE] = "KEY_VIDEOPHONE",
	[KEY_GAMES] = "KEY_GAMES",
	[KEY_ZOOMIN] = "KEY_ZOOMIN",
	[KEY_ZOOMOUT] = "KEY_ZOOMOUT",
	[KEY_ZOOMRESET] = "KEY_ZOOMRESET",
	[KEY_WORDPROCESSOR] = "KEY_WORDPROCESSOR",
	[KEY_EDITOR] = "KEY_EDITOR",
	[KEY_SPREADSHEET] = "KEY_SPREADSHEET",
	[KEY_GRAPHICSEDITOR] = "KEY_GRAPHICSEDITOR",
	[KEY_PRESENTATION] = "KEY_PRESENTATION",
	[KEY_DATABASE] = "KEY_DATABASE",
	[KEY_NEWS] = "KEY_NEWS",
	[KEY_VOICEMAIL] = "KEY_VOICEMAIL",
	[KEY_ADDRESSBOOK] = "KEY_ADDRESSBOOK",
	[KEY_MESSENGER] = "KEY_MESSENGER",
	[KEY_DISPLAYTOGGLE] = "KEY_DISPLAYTOGGLE",
	[KEY_SPELLCHECK] = "KEY_SPELLCHECK",
	[KEY_LOGOFF] = "KEY_LOGOFF",
	[KEY_DOLLAR] = "KEY_DOLLAR",
	[KEY_EURO] = "KEY_EURO",
	[KEY_FRAMEBACK] = "KEY_FRAMEBACK",
	[KEY_FRAMEFORWARD] = "KEY_FRAMEFORWARD",
	[KEY_CONTEXT_MENU] = "KEY_CONTEXT_MENU",
	[KEY_MEDIA_REPEAT] = "KEY_MEDIA_REPEAT",
	[KEY_10CHANNELSUP] = "KEY_10CHANNELSUP",
	[KEY_10CHANNELSDOWN] = "KEY_10CHANNELSDOWN",
	[KEY_IMAGES] = "KEY_IMAGES",
	[KEY_DEL_EOL] = "KEY_DEL_EOL",
	[KEY_DEL_EOS] = "KEY_DEL_EOS",
	[KEY_INS_LINE] = "KEY_INS_LINE",
	[KEY_DEL_LINE] = "KEY_DEL_LINE",
	[KEY_FN] = "KEY_FN",
	[KEY_FN_ESC] = "KEY_FN_ESC",
	[KEY_FN_F1] = "KEY_FN_F1",
	[KEY_FN_F2] = "KEY_FN_F2",
	[KEY_FN_F3] = "KEY_FN_F3",
	[KEY_FN_F4] = "KEY_FN_F4",
	[KEY_FN_F5] = "KEY_FN_F5",
	[KEY_FN_F6] = "KEY_FN_F6",
	[KEY_FN_F7] = "KEY_FN_F7",
	[KEY_FN_F8] = "KEY_FN_F8",
	[KEY_FN_F9] = "KEY_FN_F9",
	[KEY_FN_F10] = "KEY_FN_F10",
	[KEY_FN_F11] = "KEY_FN_F11",
	[KEY_FN_F12] = "KEY_FN_F12",
	[KEY_FN_1] = "KEY_FN_1",
	[KEY_FN_2] = "KEY_FN_2",
	[KEY_FN_D] = "KEY_FN_D",
	[KEY_FN_E] = "KEY_FN_E",
	[KEY_FN_F] = "KEY_FN_F",
	[KEY_FN_S] = "KEY_FN_S",
	[KEY_FN_B] = "KEY_FN_B",
	[KEY_BRL_DOT1] = "KEY_BRL_DOT1",
	[KEY_BRL_DOT2] = "KEY_BRL_DOT2",
	[KEY_BRL_DOT3] = "KEY_BRL_DOT3",
	[KEY_BRL_DOT4] = "KEY_BRL_DOT4",
	[KEY_BRL_DOT5] = "KEY_BRL_DOT5",
	[KEY_BRL_DOT6] = "KEY_BRL_DOT6",
	[KEY_BRL_DOT7] = "KEY_BRL_DOT7",
	[KEY_BRL_DOT8] = "KEY_BRL_DOT8",
	[KEY_BRL_DOT9] = "KEY_BRL_DOT9",
	[KEY_BRL_DOT10] = "KEY_BRL_DOT10",
	[KEY_NUMERIC_0] = "KEY_NUMERIC_0",
	[KEY_NUMERIC_1] = "KEY_NUMERIC_1",
	[KEY_NUMERIC_2] = "KEY_NUMERIC_2",
	[KEY_NUMERIC_3] = "KEY_NUMERIC_3",
	[KEY_NUMERIC_4] = "KEY_NUMERIC_4",
	[KEY_NUMERIC_5] = "KEY_NUMERIC_5",
	[KEY_NUMERIC_6] = "KEY_NUMERIC_6",
	[KEY_NUMERIC_7] = "KEY_NUMERIC_7",
	[KEY_NUMERIC_8] = "KEY_NUMERIC_8",
	[KEY_NUMERIC_9] = "KEY_NUMERIC_9",
	[KEY_NUMERIC_STAR] = "KEY_NUMERIC_STAR",
	[KEY_NUMERIC_POUND] = "KEY_NUMERIC_POUND",
	[KEY_CAMERA_FOCUS] = "KEY_CAMERA_FOCUS",
	[KEY_WPS_BUTTON] = "KEY_WPS_BUTTON",
	[KEY_TOUCHPAD_TOGGLE] = "KEY_TOUCHPAD_TOGGLE",
	[KEY_TOUCHPAD_ON] = "KEY_TOUCHPAD_ON",
	[KEY_TOUCHPAD_OFF] = "KEY_TOUCHPAD_OFF",
	[KEY_CAMERA_ZOOMIN] = "KEY_CAMERA_ZOOMIN",
	[KEY_CAMERA_ZOOMOUT] = "KEY_CAMERA_ZOOMOUT",
	[KEY_CAMERA_UP] = "KEY_CAMERA_UP",
	[KEY_CAMERA_DOWN] = "KEY_CAMERA_DOWN",
	[KEY_CAMERA_LEFT] = "KEY_CAMERA_LEFT",
	[KEY_CAMERA_RIGHT] = "KEY_CAMERA_RIGHT",
	[BTN_TRIGGER_HAPPY] = "BTN_TRIGGER_HAPPY",
	[BTN_TRIGGER_HAPPY1] = "BTN_TRIGGER_HAPPY1",
	[BTN_TRIGGER_HAPPY2] = "BTN_TRIGGER_HAPPY2",
	[BTN_TRIGGER_HAPPY3] = "BTN_TRIGGER_HAPPY3",
	[BTN_TRIGGER_HAPPY4] = "BTN_TRIGGER_HAPPY4",
	[BTN_TRIGGER_HAPPY5] = "BTN_TRIGGER_HAPPY5",
	[BTN_TRIGGER_HAPPY6] = "BTN_TRIGGER_HAPPY6",
	[BTN_TRIGGER_HAPPY7] = "BTN_TRIGGER_HAPPY7",
	[BTN_TRIGGER_HAPPY8] = "BTN_TRIGGER_HAPPY8",
	[BTN_TRIGGER_HAPPY9] = "BTN_TRIGGER_HAPPY9",
	[BTN_TRIGGER_HAPPY10] = "BTN_TRIGGER_HAPPY10",
	[BTN_TRIGGER_HAPPY11] = "BTN_TRIGGER_HAPPY11",
	[BTN_TRIGGER_HAPPY12] = "BTN_TRIGGER_HAPPY12",
	[BTN_TRIGGER_HAPPY13] = "BTN_TRIGGER_HAPPY13",
	[BTN_TRIGGER_HAPPY14] = "BTN_TRIGGER_HAPPY14",
	[BTN_TRIGGER_HAPPY15] = "BTN_TRIGGER_HAPPY15",
	[BTN_TRIGGER_HAPPY16] = "BTN_TRIGGER_HAPPY16",
	[BTN_TRIGGER_HAPPY17] = "BTN_TRIGGER_HAPPY17",
	[BTN_TRIGGER_HAPPY18] = "BTN_TRIGGER_HAPPY18",
	[BTN_TRIGGER_HAPPY19] = "BTN_TRIGGER_HAPPY19",
	[BTN_TRIGGER_HAPPY20] = "BTN_TRIGGER_HAPPY20",
	[BTN_TRIGGER_HAPPY21] = "BTN_TRIGGER_HAPPY21",
	[BTN_TRIGGER_HAPPY22] = "BTN_TRIGGER_HAPPY22",
	[BTN_TRIGGER_HAPPY23] = "BTN_TRIGGER_HAPPY23",
	[BTN_TRIGGER_HAPPY24] = "BTN_TRIGGER_HAPPY24",
	[BTN_TRIGGER_HAPPY25] = "BTN_TRIGGER_HAPPY25",
	[BTN_TRIGGER_HAPPY26] = "BTN_TRIGGER_HAPPY26",
	[BTN_TRIGGER_HAPPY27] = "BTN_TRIGGER_HAPPY27",
	[BTN_TRIGGER_HAPPY28] = "BTN_TRIGGER_HAPPY28",
	[BTN_TRIGGER_HAPPY29] = "BTN_TRIGGER_HAPPY29",
	[BTN_TRIGGER_HAPPY30] = "BTN_TRIGGER_HAPPY30",
	[BTN_TRIGGER_HAPPY31] = "BTN_TRIGGER_HAPPY31",
	[BTN_TRIGGER_HAPPY32] = "BTN_TRIGGER_HAPPY32",
	[BTN_TRIGGER_HAPPY33] = "BTN_TRIGGER_HAPPY33",
	[BTN_TRIGGER_HAPPY34] = "BTN_TRIGGER_HAPPY34",
	[BTN_TRIGGER_HAPPY35] = "BTN_TRIGGER_HAPPY35",
	[BTN_TRIGGER_HAPPY36] = "BTN_TRIGGER_HAPPY36",
	[BTN_TRIGGER_HAPPY37] = "BTN_TRIGGER_HAPPY37",
	[BTN_TRIGGER_HAPPY38] = "BTN_TRIGGER_HAPPY38",
	[BTN_TRIGGER_HAPPY39] = "BTN_TRIGGER_HAPPY39",
	[BTN_TRIGGER_HAPPY40] = "BTN_TRIGGER_HAPPY40",
	[KEY_MAX] = "ERROR_KEY",
};




const char *snd_source_str[] = {
#if (SUPPORT_LAPSULE == 1)
	[SND_SRC_LAPSULE] = "internet",
#elif (SUPPORT_ATALK == 1)
	[SND_SRC_ATALK] = "internet",
#endif
#if (SUPPORT_BT == BT_BCM)
	[SND_SRC_BT_AVK] = "bluetooth",
#endif
#if (SUPPORT_LOCALPLAYER == 1)
	[SND_SRC_LOCALPLAY] = "localplay",
#endif
	[SND_SRC_LINEIN] = "linein",
};

snd_source_t snd_source = SND_SRC_NONE;

void mozart_wifi_mode(void)
{
	wifi_ctl_msg_t new_mode;
	wifi_info_t infor = get_wifi_mode();

	new_mode.force = true;

	if (infor.wifi_mode == STA || infor.wifi_mode == STANET || infor.wifi_mode == STA_WAIT){
		new_mode.cmd = SW_AP;
		memset(new_mode.param.switch_ap.ssid, 0, sizeof(new_mode.param.switch_ap.ssid));
	}
	else if (infor.wifi_mode == AP || infor.wifi_mode == AP_WAIT){
		new_mode.cmd = SW_STA;
		memset(new_mode.param.switch_sta.ssid, 0, sizeof(new_mode.param.switch_sta.ssid));
	}
	else if (infor.wifi_mode == AIRKISS || infor.wifi_mode == WIFI_NULL){
		new_mode.cmd = SW_AP;
		memset(new_mode.param.switch_ap.ssid, 0, sizeof(new_mode.param.switch_ap.ssid));
	}
	else {
		new_mode.cmd = SW_AP;
		memset(new_mode.param.switch_ap.ssid, 0, sizeof(new_mode.param.switch_ap.ssid));
	}

	strcpy(new_mode.name, app_name);
	if(request_wifi_mode(new_mode) != true)
		printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
}

void mozart_config_wifi(void)
{
	wifi_ctl_msg_t new_mode;
	new_mode.force = true;

	memset(&new_mode, 0, sizeof(wifi_ctl_msg_t));
	new_mode.cmd = SW_NETCFG;
	strcpy(new_mode.name, app_name);
	new_mode.param.network_config.timeout = -1;
	strcpy(new_mode.param.network_config.product_model, "ALINKTEST_LIVING_LIGHT_SMARTLED");
	strcpy(new_mode.param.network_config.wl_module, wifi_module_str[BROADCOM]);
	new_mode.param.network_config.method |= 0x08;
	if(request_wifi_mode(new_mode) != true)
		printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
}

void mozart_previous_song(void)
{
	memory_domain domain;
	int ret = -1;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
	control_point_info *info = NULL;
#endif

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER_DOMAIN:
		mozart_localplayer_prev_music();
		break;
#endif
#if (SUPPORT_VR != VR_NULL)
	case VR_DOMAIN:
#if (SUPPORT_VR == VR_BAIDU)
		mozart_vr_prev_music();
#elif (SUPPORT_VR == VR_SPEECH)
		ximalaya_pre();
#elif (SUPPORT_VR == VR_IFLYTEK)
		printf("TODO: vr iflytek pre music.\n");
#elif (SUPPORT_VR == VR_UNISOUND)
		printf("TODO: vr unisound pre music.\n");
#endif
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		mozart_bluetooth_avk_prev_music();
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK_DOMAIN:
		mozart_atalk_prev();
		break;
#endif
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
	default:
#if (SUPPORT_LAPSULE == 1)
		info = mozart_render_get_controler();
		if (info && info->music_provider &&
		    strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){        //is lapsule in control
			if(0 != lapsule_do_prev_song()){
				printf("lapsule_do_next_song failure.\n");
			}
		}else{
#endif
			printf("Do not support Pre song on normal DLNA mode in %s:%s:%d, may support in future.\n",
			       __FILE__, __func__, __LINE__);
#if (SUPPORT_LAPSULE == 1)
		}
#endif
#endif
		break;
	}
}

void mozart_next_song(void)
{
	memory_domain domain;
	int ret = -1;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
	control_point_info *info = NULL;
#endif

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER_DOMAIN:
		mozart_localplayer_next_music();
		break;
#endif
#if (SUPPORT_VR != VR_NULL)
	case VR_DOMAIN:
#if (SUPPORT_VR == VR_BAIDU)
		mozart_vr_next_music();
#elif (SUPPORT_VR == VR_SPEECH)
		ximalaya_next();
#elif (SUPPORT_VR == VR_IFLYTEK)
		printf("TODO: vr iflytek next music.\n");
#elif (SUPPORT_VR == VR_UNISOUND)
		printf("TODO: vr unisound next music.\n");
#endif
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		mozart_bluetooth_avk_next_music();
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK_DOMAIN:
		mozart_atalk_next();
		break;
#endif
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
	default:
#if (SUPPORT_LAPSULE == 1)
		info = mozart_render_get_controler();
		if (info && info->music_provider &&
		    strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){        //is lapsule in control
			if(0 != lapsule_do_next_song()){
				printf("lapsule_do_next_song failure.\n");
			}
		}else{
#endif
			printf("Do not support Next song on normal DLNA mode in %s:%s:%d.\n",
			       __FILE__, __func__, __LINE__);
#if (SUPPORT_LAPSULE == 1)
		}
#endif
		break;
#endif
	}
}
void mozart_stop_snd_source_play(void)
{
	memory_domain domain;
	int ret = -1;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
	control_point_info *info = NULL;
#endif
	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}
	switch (domain) {
	case UNKNOWN_DOMAIN:
		printf("system is in idle mode in %s:%s:%d.\n",
		       __FILE__, __func__, __LINE__);
		break;
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER_DOMAIN:
		mozart_localplayer_stop_playback();
		snd_source = SND_SRC_LOCALPLAY;
		break;
#endif
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY_DOMAIN:
		if (mozart_airplay_stop_playback()) {
			printf("stop airplay playback error in %s:%s:%d, return.\n",
			       __FILE__, __func__, __LINE__);
			return;
		}
		break;
#endif
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
#if (SUPPORT_LAPSULE == 1)
		info = mozart_render_get_controler();
		if (info && info->music_provider &&
		    strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){        //is lapsule in control
			if(0 != lapsule_do_stop_play()){
				printf("lapsule_do_toggle failure.\n");
				return;
			}
			snd_source = SND_SRC_LAPSULE;
		}else{
#endif
			if (mozart_render_stop_playback()) {
				printf("stop render playback error in %s:%s:%d, return.\n",
				       __FILE__, __func__, __LINE__);
				return;
			}
#if (SUPPORT_LAPSULE == 1)
		}
#endif
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK_DOMAIN:
		mozart_atalk_stop();
		snd_source = SND_SRC_ATALK;
		break;
#endif
#if (SUPPORT_VR != VR_NULL)
	case VR_DOMAIN:
#if (SUPPORT_VR == VR_BAIDU)
		if (mozart_vr_stop()) {
			printf("stop vr playback error in %s:%s:%d, return.\n",
			       __FILE__, __func__, __LINE__);
			return;
		}
#elif (SUPPORT_VR == VR_SPEECH)
#ifdef SUPPORT_SONG_SUPPLYER
		switch (SUPPORT_SONG_SUPPLYER) {
		case SONG_SUPPLYER_XIMALAYA:
			ximalaya_stop();
			break;
		case SONG_SUPPLYER_LAPSULE:
#if (SUPPORT_LAPSULE == 1)
			lapsule_do_stop_play();
#endif
			break;
		default:
			break;
		}
		snd_source = SND_SRC_LAPSULE;
#endif
#elif (SUPPORT_VR == VR_IFLYTEK)
		printf("TODO: vr iflytek stop music.\n");
#elif (SUPPORT_VR == VR_UNISOUND)
		printf("TODO: vr unisound stop music.\n");
#endif
		break;
#endif
#if 0
#if (SUPPORT_BT != BT_NULL)
	case BT_HS_DOMAIN:
		printf("bt priority is highest, do nothing in %s:%d.\n", __func__, __LINE__);
		return;
#endif
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		mozart_bluetooth_avk_stop_play();
		snd_source = SND_SRC_BT_AVK;
		break;
#endif
	default:
		printf("Not support module(domain id:%d) in %s:%d.\n", domain, __func__, __LINE__);
		break;
	}
	// turn off linein before switch to other sound source.
	if (snd_source == SND_SRC_LINEIN) {
		if (mozart_linein_is_in()) {
			mozart_linein_off();
		}
	}
}
void mozart_snd_source_switch(void)
{
	int i, cnt = 0;
	mozart_stop_snd_source_play();

	// find the next snd source.
	snd_source = (snd_source + 1) % SND_SRC_MAX;

	while (1) {
		if ((snd_source == SND_SRC_LAPSULE && get_wifi_mode().wifi_mode != STA &&
		     get_wifi_mode().wifi_mode != STANET) ||
#if (SUPPORT_LOCALPLAYER == 1)
		    (snd_source == SND_SRC_LOCALPLAY &&
		     (tfcard_status == 0 || tfcard_status == -1)) ||
#endif
		    (snd_source == SND_SRC_LINEIN && !mozart_linein_is_in())) {
				snd_source = (snd_source + 1) % SND_SRC_MAX;
				continue;
		}

		break;
	}

	// waiting for dsp idle.
	cnt = 500;
	while (cnt--) {
		if (check_dsp_opt(O_WRONLY))
			break;
		usleep(10 * 1000);
	}
	if (cnt < 0) {
		printf("5 seconds later, /dev/dsp is still in use(writing), try later in %s:%d!!\n", __func__, __LINE__);
		return;
	}

	// do it.

	printf("switching to %s mode.\n\n", snd_source_str[snd_source]);
	switch (snd_source) {
#if (SUPPORT_LOCALPLAYER == 1)
	case SND_SRC_LOCALPLAY:
#if (SUPPORT_LCD == 1)
		mozart_lcd_display(TF_PNG_PATH,TF_PLAY_CN);
#endif
		mozart_play_key("native_mode");
		mozart_localplayer_start_playback();
		break;
#endif
	case SND_SRC_LINEIN:
#if(SUPPORT_LCD == 1)
		mozart_lcd_display(LINEIN_PNG_PATH,LINEIN_PLAY_CN);
#endif
		mozart_play_key("line_mode");
		mozart_linein_on();
		break;
#if (SUPPORT_LAPSULE == 1)
	case SND_SRC_LAPSULE:
#if(SUPPORT_LCD == 1)
		mozart_lcd_display(WIFI_PNG_PATH,WIFI_PLAY_CN);
#endif
		mozart_play_key("cloud_mode");
		ximalaya_cloudplayer_start();
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case SND_SRC_ATALK:
#if(SUPPORT_LCD == 1)
		mozart_lcd_display(WIFI_PNG_PATH,WIFI_PLAY_CN);
#endif
		mozart_play_key("cloud_mode");
		mozart_atalk_start();
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case SND_SRC_BT_AVK:
#if(SUPPORT_LCD == 1)
		mozart_lcd_display(BT_PNG_PATH,BT_PLAY_CN);
#endif
		mozart_play_key("bluetooth_mode");
		mozart_bluetooth_avk_start_play();
		break;
#endif
	default:
		printf("Unknow snd source(id: %d).\n", snd_source);
#if (SUPPORT_LOCALPLAYER == 1)
		printf("force to LOCALPLAYE.\n");
#if (SUPPORT_LCD == 1)
		mozart_lcd_display(TF_PNG_PATH,TF_PLAY_CN);
#endif
		snd_source = SND_SRC_LOCALPLAY;
		mozart_localplayer_start_playback();
#endif
		break;
	}
	return;
}

void mozart_volume_up(void)
{
	int vol = 0;
	char vol_buf[8] = {};
	memory_domain domain;
	int ret = -1;

#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
	control_point_info *info = NULL;
#endif

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	if (mozart_ini_getkey("/usr/data/system.ini", "volume", "music", vol_buf)) {
		printf("failed to parse /usr/data/system.ini, set music volume to 20.\n");
		vol = 20;
	} else {
		vol = atoi(vol_buf);
	}

	if (vol == 100) {
		printf("max volume already, do nothing.\n");
		return;
	}

	vol += 10;
	if (vol > 100)
		vol = 100;
	else if(vol < 0)
		vol = 0;

	switch (domain) {
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
#if (SUPPORT_LAPSULE == 1)
		info = mozart_render_get_controler();
		if (info && info->music_provider &&
		    strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){ //is lapsule in control
			//FIXME: may be error while set volume, should check!!!!
			if(0 != lapsule_do_set_vol(vol)){
				printf("lapsule_do_set_vol failure.\n");
				return;
			}
		}
#endif
		mozart_render_set_volume(vol);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_HS_DOMAIN:
		{
			int i = 0;
			char vol_buf[8] = {};
			int hs_volume_set[16] = {0, 6, 13, 20, 26, 33, 40, 46, 53, 60, 66, 73, 80, 86, 93, 100};

			if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_call", vol_buf)) {
				printf("failed to parse /usr/data/system.ini, set BT volume to 66.\n");
				vol = 66;
			} else {
				vol = atoi(vol_buf);
			}

			if(vol >= 100) {
				printf("bt volume has maximum!\n");
				break;
			}
			for(i = 0; i < 16; i++) {
				if(hs_volume_set[i] == vol)
					break;
			}
			if(i > 15) {
				printf("failed to get bt volume %d from hs_volume_set\n", vol);
				break;
			}
			mozart_volume_set(hs_volume_set[i + 1], BT_CALL_VOLUME);
			mozart_blutooth_hs_set_volume(BTHF_VOLUME_TYPE_SPK, i + 1);
			mozart_blutooth_hs_set_volume(BTHF_VOLUME_TYPE_MIC, i + 1);
		}
		break;
	case BT_AVK_DOMAIN:
		{
			int i = 0;
			int avk_volume_max = 17;
			UINT8 avk_volume_set_dsp[17] = {0, 6, 12, 18, 25, 31, 37, 43, 50, 56, 62, 68, 75, 81, 87, 93, 100};
			UINT8 avk_volume_set_phone[17] = {0, 15, 23, 31, 39, 47, 55, 63, 71, 79, 87, 95, 103, 111, 113, 125, 127};

			if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_music", vol_buf)) {
				printf("failed to parse /usr/data/system.ini, set BT volume to 66.\n");
				vol = 63;
			} else {
				vol = atoi(vol_buf);
			}

			if(vol >= 100) {
				printf("bt volume has maximum!\n");
				break;
			}
			for(i = 0; i < avk_volume_max; i++) {
				if(avk_volume_set_dsp[i] > vol)
					break;
			}
			if(i >= avk_volume_max) {
				printf("failed to get music volume %d from avk_volume_set_dsp\n", vol);
				break;
			}
			printf("set avk dsp %d, set phone %d\n", avk_volume_set_dsp[i], avk_volume_set_phone[i]);
			mozart_volume_set(avk_volume_set_dsp[i], BT_MUSIC_VOLUME);
			mozart_bluetooth_avk_set_volume_up(avk_volume_set_phone[i]);
		}
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK_DOMAIN:
		mozart_atalk_volume_set(vol);
		break;
#endif
	default:
		mozart_volume_set(vol, MUSIC_VOLUME);
		break;
	}
}

void mozart_volume_down(void)
{
	int vol = 0;
	char vol_buf[8] = {};

	memory_domain domain;
	int ret = -1;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
	control_point_info *info = NULL;
#endif

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	if (mozart_ini_getkey("/usr/data/system.ini", "volume", "music", vol_buf)) {
		printf("failed to parse /usr/data/system.ini, set music volume to 20.\n");
		vol = 20;
	} else {
		vol = atoi(vol_buf);
	}

	if (vol == 0) {
		printf("min volume already, do nothing.\n");
		return;
	}

	vol -= 10;
	if(vol > 100)
		vol = 100;
	else if(vol < 0)
		vol = 0;

	switch (domain) {
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
#if (SUPPORT_LAPSULE == 1)
		info = mozart_render_get_controler();
		if (info && info->music_provider &&
		    strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){ //is lapsule in control
			if(0 != lapsule_do_set_vol(vol)){
				printf("lapsule_do_set_vol failure.\n");
				return;
			}
		}
#endif
		mozart_render_set_volume(vol);
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK_DOMAIN:
		mozart_atalk_volume_set(vol);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_HS_DOMAIN:
		{
			int i = 0;
			char vol_buf[8] = {};
			int hs_volume_set[16] = {0, 6, 13, 20, 26, 33, 40, 46, 53, 60, 66, 73, 80, 86, 93, 100};

			if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_call", vol_buf)) {
				printf("failed to parse /usr/data/system.ini, set BT volume to 66.\n");
				vol = 66;
			} else {
				vol = atoi(vol_buf);
			}

			if(vol <= 0) {
				printf("bt volume has maximum!\n");
				break;
			}
			for(i = 0; i < 16; i++) {
				if(hs_volume_set[i] == vol)
					break;
			}
			if(i > 15) {
				printf("failed to get bt volume %d from hs_volume_set\n", vol);
				break;
			}
			mozart_volume_set(hs_volume_set[i - 1], BT_CALL_VOLUME);
			mozart_blutooth_hs_set_volume(BTHF_VOLUME_TYPE_SPK, i - 1);
			mozart_blutooth_hs_set_volume(BTHF_VOLUME_TYPE_MIC, i - 1);
		}
		break;
	case BT_AVK_DOMAIN:
		{
			int i = 0;
			int avk_volume_max = 17;
			UINT8 avk_volume_set_dsp[17] = {0, 6, 12, 18, 25, 31, 37, 43, 50, 56, 62, 68, 75, 81, 87, 93, 100};
			UINT8 avk_volume_set_phone[17] = {0, 15, 23, 31, 39, 47, 55, 63, 71, 79, 87, 95, 103, 111, 113, 125, 127};

			if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_music", vol_buf)) {
				printf("failed to parse /usr/data/system.ini, set BT volume to 63.\n");
				vol = 63;
			} else {
				vol = atoi(vol_buf);
			}

			if(vol <= 0) {
				printf("bt volume has minimum!\n");
				break;
			}
			for(i = (avk_volume_max - 1); i >= 0; i--) {
				if(avk_volume_set_dsp[i] < vol)
					break;
			}
			if(i < 0) {
				printf("failed to get music volume %d from avk_volume_set_dsp\n", vol);
				break;
			}
			printf("set avk dsp %d, set phone %d\n", avk_volume_set_dsp[i], avk_volume_set_phone[i]);

			mozart_volume_set(avk_volume_set_dsp[i], BT_MUSIC_VOLUME);
			mozart_bluetooth_avk_set_volume_down(avk_volume_set_phone[i]);
		}
		break;
#endif
	default:
		mozart_volume_set(vol, MUSIC_VOLUME);
		break;
	}
}

void mozart_linein_on(void)
{
	mozart_play_key_sync("linein_mode");

	mozart_linein_open();

	// the next line used for invoking __func__ in mozart.c
	if (snd_source != SND_SRC_LINEIN)
		snd_source = SND_SRC_LINEIN;
}

void mozart_linein_off(void)
{
	mozart_linein_close();

	//TODO; tone contents may need change here.
	mozart_play_key("linein_off");
}

void mozart_play_pause(void)
{
	memory_domain domain;
	int ret = -1;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
	control_point_info *info = NULL;
#endif

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
	case UNKNOWN_DOMAIN:
		printf("system is in idle mode in %s:%s:%d.\n",
		       __FILE__, __func__, __LINE__);
#if (SUPPORT_LOCALPLAYER == 1)
		if (snd_source != SND_SRC_LINEIN) {
			printf("start localplayer playback...\n");
			snd_source = SND_SRC_LOCALPLAY;
			if (mozart_localplayer_start_playback()) {
				printf("start localplayer playback error in %s:%s:%d.\n",
				       __FILE__, __func__, __LINE__);
			}
		}
#endif
		break;
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER_DOMAIN:
		mozart_localplayer_play_pause();
		break;
#endif
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY_DOMAIN:
		printf("airplay protocol is playing, do not support play/pause in %s:%s:%d, return.\n",
		       __FILE__, __func__, __LINE__);
		return;
#endif
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
#if (SUPPORT_LAPSULE == 1)
		info = mozart_render_get_controler();
		if(info && info->music_provider &&
		   strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){        //is lapsule in control
			if(0 != lapsule_do_toggle()){
				printf("lapsule_do_toggle failure.\n");
			}
		}else{
#endif
			if (mozart_render_play_pause()) {
				printf("play/pause render playback error in %s:%s:%d, return.\n",
				       __FILE__, __func__, __LINE__);
				return;
			}
#if (SUPPORT_LAPSULE == 1)
		}
#endif
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK_DOMAIN:
		mozart_atalk_toogle();
		break;
#endif
#if (SUPPORT_VR != VR_NULL)
	case VR_DOMAIN:
#if (SUPPORT_VR == VR_BAIDU)
		if (mozart_vr_stop()) {
			printf("stop vr playback error in %s:%s:%d, return.\n",
			       __FILE__, __func__, __LINE__);
			return;
		}
#elif (SUPPORT_VR == VR_SPEECH)
		printf("run here: %s:%s:%d.\n", __FILE__, __func__, __LINE__);
		ximalaya_play_pause();
		printf("run here: %s:%s:%d.\n", __FILE__, __func__, __LINE__);
#elif (SUPPORT_VR == VR_IFLYTEK)
		printf("TODO: vr iflytek stop music.\n");
#elif (SUPPORT_VR == VR_UNISOUND)
		printf("TODO: vr unisound stop music.\n");
#endif
		break;
#endif

#if (SUPPORT_BT != BT_NULL)
	case BT_HS_DOMAIN:
		printf("bt priority is highest, WILL NOT play/pause in %s:%d.\n", __func__, __LINE__);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		mozart_bluetooth_avk_play_pause();
		break;
#endif
	default:
		printf("Not support module(domain id:%d) in %s:%d.\n", domain, __func__, __LINE__);
		break;
	}
}


void mozart_music_list(int keyIndex)
{
	int ret = -1;
	memory_domain domain;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
	control_point_info *info = NULL;
#endif

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
	case UNKNOWN_DOMAIN:
		printf("system is in idle mode in %s:%s:%d.\n",
		       __FILE__, __func__, __LINE__);
		break;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
	case RENDER_DOMAIN:
		info = mozart_render_get_controler();
		/* is lapsule in control */
		if (info && info->music_provider &&
		    strcmp(LAPSULE_PROVIDER, info->music_provider) == 0)
			lapsule_do_music_list(keyIndex);
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK_DOMAIN:
		mozart_atalk_next_channel();
		break;
#endif
	default:
		printf("Not support module(domain id:%d) in %s:%d.\n", domain, __func__, __LINE__);
		break;
	}
}
