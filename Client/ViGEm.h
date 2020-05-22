#pragma once

#include "ViGEm/Client.h"

#include "main.h"

enum {
    VIGEM_DEVICE_NONE   = 0,
    VIGEM_DEVICE_DS4    = 1,
} VIGEM_DEVICE;

void vgDestroy();
bool vgInit();
bool vgSubmit(pPadPacket packet);