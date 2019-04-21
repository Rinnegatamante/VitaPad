#ifndef __VIGEM_H__
#define __VIGEM_H__

#include "vendor/include/ViGEm/Client.h"

PVIGEM_TARGET create_device();
VIGEM_ERROR send_report();
void vigem_cleanup(void);

#endif // __VIGEM_H__
