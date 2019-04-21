#include "ctrl.h"

#include <stdlib.h>

bool is_different(SceCtrlData *data1, SceCtrlData *data2)
{
    if (data1->buttons != data2->buttons)
    {
        return true;
    }

    if (data1->lx != data2->lx)
    {
        return true;
    }
    if (data1->ly != data2->ly)
    {
        return true;
    }
    if (data1->rx != data2->rx)
    {
        return true;
    }
    if (data1->ry != data2->ry)
    {
        return true;
    }

    return false;
}

Buttons convert(SceCtrlData data)
{
    Buttons buttons = {0};

    if (data.buttons & SCE_CTRL_SELECT)
    {
        buttons.select = true;
    }
    if (data.buttons & SCE_CTRL_START)
    {
        buttons.start = true;
    }
    if (data.buttons & SCE_CTRL_UP)
    {
        buttons.up = true;
    }
    if (data.buttons & SCE_CTRL_RIGHT)
    {
        buttons.right = true;
    }
    if (data.buttons & SCE_CTRL_DOWN)
    {
        buttons.down = true;
    }
    if (data.buttons & SCE_CTRL_LEFT)
    {
        buttons.left = true;
    }
    if (data.buttons & SCE_CTRL_LTRIGGER)
    {
        buttons.lt = true;
    }
    if (data.buttons & SCE_CTRL_RTRIGGER)
    {
        buttons.rt = true;
    }
    if (data.buttons & SCE_CTRL_TRIANGLE)
    {
        buttons.triangle = true;
    }
    if (data.buttons & SCE_CTRL_CIRCLE)
    {
        buttons.circle = true;
    }
    if (data.buttons & SCE_CTRL_CROSS)
    {
        buttons.cross = true;
    }
    if (data.buttons & SCE_CTRL_SQUARE)
    {
        buttons.square = true;
    }

    return buttons;
}