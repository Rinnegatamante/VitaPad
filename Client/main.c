#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <poll.h>
#include <linux/uinput.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "uinput.h"
#elif defined _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>
#include "vigem.h"
#include "vendor/include/ViGEm/Client.h"
#include "vendor/include/ViGEm/Common.h"
#endif

#include "../include/types.h"

#ifdef _WIN32
PVIGEM_TARGET target;
#endif

#ifdef __linux__
#define die(str)           \
    puts(str);             \
    puts(strerror(errno)); \
    exit(EXIT_FAILURE);
#endif

#ifdef _WIN32
void cleanup(void)
{
    vigem_cleanup();
    WSACleanup();
    vigem_target_free(target);
}
#endif

int main(int argc, char *argv[])
{
#ifdef __linux__
    int dev_fd = create_device();
    if (dev_fd == -1)
    {
        die("uinput error");
    }
#elif defined _WIN32
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 0), &WSAData);
    atexit(cleanup);

    target = create_device();
    //TODO: manage errors
    if (target == NULL)
    {
        puts("Error during device creation");
        exit(EXIT_FAILURE);
    }
#endif

    if (argc < 2)
    {
        puts("missing ip argument");
        exit(EXIT_FAILURE);
    }
    char *ip = argv[1];

#ifdef __linux__
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        puts("socket error");
        exit(EXIT_FAILURE);
    }

#elif defined _WIN32
    SOCKET sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        puts("socket error");
        exit(EXIT_FAILURE);
    }

#endif

#ifdef __linux__
    struct sockaddr_in vita;
#elif defined _WIN32
    SOCKADDR_IN vita;
#endif

    vita.sin_addr.s_addr = inet_addr(ip);
    vita.sin_family = AF_INET;
    vita.sin_port = htons(GAMEPAD_PORT);

#ifdef __linux__
    if (connect(sock, (struct sockaddr *)&vita, sizeof(struct sockaddr)) == -1)
    {
        die("Failed to connect");
    }
#elif defined _WIN32
    if (connect(sock, (SOCKADDR *)&vita, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        puts("connect error");
        exit(EXIT_FAILURE);
    }
#endif

#ifdef __linux__
    struct pollfd fd[1];
    fd[0].fd = sock;
    fd[0].events = POLLIN | POLLRDHUP;

    while (poll(fd, 1, (int)5E3) != -1)
    {
        Packet packet;
        if (fd[0].revents & POLLRDHUP)
        {
            puts("Connection closed.");
            exit(EXIT_SUCCESS);
        }
        if (fd[0].revents & POLLIN)
        {
            if (recv(sock, (char *)&packet, sizeof(Packet), 0) != 0)
            {
                if (emit(dev_fd, EV_ABS, ABS_X, packet.lx) == -1)
                {
                    die("WRITEERR: lx");
                }
                if (emit(dev_fd, EV_ABS, ABS_Y, packet.ly) == -1)
                {
                    die("WRITEERR: ly");
                }

                if (emit(dev_fd, EV_ABS, ABS_RX, packet.rx) == -1)
                {
                    die("WRITEERR: rx");
                }
                if (emit(dev_fd, EV_ABS, ABS_RY, packet.ry) == -1)
                {
                    die("WRITEERR: ry");
                }

                if (emit(dev_fd, EV_KEY, BTN_B, packet.buttons.circle) == -1)
                {
                    die("WRITEERR: buttons.circle");
                }
                if (emit(dev_fd, EV_KEY, BTN_A, packet.buttons.cross) == -1)
                {
                    die("WRITEERR: buttons.cross");
                }
                if (emit(dev_fd, EV_KEY, BTN_Y, packet.buttons.square) == -1)
                {
                    die("WRITEERR: buttons.square");
                }
                if (emit(dev_fd, EV_KEY, BTN_X, packet.buttons.triangle) == -1)
                {
                    die("WRITEERR: buttons.triangle");
                }
                if (emit(dev_fd, EV_KEY, BTN_TL, packet.buttons.lt) == -1)
                {
                    die("WRITEERR: buttons.lt");
                }
                if (emit(dev_fd, EV_KEY, BTN_TR, packet.buttons.rt) == -1)
                {
                    die("WRITEERR: buttons.rt");
                }
                if (emit(dev_fd, EV_KEY, BTN_START, packet.buttons.start) == -1)
                {
                    die("WRITEERR: buttons.start");
                }
                if (emit(dev_fd, EV_KEY, BTN_SELECT, packet.buttons.select) == -1)
                {
                    die("WRITEERR: buttons.select");
                }
                if (emit(dev_fd, EV_KEY, BTN_DPAD_UP, packet.buttons.up) == -1)
                {
                    die("WRITEERR: buttons.up");
                }
                if (emit(dev_fd, EV_KEY, BTN_DPAD_DOWN, packet.buttons.down) == -1)
                {
                    die("WRITEERR: buttons.down");
                }
                if (emit(dev_fd, EV_KEY, BTN_DPAD_LEFT, packet.buttons.left) == -1)
                {
                    die("WRITEERR: buttons.left");
                }
                if (emit(dev_fd, EV_KEY, BTN_DPAD_RIGHT, packet.buttons.right) == -1)
                {
                    die("WRITEERR: buttons.right");
                }
            }
        }
    }
#elif defined _WIN32
    WSAPOLLFD fd = {0};
    fd.fd = sock;
    fd.events = POLLRDNORM;

    while (true)
    {
        int ret = WSAPoll(&fd, 1, -1);

        if (ret == SOCKET_ERROR)
            exit(EXIT_FAILURE);

        if (fd.revents & POLLRDNORM)
        {
            Packet packet;
            if (recv(sock, (char *)&packet, sizeof(Packet), 0) != 0)
            {
                XUSB_REPORT report;
                XUSB_REPORT_INIT(&report);
                report.sThumbLX = (packet.lx - 128) * 128;
                report.sThumbLY = (packet.ly - 128) * 128;
                report.sThumbRX = (packet.rx - 128) * 128;
                report.sThumbRY = (packet.ry - 128) * 128;

                if (packet.buttons.circle)
                    report.wButtons |= XUSB_GAMEPAD_B;

                if (packet.buttons.cross)
                    report.wButtons |= XUSB_GAMEPAD_A;

                if (packet.buttons.square)
                    report.wButtons |= XUSB_GAMEPAD_X;

                if (packet.buttons.triangle)
                    report.wButtons |= XUSB_GAMEPAD_Y;

                if (packet.buttons.lt)
                    report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;

                if (packet.buttons.rt)
                    report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;

                if (packet.buttons.start)
                    report.wButtons |= XUSB_GAMEPAD_START;

                if (packet.buttons.select)
                    report.wButtons |= XUSB_GAMEPAD_BACK;

                if (packet.buttons.up)
                    report.wButtons |= XUSB_GAMEPAD_DPAD_UP;

                if (packet.buttons.down)
                    report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;

                if (packet.buttons.left)
                    report.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;

                if (packet.buttons.right)
                    report.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;

                if (!VIGEM_SUCCESS(send_report(report, target)))
                {
                    puts("report error");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
#endif

#ifdef _WIN32
    WSACleanup();
#endif

    return EXIT_SUCCESS;
}
