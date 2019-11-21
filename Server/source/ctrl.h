#ifndef __CTRL_H__
#define __CTRL_H__

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/motion.h>

#include "../../include/net_types.h"


bool is_different(SceCtrlData *data1, SceCtrlData *data2);
bool is_different_touch(SceTouchData *data1, SceTouchData *data2);
bool is_different_motion(SceMotionSensorState *data1, SceMotionSensorState *data2);
Buttons convert(SceCtrlData data);

#endif // __CTRL_H__