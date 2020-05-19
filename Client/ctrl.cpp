#include "ctrl.hpp"

#ifdef __linux__
void emit_pad_data(const Pad::MainPacket *pad_data, struct vita &vita_dev)
{
    auto buttons = pad_data->Buttons();

    emit_abs(vita_dev.dev, ABS_X, pad_data->lx());
    emit_abs(vita_dev.dev, ABS_Y, pad_data->ly());

    emit_abs(vita_dev.dev, ABS_RX, pad_data->rx());
    emit_abs(vita_dev.dev, ABS_RY, pad_data->ry());

    emit_button(vita_dev.dev, BTN_B, buttons->circle());
    emit_button(vita_dev.dev, BTN_A, buttons->cross());
    emit_button(vita_dev.dev, BTN_Y, buttons->square());
    emit_button(vita_dev.dev, BTN_X, buttons->triangle());
    emit_button(vita_dev.dev, BTN_TL, buttons->lt());
    emit_button(vita_dev.dev, BTN_TR, buttons->rt());
    emit_button(vita_dev.dev, BTN_START, buttons->start());
    emit_button(vita_dev.dev, BTN_SELECT, buttons->select());
    emit_button(vita_dev.dev, BTN_DPAD_UP, buttons->up());
    emit_button(vita_dev.dev, BTN_DPAD_DOWN, buttons->down());
    emit_button(vita_dev.dev, BTN_DPAD_LEFT, buttons->left());
    emit_button(vita_dev.dev, BTN_DPAD_RIGHT, buttons->right());

    auto motion = pad_data->motion();
    auto accel = motion->accelerometer();
    emit_abs(vita_dev.sensor_dev, ABS_X, accel.x());
    emit_abs(vita_dev.sensor_dev, ABS_Y, accel.y());
    emit_abs(vita_dev.sensor_dev, ABS_Z, accel.z());
    auto gyro = motion->gyro();
    emit_abs(vita_dev.sensor_dev, ABS_RX, gyro.x());
    emit_abs(vita_dev.sensor_dev, ABS_RY, gyro.y());
    emit_abs(vita_dev.sensor_dev, ABS_RZ, gyro.z());

    auto touches = pad_data->touch();
    for (auto const &touch_data : *touches)
    {
        switch (touch_data->port())
        {
        case Pad::TouchPort::Front:
        {
            auto reports = touch_data->reports();
            for (size_t i = 0; i != touch_data->num_reports(); i++)
            {
                auto report = reports->Get(i);
                emit_touch(vita_dev.dev, i,
                           report->id(), report->x(),
                           report->y(), report->pressure());
            }
            emit_touch_sync(vita_dev.dev);
        }
        break;

        case Pad::TouchPort::Back:
        {
            auto reports = touch_data->reports();
            for (size_t i = 0; i != touch_data->num_reports(); i++)
            {
                auto report = reports->Get(i);
                emit_touch(vita_dev.sensor_dev, i,
                           report->id(), report->x(),
                           report->y(), report->pressure());
            }
            emit_touch_sync(vita_dev.sensor_dev);
        }
        break;

        default:
            break;
        }
    }
}

#elif defined(_WIN32)
void emit_pad_data(const Pad::MainPacket *pad_data, PVIGEM_TARGET target)
{
    assert(this->target != NULL && vigem_target_is_attached(this->target));
    XUSB_REPORT x_report;
    XUSB_REPORT_INIT(&x_report);

    xpad_abs_report(x_report.sThumbLX, pad_data->lx());
    xpad_abs_report(x_report.sThumbLY, pad_data->ly());
    xpad_abs_report(x_report.sThumbRX, pad_data->rx());
    xpad_abs_report(x_report.sThumbRY, pad_data->ry());

    auto buttons = pad_data->Buttons();
    button_report(x_report.wButtons, buttons->circle(), XUSB_GAMEPAD_B);
    button_report(x_report.wButtons, buttons->cross(), XUSB_GAMEPAD_A);
    button_report(x_report.wButtons, buttons->square(), XUSB_GAMEPAD_X);
    button_report(x_report.wButtons, buttons->triangle(), XUSB_GAMEPAD_Y);
    button_report(x_report.wButtons, buttons->lt(), XUSB_GAMEPAD_LEFT_SHOULDER);
    button_report(x_report.wButtons, buttons->rt(), XUSB_GAMEPAD_RIGHT_SHOULDER);
    button_report(x_report.wButtons, buttons->start(), XUSB_GAMEPAD_START);
    button_report(x_report.wButtons, buttons->select(), XUSB_GAMEPAD_BACK);
    button_report(x_report.wButtons, buttons->up(), XUSB_GAMEPAD_DPAD_UP);
    button_report(x_report.wButtons, buttons->down(), XUSB_GAMEPAD_DPAD_DOWN);
    button_report(x_report.wButtons, buttons->left(), XUSB_GAMEPAD_DPAD_LEFT);
    button_report(x_report.wButtons, buttons->right(), XUSB_GAMEPAD_DPAD_RIGHT);

    switch (get_target_type(this->target))
    {
    case Xbox360Wired:

        assert(VIGEM_SUCCESS(send_report(x_report, target)));

        break;

    case DualShock4Wired:
        //TODO: Touch and motion support when it will be merged to ViGEm
        DS4_REPORT d_report;
        DS4_REPORT_INIT(&d_report);

        XUSB_TO_DS4_REPORT(&x_report, &d_report);

        assert(VIGEM_SUCCESS(send_report(d_report, target)));

        break;

    default:
        break;
    }
}
#endif