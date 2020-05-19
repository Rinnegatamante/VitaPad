#include "ctrl.hpp"

#include <psp2/types.h>

Pad::ButtonsData convert_pad_data(SceCtrlData data)
{
    return Pad::ButtonsData((data.buttons & SCE_CTRL_SELECT) > 0,
                            (data.buttons & SCE_CTRL_START) > 0,
                            (data.buttons & SCE_CTRL_UP) > 0,
                            (data.buttons & SCE_CTRL_RIGHT) > 0,
                            (data.buttons & SCE_CTRL_DOWN) > 0,
                            (data.buttons & SCE_CTRL_LEFT) > 0,
                            (data.buttons & SCE_CTRL_LTRIGGER) > 0,
                            (data.buttons & SCE_CTRL_RTRIGGER) > 0,
                            (data.buttons & SCE_CTRL_TRIANGLE) > 0,
                            (data.buttons & SCE_CTRL_CIRCLE) > 0,
                            (data.buttons & SCE_CTRL_CROSS) > 0,
                            (data.buttons & SCE_CTRL_SQUARE) > 0);
}

flatbuffers::Offset<Pad::TouchData> convert_touch_data(flatbuffers::FlatBufferBuilder &builder, SceTouchData data, Pad::TouchPort port)
{
    std::vector<Pad::TouchReport> reports;
    for (uint8_t i = 0; i < data.reportNum; i++)
    {
        Pad::TouchReport report = Pad::TouchReport(data.report[i].force,
                                                   data.report[i].id,
                                                   data.report[i].x,
                                                   data.report[i].y);
        reports.push_back(report);
    }
    return Pad::CreateTouchDataDirect(builder,
                                      port,
                                      &reports,
                                      data.reportNum);
}