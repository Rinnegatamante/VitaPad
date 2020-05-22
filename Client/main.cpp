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

#if !(defined(__WIN32__) || defined(__CYGWIN__) || defined(__linux__))
#  error "Your target system is not yet supported by VitaPad"
#endif

// Sockets
#ifdef __WIN32__
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
#endif

#include "tinyxml2.h"
#include "main.h"

// Input
#if defined(__WIN32__) || defined(__CYGWIN__)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  ifndef INPUT // Patch for old(?) MinGW installations
#    include <winable.h>
#    define KEYEVENTF_SCANCODE 0x0008
#  endif
#else
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/extensions/XTest.h>
#endif

// Keys
uint16_t KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_TRIANGLE, KEY_SQUARE, KEY_CROSS, KEY_CIRCLE;
uint16_t KEY_L, KEY_R, KEY_START, KEY_SELECT, KEY_LANALOG_UP, KEY_LANALOG_DOWN, KEY_LANALOG_LEFT;
uint16_t KEY_LANALOG_RIGHT, KEY_RANALOG_UP, KEY_RANALOG_DOWN, KEY_RANALOG_LEFT, KEY_RANALOG_RIGHT;

// Mouse
#if defined(__WIN32__) || defined(__CYGWIN__)
#define MOUSE_LEFT_DOWN MOUSEEVENTF_LEFTDOWN
#define MOUSE_LEFT_UP MOUSEEVENTF_LEFTUP
#define MOUSE_RIGHT_DOWN MOUSEEVENTF_RIGHTDOWN
#define MOUSE_RIGHT_UP MOUSEEVENTF_RIGHTUP
#elif __linux__
#define MOUSE_LEFT_DOWN 0
#define MOUSE_LEFT_UP 1
#define MOUSE_RIGHT_DOWN 2
#define MOUSE_RIGHT_UP 3
#endif


// VJoy
#ifdef __WIN32__
# include "VJoySDK/inc/public.h"
# include "VJoySDK/inc/vjoyinterface.h"
bool VJOY_MODE = false;
bool VJOY_ALTERNATE = false;
int VJOY_DEVID = 0;

#include "ViGEm.h"
unsigned int VIGEM_MODE = VIGEM_DEVICE_NONE;

enum {
	VJOY_CTRL_SELECT     = 1 << 6,	//!< Select button.
	VJOY_CTRL_START      = 1 << 7,	//!< Start button.
	VJOY_CTRL_LBUMPER   = 1 << 4,	//!< Left bumper.
	VJOY_CTRL_RBUMPER   = 1 << 5,	//!< Right bumper.
	VJOY_CTRL_TRIANGLE   = 1 << 3,	//!< Triangle button.
	VJOY_CTRL_CIRCLE     = 1 << 1,	//!< Circle button.
	VJOY_CTRL_CROSS      = 1 << 0,	//!< Cross button.
	VJOY_CTRL_SQUARE     = 1 << 2	//!< Square button.
};
#endif

// VJoy implementation
#ifdef __WIN32__
void abortVjoy()
{
	VJOY_MODE = false;
	VJOY_DEVID = 0;
	printf("\nERROR: An error occurred while initializing VJOY. Reverting back to keybinds.\n");
}
void initVjoy()
{
	if (!vJoyEnabled())
	{
		abortVjoy();
		return;
	}
	for (UINT devId = 1; devId <= 16; devId++)
	{
		if (VJD_STAT_FREE == GetVJDStatus(devId))
		{
			if (8 > GetVJDButtonNumber(devId))
			{
				printf("VJOY: ID:%u Buttons number insuffisent.\n", devId);
				continue;
			}
			if (!GetVJDAxisExist(devId, HID_USAGE_X) || 
				!GetVJDAxisExist(devId, HID_USAGE_Y) || 
				!GetVJDAxisExist(devId, HID_USAGE_Z) || 
				!GetVJDAxisExist(devId, HID_USAGE_RX) || 
				!GetVJDAxisExist(devId, HID_USAGE_RY) || 
				!GetVJDAxisExist(devId, HID_USAGE_RZ) || 
				!GetVJDAxisExist(devId, HID_USAGE_SL0) || 
				!GetVJDAxisExist(devId, HID_USAGE_SL1) || 
				!GetVJDAxisExist(devId, HID_USAGE_WHL) || 
				!GetVJDAxisExist(devId, HID_USAGE_POV))
			{
				printf("VJOY: ID:%u Some axis not defined.\n", devId);
				continue;
			}
			VJOY_DEVID = devId;
			break;
		}
		else
		{
			printf("VJOY: ID:%u Not ready, status:%u.\n", devId, GetVJDStatus(devId));
		}
	}
	
	if (0 == VJOY_DEVID || !AcquireVJD(VJOY_DEVID))
	{
		abortVjoy();
		return;
	}
	
	printf("Acquired ID:%u VJOY device.\n", VJOY_DEVID);
}
#endif

void loadConfig(const char* path)
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
	
#ifdef __WIN32__
	k1 = doc.FirstChildElement("VJOY_MODE");
	bool tmp_bool = 0;
	if (NULL != k1 && tinyxml2::XML_NO_ERROR == k1->QueryBoolText(&tmp_bool) && true == tmp_bool)
	{
		VJOY_MODE = true;
	}
	
	k1 = doc.FirstChildElement("VJOY_ALTERNATE");
	tmp_bool = 0;
	if (NULL != k1 && tinyxml2::XML_NO_ERROR == k1->QueryBoolText(&tmp_bool) && true == tmp_bool)
	{
		VJOY_ALTERNATE = true;
	}

	k1 = doc.FirstChildElement("VIGEM_MODE");
	unsigned int tmp_int = 0;
	if (NULL != k1 && tinyxml2::XML_NO_ERROR == k1->QueryUnsignedText(&tmp_int) && 0 != tmp_int)
	{
		VIGEM_MODE = tmp_int;
	}
#endif
	
}

#define u32 uint32_t
#define GAMEPAD_PORT 5000
typedef struct{
	u32 sock;
	struct sockaddr_in addrTo;
} Socket;

#if defined(__WIN32__) || defined(__CYGWIN__)
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
#elif __linux__
void SendMoveMouse (Display* display, int x, int y) {
    // Get current mouse position
    int mouse_x, mouse_y;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_return;
    Window widow_returned;
    XQueryPointer(display, DefaultRootWindow (display),
                  &widow_returned, &widow_returned, &root_x, &root_y,
                  &win_x, &win_y, &mask_return);
    mouse_x = root_x;
    mouse_y = root_y;

    // Set mouse position
    XWarpPointer(display, None, None, 0, 0, 0, 0, x - mouse_x, y - mouse_y);
    XFlush(display);
#endif
}

#if defined(__WIN32__) || defined(__CYGWIN__)
#define SEND_MOVE_MOUSE(x,y) SendMoveMouse(x,y)
#elif __linux__
#define SEND_MOVE_MOUSE(...) SendMoveMouse(display, __VA_ARGS__)
#endif

#if defined(__WIN32__) || defined(__CYGWIN__)
void SendMouseEvent(uint16_t event){
	INPUT ip = { 0 };
	ip.type = INPUT_MOUSE;
	ip.mi.dx = 0;
	ip.mi.dy = 0;
	ip.mi.mouseData = 0;
	ip.mi.time = 0;
	ip.mi.dwFlags = event;
	SendInput(1, &ip, sizeof(INPUT));
#elif __linux__
void SendMouseEvent (Display* display, uint16_t event) {
    Bool down;
    int button;

    switch (event) {
        case MOUSE_LEFT_DOWN:
            down = True;
            button = 1;
            break;

        case MOUSE_LEFT_UP:
            down = False;
            button = 1;
            break;

        case MOUSE_RIGHT_DOWN:
            down = True;
            button = 3;
            break;

        case MOUSE_RIGHT_UP:
            down = False;
            button = 3;
            break;
    }

    XTestFakeButtonEvent(display, button, down, CurrentTime);
    XFlush(display);
#endif
}

#if defined(__WIN32__) || defined(__CYGWIN__)
#define SEND_MOUSE_EVENT(x) SendMouseEvent(x)
#elif __linux__
#define SEND_MOUSE_EVENT(...) SendMouseEvent(display, __VA_ARGS__)
#endif

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

time_t getLastModifiedTime(const char *path) {
	#if defined(__MINGW32__) && defined(__stat64)
	struct __stat64 attr;
	__stat64(path, &attr);
	return attr.st_mtime;
	#else
	struct stat attr;
	stat(path, &attr);
	return attr.st_mtime;
	#endif
}

#ifdef __WIN32__
void ControllerCleanup()
{
    if (VJOY_MODE)
    {
        abortVjoy();
        VJOY_MODE = false;
    }
    else if (VIGEM_MODE == VIGEM_DEVICE_DS4)
    {
        vgDestroy();
        VIGEM_MODE = VIGEM_DEVICE_NONE;
    }
}

BOOL WINAPI HandlerRoutine(
    _In_ DWORD dwCtrlType
    )
{
   switch (dwCtrlType)
   {
       case CTRL_C_EVENT:
       {
            printf("\nQuitting...\n");
            ControllerCleanup();
			ExitProcess(0);
            return TRUE;
       }
       default:
       {
            return FALSE;
       }
   }
}
#endif

int main(int argc,char** argv){
	
	#ifdef __WIN32__
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

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
	#ifdef __WIN32__
	if (VJOY_MODE && (VIGEM_MODE != VIGEM_DEVICE_NONE))
	{
        printf("!!!CONFLICTING!!!\nvJoy and ViGEm cannot be enabled at the same time.\nEdit the config and restart the application to disable at least one of them.\n");
	}
    else if (VJOY_MODE)
    {
        printf("!!!STARTING IN VJOY MODE!!!\nEdit the config and restart the application to disable it.\n");
		initVjoy();
    }
    else if (VIGEM_MODE == VIGEM_DEVICE_DS4)
    {
        printf("!!!STARTING IN VIGEM MODE!!!\nEdit the config and restart the application to disable it.\n");
        if (!vgInit())
        {
            printf("ERROR: An error occurred while initializing ViGEm. Reverting back to keybinds.\n");
            VIGEM_MODE = VIGEM_DEVICE_NONE;
        }
    }
	#endif
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
	#ifdef __WIN32__
	JOYSTICK_POSITION_V3 joystickData;
	JOYSTICK_POSITION_V3 joystickDataOld;
	#endif

	for (;;){
	
		// Checking if we need a mapping reload
		time_t re_tick;
		#ifdef __linux__
		if ((re_tick = getLastModifiedTime("linux.xml")) != life_tick){
			loadConfig("linux.xml");
		#else
		if ((re_tick = getLastModifiedTime("windows.xml")) != life_tick){
			loadConfig("windows.xml");
		#endif
			life_tick = re_tick;
			printf("\nConfig file reloaded since a modification has been detected.");
		}		
		
		send(my_socket->sock, "request", 8, 0);
		int count = recv(my_socket->sock, (char*)&data, 256, 0);
		if (firstScan){
			firstScan = 0;
			memcpy(&olddata,&data,sizeof(PadPacket));
			#ifdef __WIN32__
			memcpy(&joystickDataOld,&joystickData,sizeof(JOYSTICK_POSITION_V3));
			#endif
		}

		if (count != 0){
			#ifdef __WIN32__
			if (VJOY_MODE)
			{				
				joystickData.bDevice = VJOY_DEVID;
				joystickData.wAxisZ = 16384;
				joystickData.wAxisZRot = 16384;
				joystickData.wSlider = 16384;
				
				LONG buttons = 0;
				if (data.buttons & SCE_CTRL_LEFT)
				{
					joystickData.wAxisZRot = 0;
				}
				else if (data.buttons & SCE_CTRL_RIGHT)
				{
					joystickData.wAxisZRot = 32768;
				}
				if (data.buttons & SCE_CTRL_UP)
				{
					joystickData.wSlider = 32768;
				}
				else if (data.buttons & SCE_CTRL_DOWN)
				{
					joystickData.wSlider = 0;
				}
				if (data.buttons & SCE_CTRL_TRIANGLE)
				{
					buttons = buttons | VJOY_CTRL_TRIANGLE;
				}
				if (data.buttons & SCE_CTRL_SQUARE)
				{
					buttons = buttons | VJOY_CTRL_SQUARE;
				}
				if (data.buttons & SCE_CTRL_CROSS)
				{
					buttons = buttons | VJOY_CTRL_CROSS;
				}
				if (data.buttons & SCE_CTRL_CIRCLE)
				{
					buttons = buttons | VJOY_CTRL_CIRCLE;
				}
				if (data.buttons & SCE_CTRL_RTRIGGER)
				{
					joystickData.wAxisZ = 0;
					//SetAxis(32768, VJOY_DEVID, HID_USAGE_WHL);
				}
				else
				{
					//SetAxis(16384, VJOY_DEVID, HID_USAGE_WHL);
				}
				if (data.buttons & SCE_CTRL_LTRIGGER)
				{
					joystickData.wAxisZ = 32768;
				}
				if (data.buttons & SCE_CTRL_START)
				{
					buttons = buttons | VJOY_CTRL_START;
				}
				if (data.buttons & SCE_CTRL_SELECT)
				{
					buttons = buttons | VJOY_CTRL_SELECT;
				}
				if (VJOY_ALTERNATE)
				{
					if (data.rx < 70)
					{
						buttons = buttons | VJOY_CTRL_LBUMPER;
					}
					else if (data.rx > 180)
					{
						buttons = buttons | VJOY_CTRL_RBUMPER;
					}
					joystickData.wAxisXRot = 16384;
					joystickData.wAxisYRot = 16384;
				}
				else
				{
					if (data.click & MOUSE_MOV && data.tx < SCREEN_WIDTH/2)
					{
						buttons = buttons | VJOY_CTRL_LBUMPER;
					}
					if (data.click & MOUSE_MOV && data.tx > SCREEN_WIDTH/2)
					{
						buttons = buttons | VJOY_CTRL_RBUMPER;
					}
					joystickData.wAxisXRot = data.rx*128;
					joystickData.wAxisYRot = data.ry*128;
				}
				joystickData.lButtons = buttons;
				joystickData.wAxisX = data.lx*128;
				joystickData.wAxisY = data.ly*128;
				
				if (0 != memcmp(&joystickDataOld, &joystickData, sizeof(JOYSTICK_POSITION_V3)))
				{
					if (!UpdateVJD(VJOY_DEVID, &joystickData))
					{
						printf("\nERROR: Feeding VJOY failed, please restart the app.\n");
						abortVjoy();
					}
					
					memcpy(&joystickDataOld,&joystickData,sizeof(JOYSTICK_POSITION_V3));
				}
			}
            else if (VIGEM_MODE == VIGEM_DEVICE_DS4)
            {
                if (!vgSubmit(&data))
                {
                    printf("\nERROR: Feeding VIGEM failed, please restart the app.\n");
					break;
                }
            }
			else
			#endif
			{
				// Down
				if ((data.buttons & SCE_CTRL_DOWN) && (!(olddata.buttons & SCE_CTRL_DOWN))) SEND_BUTTON_PRESS(KEY_DOWN);
				else if ((olddata.buttons & SCE_CTRL_DOWN) && (!(data.buttons & SCE_CTRL_DOWN))) SEND_BUTTON_RELEASE(KEY_DOWN);

				// Up
				if ((data.buttons & SCE_CTRL_UP) && (!(olddata.buttons & SCE_CTRL_UP))) SEND_BUTTON_PRESS(KEY_UP);
				else if ((olddata.buttons & SCE_CTRL_UP) && (!(data.buttons & SCE_CTRL_UP))) SEND_BUTTON_RELEASE(KEY_UP);

				// Left
				if ((data.buttons & SCE_CTRL_LEFT) && (!(olddata.buttons & SCE_CTRL_LEFT))) SEND_BUTTON_PRESS(KEY_LEFT);
				else if ((olddata.buttons & SCE_CTRL_LEFT) && (!(data.buttons & SCE_CTRL_LEFT))) SEND_BUTTON_RELEASE(KEY_LEFT);

				// Right
				if ((data.buttons & SCE_CTRL_RIGHT) && (!(olddata.buttons & SCE_CTRL_RIGHT))) SEND_BUTTON_PRESS(KEY_RIGHT);
				else if ((olddata.buttons & SCE_CTRL_RIGHT) && (!(data.buttons & SCE_CTRL_RIGHT))) SEND_BUTTON_RELEASE(KEY_RIGHT);

				// Triangle
				if ((data.buttons & SCE_CTRL_TRIANGLE) && (!(olddata.buttons & SCE_CTRL_TRIANGLE))) SEND_BUTTON_PRESS(KEY_TRIANGLE);
				else if ((olddata.buttons & SCE_CTRL_TRIANGLE) && (!(data.buttons & SCE_CTRL_TRIANGLE))) SEND_BUTTON_RELEASE(KEY_TRIANGLE);

				// Square
				if ((data.buttons & SCE_CTRL_SQUARE) && (!(olddata.buttons & SCE_CTRL_SQUARE))) SEND_BUTTON_PRESS(KEY_SQUARE);
				else if ((olddata.buttons & SCE_CTRL_SQUARE) && (!(data.buttons & SCE_CTRL_SQUARE))) SEND_BUTTON_RELEASE(KEY_SQUARE);

				// Cross
				if ((data.buttons & SCE_CTRL_CROSS) && (!(olddata.buttons & SCE_CTRL_CROSS))) SEND_BUTTON_PRESS(KEY_CROSS);
				else if ((olddata.buttons & SCE_CTRL_CROSS) && (!(data.buttons & SCE_CTRL_CROSS))) SEND_BUTTON_RELEASE(KEY_CROSS);

				// Circle
				if ((data.buttons & SCE_CTRL_CIRCLE) && (!(olddata.buttons & SCE_CTRL_CIRCLE))) SEND_BUTTON_PRESS(KEY_CIRCLE);
				else if ((olddata.buttons & SCE_CTRL_CIRCLE) && (!(data.buttons & SCE_CTRL_CIRCLE))) SEND_BUTTON_RELEASE(KEY_CIRCLE);

				// L Trigger
				if ((data.buttons & SCE_CTRL_LTRIGGER) && (!(olddata.buttons & SCE_CTRL_LTRIGGER))) SEND_BUTTON_PRESS(KEY_L);
				else if ((olddata.buttons & SCE_CTRL_LTRIGGER) && (!(data.buttons & SCE_CTRL_LTRIGGER))) SEND_BUTTON_RELEASE(KEY_L);

				// R Trigger
				if ((data.buttons & SCE_CTRL_RTRIGGER) && (!(olddata.buttons & SCE_CTRL_RTRIGGER))) SEND_BUTTON_PRESS(KEY_R);
				else if ((olddata.buttons & SCE_CTRL_RTRIGGER) && (!(data.buttons & SCE_CTRL_RTRIGGER))) SEND_BUTTON_RELEASE(KEY_R);

				// Start
				if ((data.buttons & SCE_CTRL_START) && (!(olddata.buttons & SCE_CTRL_START))) SEND_BUTTON_PRESS(KEY_START);
				else if ((olddata.buttons & SCE_CTRL_START) && (!(data.buttons & SCE_CTRL_START))) SEND_BUTTON_RELEASE(KEY_START);

				// Select
				if ((data.buttons & SCE_CTRL_SELECT) && (!(olddata.buttons & SCE_CTRL_SELECT))) SEND_BUTTON_PRESS(KEY_SELECT);
				else if ((olddata.buttons & SCE_CTRL_SELECT) && (!(data.buttons & SCE_CTRL_SELECT))) SEND_BUTTON_RELEASE(KEY_SELECT);

				// Left Analog
				if ((data.ly < 50) && (!(olddata.ly < 50))) SEND_BUTTON_PRESS(KEY_LANALOG_UP);
				else if ((olddata.ly < 50) && (!(data.ly < 50))) SEND_BUTTON_RELEASE(KEY_LANALOG_UP);
				if ((data.lx < 50) && (!(olddata.lx < 50))) SEND_BUTTON_PRESS(KEY_LANALOG_LEFT);
				else if ((olddata.lx < 50) && (!(data.lx < 50))) SEND_BUTTON_RELEASE(KEY_LANALOG_LEFT);
				if ((data.lx > 170) && (!(olddata.lx > 170))) SEND_BUTTON_PRESS(KEY_LANALOG_RIGHT);
				else if ((olddata.lx > 170) && (!(data.lx > 170))) SEND_BUTTON_RELEASE(KEY_LANALOG_RIGHT);
				if ((data.ly > 170) && (!(olddata.ly > 170))) SEND_BUTTON_PRESS(KEY_LANALOG_DOWN);
				else if ((olddata.ly > 170) && (!(data.ly > 170))) SEND_BUTTON_RELEASE(KEY_LANALOG_DOWN);

				// Right Analog
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
					if (data.click & MOUSE_MOV) SEND_MOVE_MOUSE(data.tx, data.ty);
					if ((data.click & LEFT_CLICK) && (!(olddata.click & LEFT_CLICK))) SEND_MOUSE_EVENT(MOUSE_LEFT_DOWN);
					else if ((olddata.click & LEFT_CLICK) && (!(data.click & LEFT_CLICK))) SEND_MOUSE_EVENT(MOUSE_LEFT_UP);
					if ((data.click & RIGHT_CLICK) && (!(olddata.click & RIGHT_CLICK))) SEND_MOUSE_EVENT(MOUSE_RIGHT_DOWN);
					else if ((olddata.click & RIGHT_CLICK) && (!(data.click & RIGHT_CLICK))) SEND_MOUSE_EVENT(MOUSE_RIGHT_UP);
				}
			}
			
			// Saving old pad status
			memcpy(&olddata,&data,sizeof(PadPacket));
		}
	}

    #ifdef __linux__
    XCloseDisplay(display);
    #elif defined(__WIN32__)
    ControllerCleanup();
    #endif

	return 1;
}
