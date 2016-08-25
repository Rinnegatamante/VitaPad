#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

// PSVITA Related stuffs
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1088

// Sockets
#ifdef __WIN32__
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
#endif

// Input
#if defined(__WIN32__) || defined(__CYGWIN__)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#endif

// Keys
#include "tinyxml2.h"

uint16_t KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_TRIANGLE, KEY_SQUARE, KEY_CROSS, KEY_CIRCLE;
uint16_t KEY_L, KEY_R, KEY_START, KEY_SELECT, KEY_LANALOG_UP, KEY_LANALOG_DOWN, KEY_LANALOG_LEFT;
uint16_t KEY_LANALOG_RIGHT, KEY_RANALOG_UP, KEY_RANALOG_DOWN, KEY_RANALOG_LEFT, KEY_RANALOG_RIGHT;

void loadConfig(char* path)
{
	
	// Loading XML file
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(path) != tinyxml2::XML_NO_ERROR){
		printf("\nERROR: An error occurred while opening config file.");
		return;
	}
	
	// Getting elements
	tinyxml2::XMLElement* k1 = doc.FirstChildElement("KEY_DOWN");
	char* tmp = (char*)k1->GetText();
	KEY_DOWN = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_UP");
	tmp = (char*)k1->GetText();
	KEY_UP = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_LEFT");
	tmp = (char*)k1->GetText();
	KEY_LEFT = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_RIGHT");
	tmp = (char*)k1->GetText();
	KEY_RIGHT = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_TRIANGLE");
	tmp = (char*)k1->GetText();
	KEY_TRIANGLE = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_SQUARE");
	tmp = (char*)k1->GetText();
	KEY_SQUARE = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_CROSS");
	tmp = (char*)k1->GetText();
	KEY_CROSS = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_CIRCLE");
	tmp = (char*)k1->GetText();
	KEY_CIRCLE = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_L");
	tmp = (char*)k1->GetText();
	KEY_L = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_R");
	tmp = (char*)k1->GetText();
	KEY_R = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_START");
	tmp = (char*)k1->GetText();
	KEY_START = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_SELECT");
	tmp = (char*)k1->GetText();
	KEY_SELECT = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_LANALOG_UP");
	tmp = (char*)k1->GetText();
	KEY_LANALOG_UP = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_LANALOG_DOWN");
	tmp = (char*)k1->GetText();
	KEY_LANALOG_DOWN = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_LANALOG_LEFT");
	tmp = (char*)k1->GetText();
	KEY_LANALOG_LEFT = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_LANALOG_RIGHT");
	tmp = (char*)k1->GetText();
	KEY_LANALOG_RIGHT = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_RANALOG_UP");
	tmp = (char*)k1->GetText();
	KEY_RANALOG_UP = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_RANALOG_DOWN");
	tmp = (char*)k1->GetText();
	KEY_RANALOG_DOWN = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_RANALOG_LEFT");
	tmp = (char*)k1->GetText();
	KEY_RANALOG_LEFT = strtoul(tmp, NULL, 16);
	k1 = doc.FirstChildElement("KEY_RANALOG_RIGHT");
	tmp = (char*)k1->GetText();
	KEY_RANALOG_RIGHT = strtoul(tmp, NULL, 16);
	
}

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
	SCE_CTRL_SQUARE     = 0x008000	//!< Square button.
};
#define NO_INPUT 0
#define MOUSE_MOV 0x01
#define LEFT_CLICK 0x08
#define RIGHT_CLICK 0x10

typedef struct{
	uint32_t buttons;
	uint8_t lx;
	uint8_t ly;
	uint8_t rx;
	uint8_t ry;
	uint16_t tx;
	uint16_t ty;
	uint8_t click;
} PadPacket;

void SendMoveMouse(int x, int y){
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	float x_molt = (1.0 * x)/SCREEN_WIDTH;
	float y_molt = (1.0 * y)/SCREEN_HEIGHT;
	ip.mi.dx = (x_molt*65535);
	ip.mi.dy = (y_molt*65535);
	ip.mi.mouseData = 0;
	ip.mi.time = 0;
	ip.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	SendInput(1, &ip, sizeof(INPUT));
}

void SendMouseEvent(uint16_t event){
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	ip.mi.dx = 0;
	ip.mi.dy = 0;
	ip.mi.mouseData = 0;
	ip.mi.time = 0;
	ip.mi.dwFlags = event;
	SendInput(1, &ip, sizeof(INPUT));
}

#if defined(__WIN32__) || defined(__CYGWIN__)
void SendButtonPress(int btn){
	INPUT ip = { 0 };
	ip.type = INPUT_KEYBOARD;
	ip.ki.time = 0;
	ip.ki.wVk = 0;
	ip.ki.dwExtraInfo = 0;
	ip.ki.wScan = btn;
	ip.ki.dwFlags = KEYEVENTF_SCANCODE;
	SendInput(1, &ip, sizeof(INPUT));
#elif __linux__
void SendButtonPress (Display* display, int btn) {
    // Find the window which has the current keyboard focus.
    Window win;
    int    revert;
    XGetInputFocus(display, &win, &revert);
    Window winRoot = XDefaultRootWindow(display);

    // Send event
    XKeyEvent event;

    event.display     = display;
    event.window      = win;
    event.root        = winRoot;
    event.subwindow   = None;
    event.time        = CurrentTime;
    event.x           = 1;
    event.y           = 1;
    event.x_root      = 1;
    event.y_root      = 1;
    event.same_screen = True;
    event.keycode     = XKeysymToKeycode(display, btn);
    event.state       = 0; //modifiers;
    event.type        = KeyPress;
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
#endif
}

#if defined(__WIN32__) || defined(__CYGWIN__)
#define SEND_BUTTON_PRESS(x) SendButtonPress(x)
#elif __linux__
#define SEND_BUTTON_PRESS(...) SendButtonPress(display, __VA_ARGS__)
#endif

#if defined(__WIN32__) || defined(__CYGWIN__)
void SendButtonRelease(int btn){
	INPUT ip = { 0 };
	ip.type = INPUT_KEYBOARD;
	ip.ki.time = 0;
	ip.ki.wVk = 0;
	ip.ki.dwExtraInfo = 0;
	ip.ki.wScan = btn;
	ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
#elif __linux__
void SendButtonRelease(Display* display, int btn){
    // Find the window which has the current keyboard focus.
    Window win;
    int    revert;
    XGetInputFocus(display, &win, &revert);
    Window winRoot = XDefaultRootWindow(display);

    // Send event
    XKeyEvent event;

    event.display     = display;
    event.window      = win;
    event.root        = winRoot;
    event.subwindow   = None;
    event.time        = CurrentTime;
    event.x           = 1;
    event.y           = 1;
    event.x_root      = 1;
    event.y_root      = 1;
    event.same_screen = True;
    event.keycode     = XKeysymToKeycode(display, btn);
    event.state       = 0; //modifiers;
    event.type         = KeyRelease;
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
#endif
}

#if defined(__WIN32__) || defined(__CYGWIN__)
#define SEND_BUTTON_RELEASE(x) SendButtonRelease(x)
#elif __linux__
#define SEND_BUTTON_RELEASE(...) SendButtonRelease(display, __VA_ARGS__)
#endif

time_t getLastModifiedTime(char *path) {
	#ifdef __MINGW32__
	struct __stat64 attr;
	__stat64(path, &attr);
	return attr.st_mtime;
	#else
	struct stat attr;
	stat(path, &attr);
	return attr.st_mtime;
	#endif
}


int main(int argc,char** argv){
	#ifdef __WIN32__
	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(versionWanted, &wsaData);
	#endif

    #ifdef __linux__
    Display* display = XOpenDisplay(0);
    if (display == NULL)
        exit(1);
    #endif
	
	// Loading mapping
	time_t life_tick = 0;
	#ifdef __linux__
		loadConfig("linux.xml");
		life_tick = getLastModifiedTime("linux.xml");
	#else
		loadConfig("windows.xml");
		life_tick = getLastModifiedTime("windows.xml");
	#endif
	
	printf("VitaPad Client by Rinnegatamante\n\n");
	char host[32];
	if (argc > 1){
		char* ip = (char*)(argv[1]);
		sprintf(host,"%s",ip);
	}else{
		printf("Insert Vita IP: ");
		scanf("%s",host);
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
	}else printf("\nConnection established!");
	fflush(stdout);
	
	// Preparing input packets
	uint8_t firstScan = 1;
	PadPacket data;
	PadPacket olddata;

	for (;;){
	
		// Checking if we need a mapping reload
		time_t re_tick;
		#ifdef __linux__
		if ((re_tick = getLastModifiedTime("linux.xml")) != life_tick){
			loadConfig("linux.xml");
			life_tick = re_tick;
			printf("\nConfig file reloaded since a modification has been detected.");
		}
		#else			
		if ((re_tick = getLastModifiedTime("windows.xml")) != life_tick){
			loadConfig("windows.xml");
			life_tick = re_tick;
			printf("\nConfig file reloaded since a modification has been detected.");
		}
		#endif
		
		send(my_socket->sock, "request", 8, 0);
		int count = recv(my_socket->sock, (char*)&data, 256, 0);
		if (firstScan){
			firstScan = 0;
			memcpy(&olddata,&data,sizeof(PadPacket));
		}

		if (count != 0){
			// S (Down)
			if ((data.buttons & SCE_CTRL_DOWN) && (!(olddata.buttons & SCE_CTRL_DOWN))) SEND_BUTTON_PRESS(KEY_DOWN);
			else if ((olddata.buttons & SCE_CTRL_DOWN) && (!(data.buttons & SCE_CTRL_DOWN))) SEND_BUTTON_RELEASE(KEY_DOWN);

			// W (Up)
			if ((data.buttons & SCE_CTRL_UP) && (!(olddata.buttons & SCE_CTRL_UP))) SEND_BUTTON_PRESS(KEY_UP);
			else if ((olddata.buttons & SCE_CTRL_UP) && (!(data.buttons & SCE_CTRL_UP))) SEND_BUTTON_RELEASE(KEY_UP);

			// A (Left)
			if ((data.buttons & SCE_CTRL_LEFT) && (!(olddata.buttons & SCE_CTRL_LEFT))) SEND_BUTTON_PRESS(KEY_LEFT);
			else if ((olddata.buttons & SCE_CTRL_LEFT) && (!(data.buttons & SCE_CTRL_LEFT))) SEND_BUTTON_RELEASE(KEY_LEFT);

			// D (Right)
			if ((data.buttons & SCE_CTRL_RIGHT) && (!(olddata.buttons & SCE_CTRL_RIGHT))) SEND_BUTTON_PRESS(KEY_RIGHT);
			else if ((olddata.buttons & SCE_CTRL_RIGHT) && (!(data.buttons & SCE_CTRL_RIGHT))) SEND_BUTTON_RELEASE(KEY_RIGHT);

			// I (Triangle)
			if ((data.buttons & SCE_CTRL_TRIANGLE) && (!(olddata.buttons & SCE_CTRL_TRIANGLE))) SEND_BUTTON_PRESS(KEY_TRIANGLE);
			else if ((olddata.buttons & SCE_CTRL_TRIANGLE) && (!(data.buttons & SCE_CTRL_TRIANGLE))) SEND_BUTTON_RELEASE(KEY_TRIANGLE);

			// J (Square)
			if ((data.buttons & SCE_CTRL_SQUARE) && (!(olddata.buttons & SCE_CTRL_SQUARE))) SEND_BUTTON_PRESS(KEY_SQUARE);
			else if ((olddata.buttons & SCE_CTRL_SQUARE) && (!(data.buttons & SCE_CTRL_SQUARE))) SEND_BUTTON_RELEASE(KEY_SQUARE);

			// K (Cross)
			if ((data.buttons & SCE_CTRL_CROSS) && (!(olddata.buttons & SCE_CTRL_CROSS))) SEND_BUTTON_PRESS(KEY_CROSS);
			else if ((olddata.buttons & SCE_CTRL_CROSS) && (!(data.buttons & SCE_CTRL_CROSS))) SEND_BUTTON_RELEASE(KEY_CROSS);

			// L (Circle)
			if ((data.buttons & SCE_CTRL_CIRCLE) && (!(olddata.buttons & SCE_CTRL_CIRCLE))) SEND_BUTTON_PRESS(KEY_CIRCLE);
			else if ((olddata.buttons & SCE_CTRL_CIRCLE) && (!(data.buttons & SCE_CTRL_CIRCLE))) SEND_BUTTON_RELEASE(KEY_CIRCLE);

			// Control (L Trigger)
			if ((data.buttons & SCE_CTRL_LTRIGGER) && (!(olddata.buttons & SCE_CTRL_LTRIGGER))) SEND_BUTTON_PRESS(KEY_L);
			else if ((olddata.buttons & SCE_CTRL_LTRIGGER) && (!(data.buttons & SCE_CTRL_LTRIGGER))) SEND_BUTTON_RELEASE(KEY_L);

			// Space (R Trigger)
			if ((data.buttons & SCE_CTRL_RTRIGGER) && (!(olddata.buttons & SCE_CTRL_RTRIGGER))) SEND_BUTTON_PRESS(KEY_R);
			else if ((olddata.buttons & SCE_CTRL_RTRIGGER) && (!(data.buttons & SCE_CTRL_RTRIGGER))) SEND_BUTTON_RELEASE(KEY_R);

			// Enter (Start)
			if ((data.buttons & SCE_CTRL_START) && (!(olddata.buttons & SCE_CTRL_START))) SEND_BUTTON_PRESS(KEY_START);
			else if ((olddata.buttons & SCE_CTRL_START) && (!(data.buttons & SCE_CTRL_START))) SEND_BUTTON_RELEASE(KEY_START);

			// Shift (Select)
			if ((data.buttons & SCE_CTRL_SELECT) && (!(olddata.buttons & SCE_CTRL_SELECT))) SEND_BUTTON_PRESS(KEY_SELECT);
			else if ((olddata.buttons & SCE_CTRL_SELECT) && (!(data.buttons & SCE_CTRL_SELECT))) SEND_BUTTON_RELEASE(KEY_SELECT);

			// Up/Down/Left/Right Arrows (Left Analog)
			if ((data.ly < 50) && (!(olddata.ly < 50))) SEND_BUTTON_PRESS(KEY_LANALOG_UP);
			else if ((olddata.ly < 50) && (!(data.ly < 50))) SEND_BUTTON_RELEASE(KEY_LANALOG_UP);
			if ((data.lx < 50) && (!(olddata.lx < 50))) SEND_BUTTON_PRESS(KEY_LANALOG_LEFT);
			else if ((olddata.lx < 50) && (!(data.lx < 50))) SEND_BUTTON_RELEASE(KEY_LANALOG_LEFT);
			if ((data.lx > 170) && (!(olddata.lx > 170))) SEND_BUTTON_PRESS(KEY_LANALOG_RIGHT);
			else if ((olddata.lx > 170) && (!(data.lx > 170))) SEND_BUTTON_RELEASE(KEY_LANALOG_RIGHT);
			if ((data.ly > 170) && (!(olddata.ly > 170))) SEND_BUTTON_PRESS(KEY_LANALOG_DOWN);
			else if ((olddata.ly > 170) && (!(data.ly > 170))) SEND_BUTTON_RELEASE(KEY_LANALOG_DOWN);

			// 8/2/4/6 Arrows (Right Analog)
			if ((data.ry < 50) && (!(olddata.ry < 50))) SEND_BUTTON_PRESS(KEY_RANALOG_UP);
			else if ((olddata.ry < 50) && (!(data.ry < 50))) SEND_BUTTON_RELEASE(KEY_RANALOG_UP);
			if ((data.rx < 50) && (!(olddata.rx < 50))) SEND_BUTTON_PRESS(KEY_RANALOG_LEFT);
			else if ((olddata.rx < 50) && (!(data.rx < 50))) SEND_BUTTON_RELEASE(KEY_RANALOG_LEFT);
			if ((data.rx > 170) && (!(olddata.rx > 170))) SEND_BUTTON_PRESS(KEY_RANALOG_RIGHT);
			else if ((olddata.rx > 170) && (!(data.rx > 170))) SEND_BUTTON_RELEASE(KEY_RANALOG_RIGHT);
			if ((data.ry > 170) && (!(olddata.ry > 170))) SEND_BUTTON_PRESS(KEY_RANALOG_DOWN);
			else if ((olddata.ry > 170) && (!(data.ry > 170))) SEND_BUTTON_RELEASE(KEY_RANALOG_DOWN);
			
			// Mouse emulation with touchscreen + retrotouch
			if (data.click != NO_INPUT){			
				if (data.click & MOUSE_MOV) SendMoveMouse(data.tx, data.ty);
				if ((data.click & LEFT_CLICK) && (!(olddata.click & LEFT_CLICK))) SendMouseEvent(MOUSEEVENTF_LEFTDOWN);
				else if ((olddata.click & LEFT_CLICK) && (!(data.click & LEFT_CLICK))) SendMouseEvent(MOUSEEVENTF_LEFTUP);
				if ((data.click & RIGHT_CLICK) && (!(olddata.click & RIGHT_CLICK))) SendMouseEvent(MOUSEEVENTF_RIGHTDOWN);
				else if ((olddata.click & RIGHT_CLICK) && (!(data.click & RIGHT_CLICK))) SendMouseEvent(MOUSEEVENTF_RIGHTUP);
			}
			
			// Saving old pad status
			memcpy(&olddata,&data,sizeof(PadPacket));
		}
	}

    #ifdef __linux__
    XCloseDisplay(display);
    #endif

	return 1;
}
