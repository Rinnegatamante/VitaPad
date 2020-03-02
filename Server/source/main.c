#include <vita2d.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/types.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <psp2/touch.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/motion.h>
#include <debugnet.h>

#include "../../include/net_types.h"

#include "ctrl.h"
#include "main.h"

static int control_thread(unsigned int args, void *argp)
{
	ThreadMessage *message = (ThreadMessage *)argp;

	SceCtrlData pad, old_pad = {0};
	while (true)
	{
		sceCtrlPeekBufferPositive(0, &pad, 1);

		PadPacket pkg = {
			.buttons = convert(pad),
			.lx = pad.lx,
			.ly = pad.ly,
			.rx = pad.rx,
			.ry = pad.ry};

		sceKernelSetEventFlag(*message->ev_flag, PAD_CHANGE);
		sceKernelSendMsgPipe(*message->msg_pipe, &pkg, sizeof(PadPacket), 0, NULL, NULL);
		old_pad = pad;
		sceKernelReceiveMsgPipe(*message->msg_pipe, NULL, 0, 0, NULL, NULL);
	}
	return 0;
}

static int motion_thread(unsigned int args, void *argp)
{
	ThreadMessage *message = (ThreadMessage *)argp;

	SceMotionSensorState motion_data, old_motion_data = {0}; //TODO: Needs calibration
	while (true)
	{
		sceMotionGetSensorState(&motion_data, 1);
		MotionPacket packet;
		memcpy(&packet.accelerometer, &motion_data.accelerometer, sizeof(Vector3));
		memcpy(&packet.gyro, &motion_data.gyro, sizeof(Vector3));

		sceKernelSetEventFlag(*message->ev_flag, MOTION_CHANGE);
		sceKernelSendMsgPipe(*message->msg_pipe, &packet, sizeof(MotionPacket), 0, NULL, NULL);
		old_motion_data = motion_data;
		sceKernelReceiveMsgPipe(*message->msg_pipe, NULL, 0, 0, NULL, NULL);
	}
	return 0;
}

volatile CONFIG_TOUCH touch_config = FRONT_AND_BACK;
static int touch_thread(unsigned int args, void *argp)
{
	ThreadMessage *message = (ThreadMessage *)argp;

	SceTouchData touch_data_front, old_touch_data_front, touch_data_back, old_touch_data_back = {0};
	while (true)
	{
		if (touch_config & FRONT)
		{
			sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch_data_front, 1);
			TouchPacket packet;
			packet.port = FRONT;
			packet.num_rep = touch_data_front.reportNum;
			for (uint8_t i = 0; i < packet.num_rep; i++)
			{
				packet.reports[i].pressure = touch_data_front.report[i].force,
				packet.reports[i].id = touch_data_front.report[i].id,
				packet.reports[i].x = touch_data_front.report[i].x,
				packet.reports[i].y = touch_data_front.report[i].y;
			}

			sceKernelSetEventFlag(*message->ev_flag, TOUCH_CHANGE);
			sceKernelSendMsgPipe(*message->msg_pipe, &packet, sizeof(TouchPacket), 0, NULL, NULL);
			old_touch_data_front = touch_data_front;
			sceKernelReceiveMsgPipe(*message->msg_pipe, NULL, 0, 0, NULL, NULL);
		}

		if (touch_config & BACK)
		{
			sceTouchPeek(SCE_TOUCH_PORT_BACK, &touch_data_back, 1);
			TouchPacket packet;
			packet.port = BACK;
			packet.num_rep = touch_data_back.reportNum;
			for (uint8_t i = 0; i < packet.num_rep; i++)
			{
				packet.reports[i].pressure = touch_data_back.report[i].force,
				packet.reports[i].id = touch_data_back.report[i].id,
				packet.reports[i].x = touch_data_back.report[i].x,
				packet.reports[i].y = touch_data_back.report[i].y;
			}

			sceKernelSetEventFlag(*message->ev_flag, TOUCH_CHANGE);
			sceKernelSendMsgPipe(*message->msg_pipe, &packet, sizeof(TouchPacket), 0, NULL, NULL);
			old_touch_data_back = touch_data_back;
			sceKernelReceiveMsgPipe(*message->msg_pipe, NULL, 0, 0, NULL, NULL);
		}
	}
	return 0;
}

static int main_thread(unsigned int args, void *argp)
{
	MainThreadMessage *message = (MainThreadMessage *)argp;
	debugNetPrintf(DEBUG, "Main thread");
	SceUInt event_timeout = 250;
	bool motion_activate = true;

	// Create an event flag to signal changes and a pipe for each thread to send infos
	SceUID ev_flag = sceKernelCreateEventFlag("EV_FLAG_CTRL", SCE_EVENT_WAITMULTIPLE, 0, 0);
	SceUID pipe_pad = sceKernelCreateMsgPipe("MAIN_PAD", 0x40, 12, 0x1000, NULL);
	SceUID pipe_touch = sceKernelCreateMsgPipe("MAIN_TOUCH", 0x40, 12, 0x1000, NULL);
	SceUID pipe_motion = sceKernelCreateMsgPipe("MAIN_MOTION", 0x40, 12, 0x1000, NULL);

	// Send a structure with all info to the created thread
	ThreadMessage pad_message = {
		.msg_pipe = &pipe_pad,
		.ev_flag = &ev_flag};
	SceUID pad_thread = sceKernelCreateThread("PadThread", &control_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(pad_thread, sizeof(ThreadMessage), &pad_message);

	ThreadMessage touch_message = {
		.msg_pipe = &pipe_touch,
		.ev_flag = &ev_flag};
	SceUID touch_thread_id = sceKernelCreateThread("TouchThread", &touch_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(touch_thread_id, sizeof(ThreadMessage), &touch_message);

	ThreadMessage motion_message = {
		.msg_pipe = &pipe_motion,
		.ev_flag = &ev_flag};
	SceUID motion_thread_id = sceKernelCreateThread("MotionThread", &motion_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(motion_thread_id, sizeof(ThreadMessage), &motion_message);

	SceUID epoll = sceNetEpollCreate("SERVER", 0);
	int fd = sceNetSocket("NET_SOCKET", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
	SceNetSockaddrIn serveraddr;
	serveraddr.sin_family = SCE_NET_AF_INET;
	serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
	serveraddr.sin_port = sceNetHtons(NET_PORT);
	sceNetBind(fd, (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));
	sceNetListen(fd, 128);

	SceNetEpollEvent ev = {0};
	ev.events = SCE_NET_EPOLLOUT | SCE_NET_EPOLLHUP | SCE_NET_EPOLLIN;
	ev.data.fd = fd;

	SceNetSockaddrIn clientaddr;
	unsigned int addrlen = sizeof(clientaddr);
	while (true)
	{
		int client = sceNetAccept(fd, (SceNetSockaddr *)&clientaddr, &addrlen);
		if (client >= 0)
		{
			sceKernelSetEventFlag(*message->ev_flag_connect_state, STATE_CONNECT);
			sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_ADD, fd, &ev);
			debugNetPrintf(DEBUG, "epoll");

			while (sceNetEpollWait(epoll, &ev, 3, 500) >= 0)
			{

				if (ev.events & SCE_NET_EPOLLOUT)
				{
					unsigned int pattern = 0;
					sceKernelWaitEventFlag(ev_flag,
										   PAD_CHANGE | TOUCH_CHANGE | MOTION_CHANGE,
										   SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR_PAT,
										   &pattern,
										   &event_timeout);

					debugNetPrintf(DEBUG, "Actual pattern %d\n", pattern);
					if (pattern & PAD_CHANGE)
					{
						debugNetPrintf(INFO, "Pad change");
						PadPacket pad;
						sceKernelReceiveMsgPipe(pipe_pad, &pad, sizeof(PadPacket), 0, NULL, NULL);
						Packet pkg = {
							.type = PAD,
							.packet.pad = pad};
						sceNetSend(client, &pkg, sizeof(Packet), 0);
						sceKernelSendMsgPipe(pipe_pad, NULL, 0, 0, NULL, NULL);
					}

					if (pattern & TOUCH_CHANGE)
					{
						debugNetPrintf(INFO, "Touch change");
						TouchPacket touch;
						sceKernelReceiveMsgPipe(pipe_touch, &touch, sizeof(TouchPacket), 0, NULL, NULL);
						Packet pkg = {
							.type = TOUCH,
							.packet.touch = touch};
						sceNetSend(client, &pkg, sizeof(Packet), 0);
						sceKernelSendMsgPipe(pipe_touch, NULL, 0, 0, NULL, NULL);
					}

					if (pattern & MOTION_CHANGE)
					{
						debugNetPrintf(INFO, "Motion change");
						MotionPacket motion;
						sceKernelReceiveMsgPipe(pipe_motion, &motion, sizeof(MotionPacket), 0, NULL, NULL);
						Packet pkg = {
							.type = MOTION,
							.packet.motion = motion};
						sceNetSend(client, &pkg, sizeof(Packet), 0);
						sceKernelSendMsgPipe(pipe_motion, NULL, 0, 0, NULL, NULL);
					}
				}

				if (ev.events & SCE_NET_EPOLLIN)
				{
					ConfigPacket cfg = {0};
					sceNetRecv(client, (char *)&cfg, sizeof(ConfigPacket), 0);
					if (touch_config != cfg.touch_config)
					{
						touch_config = cfg.touch_config;
						if (touch_config)
						{
							touch_thread_id = sceKernelCreateThread("TouchThread", &touch_thread, 0x10000100, 0x10000, 0, 0, NULL);
							sceKernelStartThread(touch_thread_id, sizeof(ThreadMessage), &touch_message);
						}
						else
						{
							sceKernelDeleteThread(touch_thread_id);
						}
					}
					if (motion_activate != cfg.motion_activate)
					{
						motion_activate = cfg.motion_activate;
						if (motion_activate)
						{
							motion_thread_id = sceKernelCreateThread("MotionThread", &motion_thread, 0x10000100, 0x10000, 0, 0, NULL);
							sceKernelStartThread(motion_thread_id, sizeof(ThreadMessage), &motion_message);
						}
						else
						{
							sceKernelDeleteThread(motion_thread_id);
						}
					}
				}

				if (ev.events & SCE_NET_EPOLLHUP)
				{
					sceKernelSetEventFlag(*message->ev_flag_connect_state, STATE_DISCONNECT);
					break;
				}
			}
		}
	}

	return 0;
}

vita2d_pgf *debug_font;
uint32_t text_color;

int main()
{

	// Enabling analog, motion and touch support
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	sceMotionStartSampling();
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
	sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);

	// Initializing graphics stuffs
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
	debug_font = vita2d_load_default_pgf();
	uint32_t text_color = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);

	// Initializing network stuffs
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	char vita_ip[32];
	int ret = sceNetShowNetstat();
	if (ret == SCE_NET_ERROR_ENOTINIT)
	{
		SceNetInitParam initparam;
		initparam.memory = malloc(NET_INIT_SIZE);
		initparam.size = NET_INIT_SIZE;
		initparam.flags = 0;
		ret = sceNetInit(&initparam);
	}
	ret = sceNetCtlInit();
	SceNetCtlInfo info;
	ret = sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
	sprintf(vita_ip, "%s", info.ip_address);
	SceNetInAddr vita_addr;
	sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &vita_addr);

	//DEBUG
	ret = debugNetInit("192.168.1.20", 8174, DEBUG);
	debugNetPrintf(DEBUG, "Test debug level %d\n", ret);

	SceUID ev_connect = sceKernelCreateEventFlag("ev_con", 0, 0, 0);
	MainThreadMessage main_message = {
		.ev_flag_connect_state = &ev_connect,
	};
	debugNetPrintf(DEBUG, "Test debug level %d\n", ev_connect);

	// Open the main thread with an event flag in argument to write the connection state
	SceUID main_thread_id = sceKernelCreateThread("MainThread", &main_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(main_thread_id, sizeof(MainThreadMessage), &main_message);
	SceUInt wait_timeout = 60 * 10000;

	unsigned int state = 0;
	do
	{
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_pgf_draw_text(debug_font, 2, 20, text_color, 1.0, "VitaPad fork v.1.1 by Rinnegatamante");
		vita2d_pgf_draw_textf(debug_font, 2, 60, text_color, 1.0, "Listening on:\nIP: %s\nPort: %d", vita_ip, NET_PORT);
		vita2d_pgf_draw_textf(debug_font, 2, 200, text_color, 1.0, "Status: %s", state & STATE_CONNECT ? "Connected!" : "Waiting connection...");
		vita2d_end_drawing();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers();
	} while (sceKernelWaitEventFlag(ev_connect, STATE_CONNECT | STATE_DISCONNECT, SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR, &state, &wait_timeout) < 0);
	debugNetFinish();
}