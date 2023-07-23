#ifndef __CTRL_H__
#define __CTRL_H__

#include <psp2/ctrl.h>
#include <psp2/motion.h>
#include <psp2/touch.h>

#include <netprotocol_generated.h>

NetProtocol::ButtonsData convert_pad_data(const SceCtrlData &data);
flatbuffers::Offset<NetProtocol::TouchData>
convert_touch_data(flatbuffers::FlatBufferBuilder &builder,
                   const SceTouchData &data);

#endif // __CTRL_H__
