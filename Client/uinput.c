#ifdef __linux__

#include "uinput.h"

int err;

struct vita create_device()
{
    struct libevdev *dev = libevdev_new();
    struct libevdev_uinput *uidev;

    libevdev_set_name(dev, "PS VITA");
    libevdev_set_id_bustype(dev, BUS_VIRTUAL);
    libevdev_set_id_vendor(dev, 0x54c);
    libevdev_set_id_product(dev, 0x2d2);
    libevdev_set_id_version(dev, 2);

    libevdev_enable_event_type(dev, EV_KEY);
    libevdev_enable_event_code(dev, EV_KEY, BTN_A, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_B, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_X, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_Y, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_TL, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_TR, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_START, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_SELECT, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_DPAD_UP, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_DPAD_DOWN, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_DPAD_LEFT, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_DPAD_RIGHT, NULL);

    struct input_absinfo joystick_abs_info = {
        .flat = 128,
        .fuzz = 0, // Already fuzzed
        .maximum = 255,
        .minimum = 0,
        .resolution = 255,
    };
    libevdev_enable_event_type(dev, EV_ABS);
    libevdev_enable_event_code(dev, EV_ABS, ABS_X, &joystick_abs_info);
    libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &joystick_abs_info);
    libevdev_enable_event_code(dev, EV_ABS, ABS_RX, &joystick_abs_info);
    libevdev_enable_event_code(dev, EV_ABS, ABS_RY, &joystick_abs_info);

    // Touchscreen (front)
    struct input_absinfo front_mt_x_info = {.minimum = 0, .maximum = 1919};
    libevdev_enable_event_code(dev, EV_ABS, ABS_MT_POSITION_X, &front_mt_x_info);
    struct input_absinfo front_mt_y_info = {.minimum = 0, .maximum = 1087};
    libevdev_enable_event_code(dev, EV_ABS, ABS_MT_POSITION_Y, &front_mt_y_info);
    struct input_absinfo front_mt_id_info = {.minimum = 0, .maximum = 255}; //TODO: Query infos
    libevdev_enable_event_code(dev, EV_ABS, ABS_MT_TRACKING_ID, &front_mt_id_info);
    struct input_absinfo front_mt_slot_info = {.minimum = 0, .maximum = 5}; // According to vitasdk docs
    libevdev_enable_event_code(dev, EV_ABS, ABS_MT_SLOT, &front_mt_slot_info);
    struct input_absinfo front_mt_pressure_info = {.minimum = 1, .maximum = 128};
    libevdev_enable_event_code(dev, EV_ABS, ABS_MT_PRESSURE, &front_mt_pressure_info);

    err = libevdev_uinput_create_from_device(dev,
                                             LIBEVDEV_UINPUT_OPEN_MANAGED,
                                             &uidev);
                                             

    sleep(1);


    // Have to create another device because sensors can't be mixed with directional axes
    // and we can't assign the back touch surface along with the touchscreen.
    // So this second device contains info for the motion sensors and the back touch surface.
    struct libevdev *sensor_dev = libevdev_new();
    struct libevdev_uinput *sensor_uidev;

    libevdev_set_name(sensor_dev, "PS VITA (Sensors)");
    libevdev_set_id_bustype(sensor_dev, BUS_VIRTUAL);
    libevdev_set_id_vendor(sensor_dev, 0x54c);
    libevdev_set_id_product(sensor_dev, 0x2d2);
    libevdev_set_id_version(sensor_dev, 3);
    libevdev_enable_property(sensor_dev, INPUT_PROP_ACCELEROMETER);

    // struct input_absinfo accel_abs_info = {}; //TODO: Query infos
    // libevdev_enable_event_type(dev, EV_ABS);
    // libevdev_enable_event_code(dev, EV_ABS, ABS_X, &accel_abs_info);
    // libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &accel_abs_info);
    // libevdev_enable_event_code(dev, EV_ABS, ABS_Z, &accel_abs_info);

    // struct input_absinfo gyro_abs_info = {}; //TODO: Query infos
    // libevdev_enable_event_code(dev, EV_ABS, ABS_RX, &gyro_abs_info);
    // libevdev_enable_event_code(dev, EV_ABS, ABS_RY, &gyro_abs_info);
    // libevdev_enable_event_code(dev, EV_ABS, ABS_RZ, &gyro_abs_info);

    struct input_absinfo mt_x_info = {.minimum = 0, .maximum = 1919};
    libevdev_enable_event_code(sensor_dev, EV_ABS, ABS_MT_POSITION_X, &mt_x_info);
    struct input_absinfo mt_y_info = {.minimum = 108, .maximum = 889};
    libevdev_enable_event_code(sensor_dev, EV_ABS, ABS_MT_POSITION_Y, &mt_y_info);
    struct input_absinfo mt_id_info = {.minimum = 0, .maximum = 255}; //TODO: Query infos
    libevdev_enable_event_code(sensor_dev, EV_ABS, ABS_MT_TRACKING_ID, &mt_id_info);
    struct input_absinfo mt_slot_info = {.minimum = 0, .maximum = 3}; // According to vitasdk docs
    libevdev_enable_event_code(sensor_dev, EV_ABS, ABS_MT_SLOT, &mt_slot_info);
    struct input_absinfo mt_pressure_info = {.minimum = 1, .maximum = 128};
    libevdev_enable_event_code(sensor_dev, EV_ABS, ABS_MT_PRESSURE, &mt_pressure_info);

    err = libevdev_uinput_create_from_device(sensor_dev,
                                             LIBEVDEV_UINPUT_OPEN_MANAGED,
                                             &sensor_uidev);

    sleep(1);

    struct vita vita_struct = {
        .dev = uidev,
        .sensor_dev = sensor_uidev,
    };

    return vita_struct;
}

int emit(struct libevdev_uinput *dev, int type, int code, int val)
{
    if ((err = libevdev_uinput_write_event(dev, type, code, val)) != 0)
        return err;
    if ((err = libevdev_uinput_write_event(dev, EV_SYN, SYN_REPORT, 0)) != 0)
        return err;

    return 0;
}

int emit_touch(struct libevdev_uinput *dev, uint8_t slot, uint8_t id, int16_t x, int16_t y, uint8_t pressure)
{
    if ((err = libevdev_uinput_write_event(dev, EV_ABS, ABS_MT_SLOT, slot)) != 0)
        return err;
    if ((err = libevdev_uinput_write_event(dev, EV_ABS, ABS_MT_TRACKING_ID, id)) != 0)
        return err;
    if ((err = libevdev_uinput_write_event(dev, EV_ABS, ABS_MT_POSITION_X, x)) != 0)
        return err;
    if ((err = libevdev_uinput_write_event(dev, EV_ABS, ABS_MT_POSITION_Y, y)) != 0)
        return err;
    if ((err = libevdev_uinput_write_event(dev, EV_ABS, ABS_MT_PRESSURE, pressure)) != 0)
        return err;

    return 0;
}

int emit_touch_sync(struct libevdev_uinput *dev)
{
    if ((err = libevdev_uinput_write_event(dev, EV_SYN, SYN_REPORT, 0)) != 0)
        return err;
}

#endif // __linux__