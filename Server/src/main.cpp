#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/types.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <sys/select.h>
#include <psp2/touch.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/motion.h>

#include <climits>
#include <unordered_map>
#include <ctime>
#include <utility>

#include <debugnet.h>

#include <common.h>
#include "ctrl.hpp"

#define MAX_EPOLL_EVENTS 5
#define NET_INIT_SIZE 1 * 1024 * 1024
#define MAX_INTERVAL 2000

#define DEBUG_IP "192.168.1.43"
#ifndef DEBUG_IP
#define debugNetPrintf(...)
#endif

typedef struct _MainThreadMessage
{
	SceUID *ev_flag_connect_state;
} MainThreadMessage;

typedef struct _ControlThreadMessage
{
	SceUID *ev_flag_ctrl_out;
	SceUID *control_msg_pipe;
} ControlThreadMessage;

typedef enum _ConnectionState
{
	DISCONNECT = 1,
	CONNECT = 2,
} ConnectionState;

typedef enum _CtrlEvents
{
	DATA_OUT = 1,
} CtrlEvents;

void disconnect_client(int efd, int cfd)
{
	sceNetEpollControl(efd, SCE_NET_EPOLL_CTL_DEL, cfd, NULL);
	sceNetSocketClose(cfd);
}

int send_all(int fd, const void *buf, int size)
{
	const char *buf_ptr = (const char *)buf;
	int bytes_sent;

	while (size > 0)
	{
		bytes_sent = sceNetSend(fd, buf_ptr, size, 0);
		if (bytes_sent < 0)
			return bytes_sent;

		buf_ptr += bytes_sent;
		size -= bytes_sent;
	}

	return 1;
}

static int control_thread(uint args, void *argp)
{
	ControlThreadMessage *message = (ControlThreadMessage *)argp;
	SceCtrlData pad = {0};
	SceMotionSensorState motion_data = {0}; //TODO: Needs calibration
	SceTouchData touch_data_front, touch_data_back = {0};

	uint state = 0;
	SceUInt TIMEOUT = (SceUInt)UINT32_MAX;
	flatbuffers::FlatBufferBuilder builder(1024);
	while (sceKernelWaitEventFlag(*message->ev_flag_ctrl_out, DATA_OUT, SCE_EVENT_WAITCLEAR, &state, &TIMEOUT) == 0)
	{
		sceCtrlPeekBufferPositive(0, &pad, 1);
		auto buttons = convert_pad_data(pad);

		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch_data_front, 1);
		auto data_front = convert_touch_data(builder, touch_data_front, Pad::TouchPort::Front);

		sceTouchPeek(SCE_TOUCH_PORT_BACK, &touch_data_back, 1);
		auto data_back = convert_touch_data(builder, touch_data_back, Pad::TouchPort::Back);

		std::vector<flatbuffers::Offset<Pad::TouchData>> td;
		td.push_back(data_front);
		td.push_back(data_back);

		sceMotionGetSensorState(&motion_data, 1);
		Pad::MotionData motion;
		std::memcpy(&motion.mutable_accelerometer(), &motion_data.accelerometer, sizeof(Pad::Vector3));
		std::memcpy(&motion.mutable_gyro(), &motion_data.gyro, sizeof(Pad::Vector3));

		auto packet = Pad::CreateMainPacketDirect(builder, &buttons,
												  pad.lx, pad.ly,
												  pad.rx, pad.ry,
												  &td, &motion,
												  pad.timeStamp);
		builder.FinishSizePrefixed(packet);

		uint32_t size = builder.GetSize() + 4;
		uint8_t *buffer = new uint8_t[size];

		std::memcpy(buffer, builder.GetBufferPointer(), size);

		sceKernelSendMsgPipe(*message->control_msg_pipe, &size, sizeof(uint32_t), 0, NULL, NULL);
		sceKernelSendMsgPipe(*message->control_msg_pipe, buffer, size, 0, NULL, NULL);

		sceKernelReceiveMsgPipe(*message->control_msg_pipe, NULL, 0, 0, NULL, NULL);
		builder.Clear();
		delete[] buffer;
	}

	return 0;
}

static int main_thread(unsigned int args, void *argp)
{
	MainThreadMessage *message = (MainThreadMessage *)argp;

	// Create the thread which manages the control part
	SceUID flag_ctrl_out = sceKernelCreateEventFlag("flag_ctrl", 0, 0, NULL);
	SceUID ctrl_msg_pipe = sceKernelCreateMsgPipe("ctrl_msg_pipe", 0x40, 12, 0x1000, NULL);
	ControlThreadMessage ctrl_message = {
		.ev_flag_ctrl_out = &flag_ctrl_out,
		.control_msg_pipe = &ctrl_msg_pipe,
	};
	SceUID ctrl_thread_id = sceKernelCreateThread("CtrlThread", &control_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(ctrl_thread_id, sizeof(ControlThreadMessage), &ctrl_message);

	int server_fd = sceNetSocket("SERVER_SOCKET", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
	auto nbio = 1;
	sceNetSetsockopt(server_fd, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &nbio, sizeof(nbio));

	SceNetSockaddrIn serveraddr;
	serveraddr.sin_family = SCE_NET_AF_INET;
	serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
	serveraddr.sin_port = sceNetHtons(NET_PORT);
	sceNetBind(server_fd, (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));

	sceNetListen(server_fd, 10);

	SceUID epoll = sceNetEpollCreate("SERVER_EPOLL", 0);
	SceNetEpollEvent ev = {0};
	ev.events = SCE_NET_EPOLLIN;
	ev.data.fd = server_fd;
	sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_ADD, server_fd, &ev);

	SceNetEpollEvent *events = new SceNetEpollEvent[MAX_EPOLL_EVENTS];
	size_t n, i;
	uint8_t hangup[1];

	while ((n = sceNetEpollWait(epoll, events, MAX_EPOLL_EVENTS, MAX_INTERVAL)) >= 0)
	{
		for (i = 0; i < n; i++)
		{
			int fd = events[i].data.fd;

			if (fd == server_fd)
			{
				SceNetSockaddrIn clientaddr = {0};
				unsigned int addrlen = sizeof(clientaddr);
				int client_fd = sceNetAccept(server_fd, (SceNetSockaddr *)&clientaddr, &addrlen);
				if (client_fd >= 0)
				{
					SceNetEpollEvent ev = {0};
					ev.events = SCE_NET_EPOLLOUT | SCE_NET_EPOLLIN;
					ev.data.fd = client_fd;
					sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_ADD, client_fd, &ev);
					sceKernelSetEventFlag(*message->ev_flag_connect_state, CONNECT);
				}
			}

			else if (events[i].events & SCE_NET_EPOLLHUP || events[i].events & SCE_NET_EPOLLERR)
			{
				disconnect_client(epoll, fd);
				sceKernelSetEventFlag(*message->ev_flag_connect_state, DISCONNECT);
			}

			else if (events[i].events & SCE_NET_EPOLLIN)
			{
				int res = sceNetRecv(events[i].data.fd, hangup, 1, SCE_NET_MSG_PEEK | SCE_NET_MSG_DONTWAIT);
				if (res == 0)
				{
					disconnect_client(epoll, fd);
					sceKernelSetEventFlag(*message->ev_flag_connect_state, DISCONNECT);
				}

				//TODO: Handle config packets
			}

			else if (events[i].events & SCE_NET_EPOLLOUT)
			{
				sceKernelSetEventFlag(flag_ctrl_out, DATA_OUT);

				uint32_t buffer_size;
				sceKernelReceiveMsgPipe(ctrl_msg_pipe, &buffer_size, sizeof(uint32_t), 0, NULL, NULL);

				uint8_t *buffer = new uint8_t[buffer_size];
				sceKernelReceiveMsgPipe(ctrl_msg_pipe, buffer, buffer_size, 0, NULL, NULL);

				send_all(fd, buffer, buffer_size);

				delete[] buffer;
				sceKernelSendMsgPipe(ctrl_msg_pipe, 0, 0, 0, NULL, NULL);
			}
		}
	}

	sceKernelSetEventFlag(*message->ev_flag_connect_state, DISCONNECT);

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

#ifdef DEBUG_IP
	//DEBUG
	debugNetInit(DEBUG_IP, 8174, DEBUG);
#endif

	SceUID ev_connect = sceKernelCreateEventFlag("ev_con", 0, 0, NULL);
	MainThreadMessage main_message = {
		.ev_flag_connect_state = &ev_connect,
	};

	// Open the main thread with an event flag in argument to write the connection state
	SceUID main_thread_id = sceKernelCreateThread("MainThread", &main_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(main_thread_id, sizeof(MainThreadMessage), &main_message);

	unsigned int state = 0;
	SceUInt TIMEOUT = (SceUInt)UINT32_MAX;
	do
	{
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_pgf_draw_text(debug_font, 2, 20, text_color, 1.0, "VitaPad fork v1.02 by Musikid");
		vita2d_pgf_draw_textf(debug_font, 2, 60, text_color, 1.0, "Listening on:\nIP: %s\nPort: %d", vita_ip, NET_PORT);
		vita2d_pgf_draw_textf(debug_font, 2, 200, text_color, 1.0, "Status: %s", state & CONNECT ? "Connected" : "Not connected");
		vita2d_end_drawing();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers();
	} while (sceKernelWaitEventFlag(ev_connect, CONNECT | DISCONNECT, SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR, &state, &TIMEOUT) == 0);

	debugNetFinish();
}