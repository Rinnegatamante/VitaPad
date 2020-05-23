#ifndef C_CTRL_HPP__
#define C_CTRL_HPP__

#include <pad.fbs.hpp>
#ifdef __linux__
#include "uinput.h"
void emit_pad_data(const Pad::MainPacket *pad_data, struct vita &vita_dev);
#elif defined(_WIN32)
#include "vigem.h"
void emit_pad_data(const Pad::MainPacket *pad_data, PVIGEM_TARGET target)
#endif

#endif