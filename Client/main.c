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

#include "main.h"
#include "../include/types.h"

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

    while (poll(fd, 1, -1) != -1)
    {
        if (fd[0].revents & POLLRDHUP)
        {
            puts("Connection closed.");
            exit(EXIT_SUCCESS);
        }
        if (fd[0].revents & POLLIN)
        {
            Packet packet;
            if (recv(sock, (char *)&packet, sizeof(Packet), 0) != 0)
            {
                emit_abs(dev_fd, ABS_X, packet.lx);
                emit_abs(dev_fd, ABS_Y, packet.ly);

                emit_abs(dev_fd, ABS_RX, packet.rx);
                emit_abs(dev_fd, ABS_RY, packet.ry);

                emit_button(dev_fd, BTN_B, packet.buttons.circle);
                emit_button(dev_fd, BTN_A, packet.buttons.cross);
                emit_button(dev_fd, BTN_Y, packet.buttons.square);
                emit_button(dev_fd, BTN_X, packet.buttons.triangle);
                emit_button(dev_fd, BTN_TL, packet.buttons.lt);
                emit_button(dev_fd, BTN_TR, packet.buttons.rt);
                emit_button(dev_fd, BTN_START, packet.buttons.start);
                emit_button(dev_fd, BTN_SELECT, packet.buttons.select);
                emit_button(dev_fd, BTN_DPAD_UP, packet.buttons.up);
                emit_button(dev_fd, BTN_DPAD_DOWN, packet.buttons.down);
                emit_button(dev_fd, BTN_DPAD_LEFT, packet.buttons.left);
                emit_button(dev_fd, BTN_DPAD_RIGHT, packet.buttons.right);
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
                abs_report(report.sThumbLX, packet.lx);
                abs_report(report.sThumbLY, packet.ly);
                abs_report(report.sThumbRX, packet.rx);
                abs_report(report.sThumbRY, packet.ry);

                button_report(report.wButtons, packet.buttons.circle, XUSB_GAMEPAD_B);
                button_report(report.wButtons, packet.buttons.cross, XUSB_GAMEPAD_A);
                button_report(report.wButtons, packet.buttons.square, XUSB_GAMEPAD_X);
                button_report(report.wButtons, packet.buttons.triangle, XUSB_GAMEPAD_Y);
                button_report(report.wButtons, packet.buttons.lt, XUSB_GAMEPAD_LEFT_SHOULDER);
                button_report(report.wButtons, packet.buttons.rt, XUSB_GAMEPAD_RIGHT_SHOULDER);
                button_report(report.wButtons, packet.buttons.start, XUSB_GAMEPAD_START);
                button_report(report.wButtons, packet.buttons.select, XUSB_GAMEPAD_BACK);
                button_report(report.wButtons, packet.buttons.up, XUSB_GAMEPAD_DPAD_UP);
                button_report(report.wButtons, packet.buttons.down, XUSB_GAMEPAD_DPAD_DOWN);
                button_report(report.wButtons, packet.buttons.left, XUSB_GAMEPAD_DPAD_LEFT);
                button_report(report.wButtons, packet.buttons.right, XUSB_GAMEPAD_DPAD_RIGHT);

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
