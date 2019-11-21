#include "ctrl.h"

#include <stdlib.h>

#define TOUCH_MARGIN 3
#define JOYSTICK_MARGIN 1
#define MOTION_MARGIN 1 //TODO: Figure out for real

bool is_different(SceCtrlData *data1, SceCtrlData *data2)
{
    return (data1->buttons != data2->buttons) 
    || (abs(data1->lx - data2->lx) > JOYSTICK_MARGIN) 
    || (abs(data1->ly - data2->ly) > JOYSTICK_MARGIN) 
    || (abs(data1->rx - data2->rx) > JOYSTICK_MARGIN) 
    || (abs(data1->ry - data2->ry) > JOYSTICK_MARGIN);
}

bool is_different_touch(SceTouchData *data1, SceTouchData *data2)
{
    if (data1->reportNum != data2->reportNum)
        return true;

    else
    {
        for (size_t i = 0; i < data1->reportNum; i++)
        {
            if (&data1->report[i].id != &data2->report[i].id)
                return true;
            else if (&data1->report[i].force != &data2->report[i].force)
                return true;
            else if (abs(&data1->report[i].x - &data2->report[i].x) > TOUCH_MARGIN)
                return true;
            else if (abs(&data1->report[i].y - &data2->report[i].y) > TOUCH_MARGIN)
                return true;
        }
        return false;
    }
}

bool is_different_vector(SceFVector3 *vec1, SceFVector3 *vec2)
{
    return (abs(vec1->x - vec2->x) > MOTION_MARGIN) || (abs(vec1->y - vec2->y) > MOTION_MARGIN) || (abs(vec1->z - vec2->z) > MOTION_MARGIN);
}

bool is_different_motion(SceMotionSensorState *data1, SceMotionSensorState *data2)
{
    return is_different_vector(&data1->accelerometer, &data2->accelerometer) || is_different_vector(&data1->gyro, &data2->gyro);
}

Buttons convert(SceCtrlData data)
{
    Buttons buttons = {0};

    if (data.buttons & SCE_CTRL_SELECT)
        buttons.select = true;
    if (data.buttons & SCE_CTRL_START)
        buttons.start = true;
    if (data.buttons & SCE_CTRL_UP)
        buttons.up = true;
    if (data.buttons & SCE_CTRL_RIGHT)
        buttons.right = true;
    if (data.buttons & SCE_CTRL_DOWN)
        buttons.down = true;
    if (data.buttons & SCE_CTRL_LEFT)
        buttons.left = true;
    if (data.buttons & SCE_CTRL_LTRIGGER)
        buttons.lt = true;
    if (data.buttons & SCE_CTRL_RTRIGGER)
        buttons.rt = true;
    if (data.buttons & SCE_CTRL_TRIANGLE)
        buttons.triangle = true;
    if (data.buttons & SCE_CTRL_CIRCLE)
        buttons.circle = true;
    if (data.buttons & SCE_CTRL_CROSS)
        buttons.cross = true;
    if (data.buttons & SCE_CTRL_SQUARE)
        buttons.square = true;

    return buttons;
}