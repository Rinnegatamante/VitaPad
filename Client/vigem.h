#ifdef _WIN32
#ifndef __VIGEM_H__
#define __VIGEM_H__

#include "vendor/include/ViGEm/Client.h"

#define xpad_abs_report(xpad_abs, vita_abs) \
    xpad_abs = (vita_abs - 128) * 128;

#define button_report(buttons_report, vita_button, button) \
    if (vita_button)                                       \
        buttons_report |= button;

PVIGEM_TARGET create_device();
VIGEM_ERROR send_report();
void vigem_cleanup(void);

#endif // __VIGEM_H__
#endif