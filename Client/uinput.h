#ifndef __UINPUT_H__
#define __UINPUT_H__

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <libevdev.h>
#include <libevdev-uinput.h>

struct vita create_device();
int emit(struct libevdev_uinput *dev, int type, int code, int val);
int emit_touch(struct libevdev_uinput *dev, uint8_t slot, uint8_t id, int16_t x, int16_t y, uint8_t pressure);
int emit_touch_sync(struct libevdev_uinput *dev);

struct vita
{
    struct libevdev_uinput *dev;
    struct libevdev_uinput *sensor_dev;
};

#endif // __UINPUT_H__