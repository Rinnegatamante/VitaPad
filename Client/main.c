#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#define _GNU_SOURCE
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

#include "main.h"
#include "../include/net_types.h"

#ifdef _WIN32
PVIGEM_TARGET target;
#endif

#ifdef __linux__
#define die(str)                   \
    printf("%s : ", __FUNCTION__); \
    puts(str);                     \
    puts(strerror(errno));         \
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
    struct vita vita_dev = create_device();
    if (vita_dev.dev == NULL || vita_dev.sensor_dev == NULL)
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
    vita.sin_port = htons(NET_PORT);

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

    while (poll(fd, 1, -1) != -1)
    {
        if (fd[0].revents & POLLRDHUP)
        {
            puts("Connection closed.");
            exit(EXIT_SUCCESS);
        }
        if (fd[0].revents & POLLIN)
        {
            Packet pkg;
            if (recv(sock, (char *)&pkg, sizeof(Packet), 0) > 0)
            {
                switch (pkg.type)
                {
                case PAD:
                    emit_abs(vita_dev.dev, ABS_X, pkg.packet.pad.lx);
                    emit_abs(vita_dev.dev, ABS_Y, pkg.packet.pad.ly);

                    emit_abs(vita_dev.dev, ABS_RX, pkg.packet.pad.rx);
                    emit_abs(vita_dev.dev, ABS_RY, pkg.packet.pad.ry);

                    emit_button(vita_dev.dev, BTN_B, pkg.packet.pad.buttons.circle);
                    emit_button(vita_dev.dev, BTN_A, pkg.packet.pad.buttons.cross);
                    emit_button(vita_dev.dev, BTN_Y, pkg.packet.pad.buttons.square);
                    emit_button(vita_dev.dev, BTN_X, pkg.packet.pad.buttons.triangle);
                    emit_button(vita_dev.dev, BTN_TL, pkg.packet.pad.buttons.lt);
                    emit_button(vita_dev.dev, BTN_TR, pkg.packet.pad.buttons.rt);
                    emit_button(vita_dev.dev, BTN_START, pkg.packet.pad.buttons.start);
                    emit_button(vita_dev.dev, BTN_SELECT, pkg.packet.pad.buttons.select);
                    emit_button(vita_dev.dev, BTN_DPAD_UP, pkg.packet.pad.buttons.up);
                    emit_button(vita_dev.dev, BTN_DPAD_DOWN, pkg.packet.pad.buttons.down);
                    emit_button(vita_dev.dev, BTN_DPAD_LEFT, pkg.packet.pad.buttons.left);
                    emit_button(vita_dev.dev, BTN_DPAD_RIGHT, pkg.packet.pad.buttons.right);
                    break;
                case TOUCH:
                    switch (pkg.packet.touch.port)
                    {
                    case FRONT:
                        for (size_t i = 0; i < pkg.packet.touch.num_rep; i++)
                        {
                            emit_touch(vita_dev.dev, i, pkg.packet.touch.reports[i].id, pkg.packet.touch.reports[i].x, pkg.packet.touch.reports[i].y, pkg.packet.touch.reports[i].pressure);
                        }
                        emit_touch_sync(vita_dev.dev);
                        break;
                    case BACK:
                        for (size_t i = 0; i < pkg.packet.touch.num_rep; i++)
                        {
                            emit_touch(vita_dev.sensor_dev, i, pkg.packet.touch.reports[i].id, pkg.packet.touch.reports[i].x, pkg.packet.touch.reports[i].y, pkg.packet.touch.reports[i].pressure);
                        }
                        emit_touch_sync(vita_dev.sensor_dev);
                        break;
                    default:
                        break;
                    }
                    break;
                case MOTION:
                    //TODO:
                    break;

                default:
                    break;
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
            Packet pkg;
            if (recv(sock, (char *)&pkg, sizeof(Packet), 0) > 0)
            {
                switch (pkg.type)
                {
                case PAD:
                    XUSB_REPORT report;
                    XUSB_REPORT_INIT(&report);
                    abs_report(report.sThumbLX, pkg.packet.pad.lx);
                    abs_report(report.sThumbLY, pkg.packet.pad.ly);
                    abs_report(report.sThumbRX, pkg.packet.pad.rx);
                    abs_report(report.sThumbRY, pkg.packet.pad.ry);

                    button_report(report.wButtons, pkg.packet.pad.buttons.circle, XUSB_GAMEPAD_B);
                    button_report(report.wButtons, pkg.packet.pad.buttons.cross, XUSB_GAMEPAD_A);
                    button_report(report.wButtons, pkg.packet.pad.buttons.square, XUSB_GAMEPAD_X);
                    button_report(report.wButtons, pkg.packet.pad.buttons.triangle, XUSB_GAMEPAD_Y);
                    button_report(report.wButtons, pkg.packet.pad.buttons.lt, XUSB_GAMEPAD_LEFT_SHOULDER);
                    button_report(report.wButtons, pkg.packet.pad.buttons.rt, XUSB_GAMEPAD_RIGHT_SHOULDER);
                    button_report(report.wButtons, pkg.packet.pad.buttons.start, XUSB_GAMEPAD_START);
                    button_report(report.wButtons, pkg.packet.pad.buttons.select, XUSB_GAMEPAD_BACK);
                    button_report(report.wButtons, pkg.packet.pad.buttons.up, XUSB_GAMEPAD_DPAD_UP);
                    button_report(report.wButtons, pkg.packet.pad.buttons.down, XUSB_GAMEPAD_DPAD_DOWN);
                    button_report(report.wButtons, pkg.packet.pad.buttons.left, XUSB_GAMEPAD_DPAD_LEFT);
                    button_report(report.wButtons, pkg.packet.pad.buttons.right, XUSB_GAMEPAD_DPAD_RIGHT);

                    if (!VIGEM_SUCCESS(send_report(report, target)))
                    {
                        puts("report error");
                        exit(EXIT_FAILURE);
                    }
                    break;
                case TOUCH:
                    //TODO: Wait for ViGem touchpad support
                    break;
                case MOTION:
                    MotionPacket motion = pkg.packet.motion;
                    //TODO: Wait for ViGem motion support
                    break;

                default:
                    break;
                }
            }
        }
    }
#endif

    return EXIT_SUCCESS;
}
