#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#ifdef __WIN32__
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
#endif
#include <windows.h>

#define u32 uint32_t
#define GAMEPAD_PORT 5000
typedef struct{
	u32 sock;
	struct sockaddr_in addrTo;
} Socket;

enum {
	SCE_CTRL_SELECT     = 0x000001,	//!< Select button.
	SCE_CTRL_START      = 0x000008,	//!< Start button.
	SCE_CTRL_UP         = 0x000010,	//!< Up D-Pad button.
	SCE_CTRL_RIGHT      = 0x000020,	//!< Right D-Pad button.
	SCE_CTRL_DOWN       = 0x000040,	//!< Down D-Pad button.
	SCE_CTRL_LEFT       = 0x000080,	//!< Left D-Pad button.
	SCE_CTRL_LTRIGGER   = 0x000100,	//!< Left trigger.
	SCE_CTRL_RTRIGGER   = 0x000200,	//!< Right trigger.
	SCE_CTRL_TRIANGLE   = 0x001000,	//!< Triangle button.
	SCE_CTRL_CIRCLE     = 0x002000,	//!< Circle button.
	SCE_CTRL_CROSS      = 0x004000,	//!< Cross button.
	SCE_CTRL_SQUARE     = 0x008000,	//!< Square button.
	SCE_CTRL_ANY        = 0x010000	//!< Any input intercepted.
};


typedef struct{
	uint32_t buttons;
	uint8_t lx;
	uint8_t ly;
	uint8_t rx;
	uint8_t ry;
} PadPacket;

void SendButtonPress(int btn){
	INPUT ip = { 0 };
	ip.type = INPUT_KEYBOARD;
	ip.ki.time = 0;
	ip.ki.wVk = 0;
	ip.ki.dwExtraInfo = 0;
	ip.ki.wScan = btn;
	ip.ki.dwFlags = KEYEVENTF_SCANCODE;
	SendInput(1, &ip, sizeof(INPUT));
}

void SendButtonRelease(int btn){
	INPUT ip = { 0 };
	ip.type = INPUT_KEYBOARD;
	ip.ki.time = 0;
	ip.ki.wVk = 0;
	ip.ki.dwExtraInfo = 0;
	ip.ki.wScan = btn;
	ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
}

int main(int argc,char** argv){

	#ifdef __WIN32__
	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(versionWanted, &wsaData);
	#endif

	printf("VitaPad Client by Rinnegatamante\n\n");
	char host[32];	
	if (argc > 1){
		char* ip = (char*)(argv[1]);
		sprintf(host,"%s",ip);		
	}else{		
		printf("Insert Vita IP: ");
		scanf("%s",&host);
	}

	// Writing info on the screen
	printf("IP: %s\nPort: %d\n\n",host, GAMEPAD_PORT);
	
	// Creating client socket
	Socket* my_socket = (Socket*) malloc(sizeof(Socket));
	memset(&my_socket->addrTo, '0', sizeof(my_socket->addrTo)); 
	my_socket->addrTo.sin_family = AF_INET;
	my_socket->addrTo.sin_port = htons(5000);
	my_socket->addrTo.sin_addr.s_addr = inet_addr(host);
	my_socket->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (my_socket->sock < 0){
		printf("\nFailed creating socket.");	
		return -1;
	}else printf("\nClient socket created on port 5000");
	
	// Connecting to VitaPad
	int err = connect(my_socket->sock, (struct sockaddr*)&my_socket->addrTo, sizeof(my_socket->addrTo));
	if (err < 0 ){ 
		printf("\nFailed connecting server.");
		close(my_socket->sock);
		return -1;
	}else printf("\nConnection estabilished!");
	fflush(stdout);
	
	// Preparing input packets
	uint8_t firstScan = 1;
	PadPacket data;
	PadPacket olddata;
	
	for (;;){
		
		send(my_socket->sock, "request", 8, 0);
		int count = recv(my_socket->sock, &data, 256, 0);
		if (firstScan){
			firstScan = 0;
			memcpy(&olddata,&data,8);
		}
		
		// If you want to change mapping, look here: http://www.philipstorr.id.au/pcbook/book3/scancode.htm
		if (count != 0){
			
			// S (Down)
			if ((data.buttons & SCE_CTRL_DOWN) && (!(olddata.buttons & SCE_CTRL_DOWN))) SendButtonPress(0x1F);
			else if ((olddata.buttons & SCE_CTRL_DOWN) && (!(data.buttons & SCE_CTRL_DOWN))) SendButtonRelease(0x1F);
			
			// W (Up)
			if ((data.buttons & SCE_CTRL_UP) && (!(olddata.buttons & SCE_CTRL_UP))) SendButtonPress(0x11);
			else if ((olddata.buttons & SCE_CTRL_UP) && (!(data.buttons & SCE_CTRL_UP))) SendButtonRelease(0x11);
			
			// A (Left)
			if ((data.buttons & SCE_CTRL_LEFT) && (!(olddata.buttons & SCE_CTRL_LEFT))) SendButtonPress(0x1E);
			else if ((olddata.buttons & SCE_CTRL_LEFT) && (!(data.buttons & SCE_CTRL_LEFT))) SendButtonRelease(0x1E);
			
			// D (Right)
			if ((data.buttons & SCE_CTRL_RIGHT) && (!(olddata.buttons & SCE_CTRL_RIGHT))) SendButtonPress(0x20);
			else if ((olddata.buttons & SCE_CTRL_RIGHT) && (!(data.buttons & SCE_CTRL_RIGHT))) SendButtonRelease(0x20);
			
			// I (Triangle)
			if ((data.buttons & SCE_CTRL_TRIANGLE) && (!(olddata.buttons & SCE_CTRL_TRIANGLE))) SendButtonPress(0x17);
			else if ((olddata.buttons & SCE_CTRL_TRIANGLE) && (!(data.buttons & SCE_CTRL_TRIANGLE))) SendButtonRelease(0x17);
			
			// J (Square)
			if ((data.buttons & SCE_CTRL_SQUARE) && (!(olddata.buttons & SCE_CTRL_SQUARE))) SendButtonPress(0x24);
			else if ((olddata.buttons & SCE_CTRL_SQUARE) && (!(data.buttons & SCE_CTRL_SQUARE))) SendButtonRelease(0x24);
			
			// K (Cross)
			if ((data.buttons & SCE_CTRL_CROSS) && (!(olddata.buttons & SCE_CTRL_CROSS))) SendButtonPress(0x25);
			else if ((olddata.buttons & SCE_CTRL_CROSS) && (!(data.buttons & SCE_CTRL_CROSS))) SendButtonRelease(0x25);
			
			// L (Circle)
			if ((data.buttons & SCE_CTRL_CIRCLE) && (!(olddata.buttons & SCE_CTRL_CIRCLE))) SendButtonPress(0x26);
			else if ((olddata.buttons & SCE_CTRL_CIRCLE) && (!(data.buttons & SCE_CTRL_CIRCLE))) SendButtonRelease(0x26);
			
			// Control (L Trigger)
			if ((data.buttons & SCE_CTRL_LTRIGGER) && (!(olddata.buttons & SCE_CTRL_LTRIGGER))) SendButtonPress(0x1D);
			else if ((olddata.buttons & SCE_CTRL_LTRIGGER) && (!(data.buttons & SCE_CTRL_LTRIGGER))) SendButtonRelease(0x1D);
			
			// Space (R Trigger)
			if ((data.buttons & SCE_CTRL_RTRIGGER) && (!(olddata.buttons & SCE_CTRL_RTRIGGER))) SendButtonPress(0x39);
			else if ((olddata.buttons & SCE_CTRL_RTRIGGER) && (!(data.buttons & SCE_CTRL_RTRIGGER))) SendButtonRelease(0x39);
			
			// Enter (Start)
			if ((data.buttons & SCE_CTRL_START) && (!(olddata.buttons & SCE_CTRL_START))) SendButtonPress(0x1C);
			else if ((olddata.buttons & SCE_CTRL_START) && (!(data.buttons & SCE_CTRL_START))) SendButtonRelease(0x1C);
			
			// Shift (Select)
			if ((data.buttons & SCE_CTRL_SELECT) && (!(olddata.buttons & SCE_CTRL_SELECT))) SendButtonPress(0x2A);
			else if ((olddata.buttons & SCE_CTRL_SELECT) && (!(data.buttons & SCE_CTRL_SELECT))) SendButtonRelease(0x2A);
			
			// Up/Down/Left/Right Arrows (Left Analog)
			if ((data.ly < 50) && (!(olddata.ly < 50))) SendButtonPress(0x48);
			else if ((olddata.ly < 50) && (!(data.ly < 50))) SendButtonRelease(0x48);
			if ((data.lx < 50) && (!(olddata.lx < 50))) SendButtonPress(0x4B);
			else if ((olddata.lx < 50) && (!(data.lx < 50))) SendButtonRelease(0x4B);
			if ((data.lx > 170) && (!(olddata.lx > 170))) SendButtonPress(0x4D);
			else if ((olddata.lx > 170) && (!(data.lx > 170))) SendButtonRelease(0x4D);
			if ((data.ly > 170) && (!(olddata.ly > 170))) SendButtonPress(0x50);
			else if ((olddata.ly > 170) && (!(data.ly > 170))) SendButtonRelease(0x50);
			
			// 8/2/4/6 Arrows (Right Analog)
			if ((data.ry < 50) && (!(olddata.ry < 50))) SendButtonPress(0x09);
			else if ((olddata.ry < 50) && (!(data.ry < 50))) SendButtonRelease(0x09);
			if ((data.rx < 50) && (!(olddata.rx < 50))) SendButtonPress(0x05);
			else if ((olddata.rx < 50) && (!(data.rx < 50))) SendButtonRelease(0x05);
			if ((data.rx > 170) && (!(olddata.rx > 170))) SendButtonPress(0x07);
			else if ((olddata.rx > 170) && (!(data.rx > 170))) SendButtonRelease(0x07);
			if ((data.ry > 170) && (!(olddata.ry > 170))) SendButtonPress(0x03);
			else if ((olddata.ry > 170) && (!(data.ry > 170))) SendButtonRelease(0x03);
					
			// Saving old pad status
			memcpy(&olddata,&data,8);
			
		}
	}
	
	return 1;
	
}