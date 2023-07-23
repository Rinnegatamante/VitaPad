#include "ctrl.hpp"

NetProtocol::ButtonsData convert_pad_data(const SceCtrlData &data) {
  return NetProtocol::ButtonsData(
      (data.buttons & SCE_CTRL_SELECT) > 0, (data.buttons & SCE_CTRL_START) > 0,
      (data.buttons & SCE_CTRL_UP) > 0, (data.buttons & SCE_CTRL_RIGHT) > 0,
      (data.buttons & SCE_CTRL_DOWN) > 0, (data.buttons & SCE_CTRL_LEFT) > 0,
      (data.buttons & SCE_CTRL_LTRIGGER) > 0,
      (data.buttons & SCE_CTRL_RTRIGGER) > 0,
      (data.buttons & SCE_CTRL_TRIANGLE) > 0,
      (data.buttons & SCE_CTRL_CIRCLE) > 0, (data.buttons & SCE_CTRL_CROSS) > 0,
      (data.buttons & SCE_CTRL_SQUARE) > 0);
}

flatbuffers::Offset<NetProtocol::TouchData>
convert_touch_data(flatbuffers::FlatBufferBuilder &builder,
                   const SceTouchData &data) {
  std::vector<NetProtocol::TouchReport> reports(data.reportNum);
  for (size_t i = 0; i < data.reportNum; i++) {
    NetProtocol::TouchReport report(data.report[i].force, data.report[i].id,
                                    data.report[i].x, data.report[i].y);
    reports[i] = report;
  }

  return NetProtocol::CreateTouchDataDirect(builder, &reports);
}
