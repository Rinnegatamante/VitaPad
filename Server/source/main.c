#include <vita2d.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/types.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <psp2/touch.h>
#include <psp2/kernel/threadmgr.h>

#include "../../common/types.h"

#include "ctrl.h"

#define NET_INIT_SIZE 1 * 1024 * 1024

static int control_thread(unsigned int args, void *argp)
{
	SceUID *pipe_id = (SceUID *)argp;

	SceCtrlData pad, old_pad;
	while (true)
	{
		sceCtrlPeekBufferPositive(0, &pad, 1);

		if (is_different(&pad, &old_pad))
		{
			Packet pkg;
			pkg.buttons = convert(pad);
			pkg.lx = pad.lx;
			pkg.ly = pad.ly;
			pkg.rx = pad.rx;
			pkg.ry = pad.ry;
			sceKernelSendMsgPipe(*pipe_id, &pkg, sizeof(Packet), 0, NULL, NULL);
			old_pad = pad;
			sceKernelReceiveMsgPipe(*pipe_id, NULL, 0, 0, NULL, NULL);
		}
	}
	return 0;
}

// Server thread
volatile int connected = 0;
static int server_thread(unsigned int args, void *argp)
{
	SceUID *pipe_id = (SceUID *)argp;

	SceUID epoll = sceNetEpollCreate("SERVER", 0);

	// Initializing a socket
	int fd = sceNetSocket("VitaPad", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
	SceNetSockaddrIn serveraddr;
	serveraddr.sin_family = SCE_NET_AF_INET;
	serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
	serveraddr.sin_port = sceNetHtons(GAMEPAD_PORT);
	sceNetBind(fd, (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));
	sceNetListen(fd, 128);

	SceNetEpollEvent ev = {0};
	ev.events = SCE_NET_EPOLLOUT | SCE_NET_EPOLLIN | SCE_NET_EPOLLHUP;
	ev.data.fd = fd;

	SceNetSockaddrIn clientaddr;
	unsigned int addrlen = sizeof(clientaddr);
	while (true)
	{
		int client = sceNetAccept(fd, (SceNetSockaddr *)&clientaddr, &addrlen);
		if (client >= 0)
		{
			connected = 1;
			sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_ADD, client, &ev);

			while (true)
			{
				SceNetEpollEvent ev = {0};
				sceNetEpollWait(epoll, &ev, 2, 100);
				if (ev.events & SCE_NET_EPOLLOUT)
				{
					Packet pkg;
					sceKernelReceiveMsgPipe(*pipe_id, (char *)&pkg, sizeof(Packet), 0, NULL, NULL);
					sceNetSend(client, &pkg, sizeof(Packet), 0);
					sceKernelSendMsgPipe(*pipe_id, NULL, 0, 0, NULL, NULL);
				}
				if (ev.events & SCE_NET_EPOLLIN)
				{
					if (sceNetRecv(client, NULL, 0, 0) == 0)
					{
						connected = 0;
						sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_DEL, client, &ev);
						break;
					}
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

	// Enabling analog and touch support
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);

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

	SceUID pipe = sceKernelCreateMsgPipe("SERV_CTRL", 0x40, 12, 0x1000, NULL);

	// Starting server thread
	SceUID serv_thread = sceKernelCreateThread("ServerThread", &server_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(serv_thread, sizeof(SceUID), &pipe);

	// Starting control thread
	SceUID ctrl_thread = sceKernelCreateThread("ControlThread", &control_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(ctrl_thread, sizeof(SceUID), &pipe);

	while (true)
	{
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_pgf_draw_text(debug_font, 2, 20, text_color, 1.0, "VitaPad v.1.1 by Rinnegatamante");
		vita2d_pgf_draw_textf(debug_font, 2, 60, text_color, 1.0, "Listening on:\nIP: %s\nPort: %d", vita_ip, GAMEPAD_PORT);
		vita2d_pgf_draw_textf(debug_font, 2, 200, text_color, 1.0, "Status: %s", connected ? "Connected!" : "Waiting connection...");
		vita2d_end_drawing();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers();
	}
}