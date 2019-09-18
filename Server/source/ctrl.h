#ifndef __CTRL_H__
#define __CTRL_H__

#include <psp2/ctrl.h>

#include "../../include/types.h"


bool is_different(SceCtrlData *data1, SceCtrlData *data2);
Buttons convert(SceCtrlData data);

#endif // __CTRL_H__