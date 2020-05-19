#ifndef __CTRL_H__
#define __CTRL_H__

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/motion.h>

#include <pad.fbs.hpp>

Pad::ButtonsData convert_pad_data(SceCtrlData data);
flatbuffers::Offset<Pad::TouchData> convert_touch_data(flatbuffers::FlatBufferBuilder &builder, SceTouchData data);

#endif // __CTRL_H__