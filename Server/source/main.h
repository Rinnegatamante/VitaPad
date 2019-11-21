#ifndef __MAIN_H__
#define __MAIN_H__

#include <psp2/kernel/threadmgr.h>
#include <psp2/types.h>

typedef struct ThreadMessage
{
    SceUID msg_pipe;
    SceUID ev_flag;
} ThreadMessage;

typedef struct MainThreadMessage
{
    SceUID ev_flag_connect_state;
} MainThreadMessage;

enum EVENT_FLAGS
{
    PAD_CHANGE = 0x1,
    TOUCH_CHANGE = 0x2,
    MOTION_CHANGE = 0x4,
    STATE_CONNECT = 0x8,
    STATE_DISCONNECT = 0x16
};

#endif // __MAIN_H__