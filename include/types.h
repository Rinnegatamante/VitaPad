#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct _buttons
{
    bool start;
    bool select;
    bool up;
    bool right;
    bool down;
    bool left;
    bool lt;
    bool rt;
    bool triangle;
    bool circle;
    bool cross;
    bool square;
} Buttons;

typedef struct _packet
{
    Buttons buttons;
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
} Packet;

#define GAMEPAD_PORT 5000

#endif // __TYPES_H__