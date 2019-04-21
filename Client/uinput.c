#ifdef __linux__

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <linux/input.h>
#include <linux/uinput.h>

int create_device()
{
    int fd = open("/dev/uinput", O_WRONLY);

    if (fd == -1)
        return -1;

    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_A) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_B) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_X) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_Y) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_TL) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_TR) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_START) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_SELECT) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_UP) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_DOWN) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_LEFT) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT) == -1)
    {
        return -1;
    }

    if (ioctl(fd, UI_SET_EVBIT, EV_ABS) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_ABSBIT, ABS_X) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_ABSBIT, ABS_Y) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_ABSBIT, ABS_RX) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_SET_ABSBIT, ABS_RY) == -1)
    {
        return -1;
    }

    struct input_absinfo abs_info;
    abs_info.flat = 128;
    abs_info.fuzz = 2;
    abs_info.maximum = 255;
    abs_info.minimum = 0;
    abs_info.resolution = 255;

    struct uinput_abs_setup abs_x;
    abs_x.code = ABS_X;
    abs_x.absinfo = abs_info;

    struct uinput_abs_setup abs_y;
    abs_y.code = ABS_Y;
    abs_y.absinfo = abs_info;

    struct uinput_abs_setup abs_rx;
    abs_rx.code = ABS_RX;
    abs_rx.absinfo = abs_info;

    struct uinput_abs_setup abs_ry;
    abs_ry.code = ABS_RY;
    abs_ry.absinfo = abs_info;

    if (ioctl(fd, UI_ABS_SETUP, &abs_x) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_ABS_SETUP, &abs_y) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_ABS_SETUP, &abs_rx) == -1)
    {
        return -1;
    }
    if (ioctl(fd, UI_ABS_SETUP, &abs_ry) == -1)
    {
        return -1;
    }

    struct uinput_setup uidev;
    memset(&uidev, 0, sizeof(uidev));
    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor = 0x54c;
    uidev.id.product = 0x2d2;
    uidev.id.version = 2;
    strcpy(uidev.name, "PSVITA");

    if (ioctl(fd, UI_DEV_SETUP, &uidev) == -1)
    {
        return -1;
    }
    
    if (ioctl(fd, UI_DEV_CREATE) == -1)
    {
        return -1;
    }

    sleep(1);

    return fd;
}

int emit(int fd, int type, int code, int val)
{
    struct input_event ev;

    ev.type = type;
    ev.code = code;
    ev.value = val;

    if (write(fd, &ev, sizeof(ev)) == -1)
        return -1;

    // Sync data
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;

    if (write(fd, &ev, sizeof(ev)) == -1)
        return -1;

    return 0;
}

#endif // __linux__