#ifndef __UINPUT_H__
#define __UINPUT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

    struct vita create_device();
    int emit(struct libevdev_uinput *dev, int type, int code, int val);
    int emit_touch(struct libevdev_uinput *dev, uint8_t slot, uint8_t id, int16_t x, int16_t y, uint8_t pressure);
    int emit_touch_sync(struct libevdev_uinput *dev);

    struct vita
    {
        struct libevdev_uinput *dev;
        struct libevdev_uinput *sensor_dev;
    };

#define emit_abs(dev_fd, axis, value) \
    emit(dev_fd, EV_ABS, axis, value)

#define emit_button(dev_fd, btn, value) \
    emit(dev_fd, EV_KEY, btn, value)

#ifdef __cplusplus
}
#endif

#endif // __UINPUT_H__