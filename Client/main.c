#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

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
#ifdef __WIN32__
#  include <windows.h>
#else
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#endif

// Keys
#ifdef __WIN32__
// If you want to change mapping, look here: http://www.philipstorr.id.au/pcbook/book3/scancode.htm
#define KEY_DOWN 0x1F
#define KEY_UP 0x11
#define KEY_LEFT 0x1E
#define KEY_RIGHT 0x20

#define KEY_TRIANGLE 0x17
#define KEY_SQUARE 0x24
#define KEY_CROSS 0x25
#define KEY_CIRCLE 0x26

#define KEY_L_TRIGGER 0x1D
#define KEY_R_TRIGGER 0x39
#define KEY_START 0x1C
#define KEY_SELECT 0x2A

#define KEY_UP_1 0x48
#define KEY_LEFT_1 0x4B
#define KEY_RIGHT_1 0x4D
#define KEY_DOWN_1 0x50

#define KEY_UP_2 0x09
#define KEY_LEFT_2 0x05
#define KEY_RIGHT_2 0x07
#define KEY_DOWN_2 0x03
#elif __linux__
// If you want to change mapping, look at /usr/include/X11/keysymdef.h
#define KEY_DOWN XK_S
#define KEY_UP XK_W
#define KEY_LEFT XK_A
#define KEY_RIGHT XK_D

#define KEY_TRIANGLE XK_I
#define KEY_SQUARE XK_J
#define KEY_CROSS XK_K
#define KEY_CIRCLE XK_L

#define KEY_L_TRIGGER XK_Control_L
#define KEY_R_TRIGGER XK_space
#define KEY_START XK_Return
#define KEY_SELECT XK_Shift_L

#define KEY_UP_1 XK_Up
#define KEY_LEFT_1 XK_Left
#define KEY_RIGHT_1 XK_Right
#define KEY_DOWN_1 XK_Down

#define KEY_UP_2 XK_8
#define KEY_LEFT_2 XK_4
#define KEY_RIGHT_2 XK_6
#define KEY_DOWN_2 XK_2
#endif

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

#ifdef __WIN32__
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

#ifdef __WIN32__
#define SEND_BUTTON_PRESS(...) SendButtonPress(...)
#elif __linux__
#define SEND_BUTTON_PRESS(...) SendButtonPress(display, __VA_ARGS__)
#endif

#ifdef __WIN32__
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

#ifdef __WIN32__
#define SEND_BUTTON_RELEASE(...) SendButtonRelease(...)
#elif __linux__
#define SEND_BUTTON_RELEASE(...) SendButtonRelease(display, __VA_ARGS__)
#endif

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
	}else printf("\nConnection established!");
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
			if ((data.buttons & SCE_CTRL_LTRIGGER) && (!(olddata.buttons & SCE_CTRL_LTRIGGER))) SEND_BUTTON_PRESS(KEY_L_TRIGGER);
			else if ((olddata.buttons & SCE_CTRL_LTRIGGER) && (!(data.buttons & SCE_CTRL_LTRIGGER))) SEND_BUTTON_RELEASE(KEY_L_TRIGGER);

			// Space (R Trigger)
			if ((data.buttons & SCE_CTRL_RTRIGGER) && (!(olddata.buttons & SCE_CTRL_RTRIGGER))) SEND_BUTTON_PRESS(KEY_R_TRIGGER);
			else if ((olddata.buttons & SCE_CTRL_RTRIGGER) && (!(data.buttons & SCE_CTRL_RTRIGGER))) SEND_BUTTON_RELEASE(KEY_R_TRIGGER);

			// Enter (Start)
			if ((data.buttons & SCE_CTRL_START) && (!(olddata.buttons & SCE_CTRL_START))) SEND_BUTTON_PRESS(KEY_START);
			else if ((olddata.buttons & SCE_CTRL_START) && (!(data.buttons & SCE_CTRL_START))) SEND_BUTTON_RELEASE(KEY_START);

			// Shift (Select)
			if ((data.buttons & SCE_CTRL_SELECT) && (!(olddata.buttons & SCE_CTRL_SELECT))) SEND_BUTTON_PRESS(KEY_SELECT);
			else if ((olddata.buttons & SCE_CTRL_SELECT) && (!(data.buttons & SCE_CTRL_SELECT))) SEND_BUTTON_RELEASE(KEY_SELECT);

			// Up/Down/Left/Right Arrows (Left Analog)
			if ((data.ly < 50) && (!(olddata.ly < 50))) SEND_BUTTON_PRESS(KEY_UP_1);
			else if ((olddata.ly < 50) && (!(data.ly < 50))) SEND_BUTTON_RELEASE(KEY_UP_1);
			if ((data.lx < 50) && (!(olddata.lx < 50))) SEND_BUTTON_PRESS(KEY_LEFT_1);
			else if ((olddata.lx < 50) && (!(data.lx < 50))) SEND_BUTTON_RELEASE(KEY_LEFT_1);
			if ((data.lx > 170) && (!(olddata.lx > 170))) SEND_BUTTON_PRESS(KEY_RIGHT_1);
			else if ((olddata.lx > 170) && (!(data.lx > 170))) SEND_BUTTON_RELEASE(KEY_RIGHT_1);
			if ((data.ly > 170) && (!(olddata.ly > 170))) SEND_BUTTON_PRESS(KEY_DOWN_1);
			else if ((olddata.ly > 170) && (!(data.ly > 170))) SEND_BUTTON_RELEASE(KEY_DOWN_1);

			// 8/2/4/6 Arrows (Right Analog)
			if ((data.ry < 50) && (!(olddata.ry < 50))) SEND_BUTTON_PRESS(KEY_UP_2);
			else if ((olddata.ry < 50) && (!(data.ry < 50))) SEND_BUTTON_RELEASE(KEY_UP_2);
			if ((data.rx < 50) && (!(olddata.rx < 50))) SEND_BUTTON_PRESS(KEY_LEFT_2);
			else if ((olddata.rx < 50) && (!(data.rx < 50))) SEND_BUTTON_RELEASE(KEY_LEFT_2);
			if ((data.rx > 170) && (!(olddata.rx > 170))) SEND_BUTTON_PRESS(KEY_RIGHT_2);
			else if ((olddata.rx > 170) && (!(data.rx > 170))) SEND_BUTTON_RELEASE(KEY_RIGHT_2);
			if ((data.ry > 170) && (!(olddata.ry > 170))) SEND_BUTTON_PRESS(KEY_DOWN_2);
			else if ((olddata.ry > 170) && (!(data.ry > 170))) SEND_BUTTON_RELEASE(KEY_DOWN_2);

			// Saving old pad status
			memcpy(&olddata,&data,8);
		}
	}

    #ifdef __linux__
    XCloseDisplay(display);
    #endif

	return 1;
}
