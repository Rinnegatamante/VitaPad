#include <vita2d.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <psp2/ctrl.h>
#include <psp2/types.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#define GAMEPAD_PORT 5000
#define NET_INIT_SIZE 1*1024*1024

// Server thread
volatile int connected = 0;
static int server_thread(unsigned int args, void* argp){
	
	// Initializing a socket
	int fd = sceNetSocket("VitaPad", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
	SceNetSockaddrIn serveraddr;
	serveraddr.sin_family = SCE_NET_AF_INET;
	serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
	serveraddr.sin_port = sceNetHtons(GAMEPAD_PORT);
	sceNetBind(fd, (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));
	sceNetListen(fd, 128);
	
	for (;;){
		SceNetSockaddrIn clientaddr;
		unsigned int addrlen = sizeof(clientaddr);
		int client = sceNetAccept(fd, (SceNetSockaddr *)&clientaddr, &addrlen);
		if (client >= 0) {
			connected = 1;
			char unused[8];
			for (;;){
				sceNetRecv(client,unused,256,0);
				SceCtrlData pad;
				sceCtrlPeekBufferPositive(0, &pad, 1);
				sceNetSend(client, &pad.buttons, 8, 0); // Sending buttons + analogs state
			}
		}
	}
	
	return 0;
}

vita2d_pgf* debug_font;
uint32_t text_color;

int main(){
	
	// Enabling analog support
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	
	// Initializing graphics stuffs
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
	debug_font = vita2d_load_default_pgf();
	uint32_t text_color = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
	
	// Initializing network stuffs
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	char vita_ip[32];
	int ret = sceNetShowNetstat();
	if (ret == SCE_NET_ERROR_ENOTINIT) {
		SceNetInitParam initparam;
		initparam.memory = malloc(NET_INIT_SIZE);
		initparam.size = NET_INIT_SIZE;
		initparam.flags = 0;
		ret=sceNetInit(&initparam);
	}
	ret = sceNetCtlInit();
	SceNetCtlInfo info;
	ret=sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
	sprintf(vita_ip,"%s",info.ip_address);
	SceNetInAddr vita_addr;
	sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &vita_addr);
	
	// Starting server thread
	SceUID thread = sceKernelCreateThread("VitaPad Thread",&server_thread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(thread, 0, NULL);
	
	for (;;){
		
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_pgf_draw_text(debug_font, 2, 20, text_color, 1.0, "VitaPad v.1.0 by Rinnegatamante");
		vita2d_pgf_draw_textf(debug_font, 2, 60, text_color, 1.0, "Listening on:\nIP: %s\nPort: %d",vita_ip,GAMEPAD_PORT);
		vita2d_pgf_draw_textf(debug_font, 2, 200, text_color, 1.0, "Status: %s",connected ? "Connected!" : "Waiting connection...");
		vita2d_end_drawing();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers();
		
	}
	
}