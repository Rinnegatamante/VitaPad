#ifndef __NET_TYPES_H__
#define __NET_TYPES_H__

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

typedef struct _pad_packet
{
    Buttons buttons;
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
} PadPacket;

typedef enum _touch_port
{
    FRONT = 0, // SCE_TOUCH_PORT_FRONT = 0
    BACK = 1,  // SCE_TOUCH_PORT_BACK = 1
} TOUCH_PORT;

typedef struct _touch_report
{
    uint8_t pressure;
    uint8_t id;
    int16_t x;
    int16_t y;
} TouchReport;

typedef struct _touch_packet
{
    TOUCH_PORT port;
    TouchReport reports[6];
    uint8_t num_rep;
} TouchPacket;

typedef struct _vector3
{
    float x;
    float y;
    float z;
} Vector3;

typedef struct _motion_packet
{
    Vector3 gyro;
    Vector3 accelerometer;
} MotionPacket;

typedef enum _packet_type {
    PAD,
    TOUCH,
    MOTION
} PACKET_TYPE;

typedef struct _packet {
    PACKET_TYPE type;
    typedef union _packet
    {
        PadPacket pad;
        TouchPacket touch;
        MotionPacket motion;
    } packet;
} Packet;

typedef enum _config_touch
{
    DISABLED = 0,
    FRONT = 0x1,
    BACK = 0x2,
    FRONT_AND_BACK = FRONT | BACK
} CONFIG_TOUCH;

typedef struct _config_packet
{
    CONFIG_TOUCH touch_config;
    bool motion_activate;
} ConfigPacket;

#define NET_PORT 5000

#endif //__NET_TYPES_H_
