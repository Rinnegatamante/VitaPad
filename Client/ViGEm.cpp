#include <Windows.h>

#include "ViGEm.h"

PVIGEM_CLIENT client;
PVIGEM_TARGET target;

void vgDestroy()
{
    vigem_target_remove(client, target);
    vigem_target_free(target);
    target = NULL;

    vigem_disconnect(client);
    vigem_free(client);
    client = NULL;
}

bool vgInit()
{
    do
    {
        if (NULL == (client = vigem_alloc())) break;
        if (!VIGEM_SUCCESS(vigem_connect(client))) break;
        if (NULL == (target = vigem_target_ds4_alloc())) break;
        if (!VIGEM_SUCCESS(vigem_target_add(client, target))) break;
        return true;
    } while (false);
    vgDestroy();
    return false;
}

bool vgSubmit(pPadPacket packet)
{
    DS4_REPORT report;
    DS4_REPORT_INIT(&report);

    report.bThumbLX = packet->lx;
    report.bThumbLY = packet->ly;
    report.bThumbRX = packet->rx;
    report.bThumbRY = packet->ry;

    if (packet->buttons & SCE_CTRL_SELECT)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_OPTIONS;
    }
    if (packet->buttons & SCE_CTRL_START)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_SHARE;
    }
    if (packet->buttons & SCE_CTRL_LTRIGGER)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_TRIGGER_LEFT;
        report.bTriggerL = 0xFF;
    }
    if (packet->buttons & SCE_CTRL_RTRIGGER)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_TRIGGER_RIGHT;
        report.bTriggerR = 0xFF;
    }
    if (packet->buttons & SCE_CTRL_TRIANGLE)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_TRIANGLE;
    }
    if (packet->buttons & SCE_CTRL_CIRCLE)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_CIRCLE;
    }
    if (packet->buttons & SCE_CTRL_CROSS)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_CROSS;
    }
    if (packet->buttons & SCE_CTRL_SQUARE)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_SQUARE;
    }
    if (packet->click & MOUSE_MOV && packet->tx < SCREEN_WIDTH / 2)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_SHOULDER_LEFT;
    }
    if (packet->click & MOUSE_MOV && packet->tx >= SCREEN_WIDTH / 2)
    {
        report.wButtons = report.wButtons | DS4_BUTTON_SHOULDER_RIGHT;
    }

    if (packet->buttons & SCE_CTRL_UP) DS4_SET_DPAD(&report, DS4_BUTTON_DPAD_NORTH);
    if (packet->buttons & SCE_CTRL_RIGHT) DS4_SET_DPAD(&report, DS4_BUTTON_DPAD_EAST);
    if (packet->buttons & SCE_CTRL_DOWN) DS4_SET_DPAD(&report, DS4_BUTTON_DPAD_SOUTH);
    if (packet->buttons & SCE_CTRL_LEFT) DS4_SET_DPAD(&report, DS4_BUTTON_DPAD_WEST);

    if (packet->buttons & SCE_CTRL_UP
        && packet->buttons & SCE_CTRL_RIGHT) DS4_SET_DPAD(&report, DS4_BUTTON_DPAD_NORTHEAST);
    if (packet->buttons & SCE_CTRL_RIGHT
        && packet->buttons & SCE_CTRL_DOWN) DS4_SET_DPAD(&report, DS4_BUTTON_DPAD_SOUTHEAST);
    if (packet->buttons & SCE_CTRL_DOWN
        && packet->buttons & SCE_CTRL_LEFT) DS4_SET_DPAD(&report, DS4_BUTTON_DPAD_SOUTHWEST);
    if (packet->buttons & SCE_CTRL_LEFT
        && packet->buttons & SCE_CTRL_UP) DS4_SET_DPAD(&report, DS4_BUTTON_DPAD_NORTHWEST);
    
    return VIGEM_SUCCESS(vigem_target_ds4_update(client, target, report));
}