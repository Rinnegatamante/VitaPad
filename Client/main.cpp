#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <linux/uinput.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "ctrl.hpp"
#include "sock_data.cpp"
#elif defined _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>
#include <map>

#include "vigem.h"
#include "vendor/include/ViGEm/Client.h"
#include "vendor/include/ViGEm/Common.h"
#endif

#include <common.h>
#include <pad.fbs.hpp>

#define PACKET_SIZE_LIMIT 1024

#ifdef _WIN32
PVIGEM_TARGET target;
#endif

#ifdef __linux__
#define MAX_EPOLL_EVENTS 5
#define die(str)                   \
    printf("%s : ", __FUNCTION__); \
    puts(str);                     \
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
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags >= 0)
    {
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }
#elif defined _WIN32
    if (connect(sock, (SOCKADDR *)&vita, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        puts("connect error");
        exit(EXIT_FAILURE);
    }
#endif

#ifdef __linux__
    int epoll_fd = epoll_create1(0), n;
    struct epoll_event ev = {0};
    ev.data.ptr = new SocketData(sock);
    ev.events = EPOLLIN | EPOLLRDHUP;
    struct epoll_event *events = new struct epoll_event[MAX_EPOLL_EVENTS];
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);

    while ((n = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 500)) != -1)
    {
        for (uint8_t i = 0; i < n; i++)
        {
            auto data = (SocketData *)events[i].data.ptr;
            if (events[i].events & EPOLLRDHUP)
            {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);
                // puts("Connection closed");
                // exit(EXIT_SUCCESS);
            }

            else if (events[i].events & EPOLLOUT)
            {
                //TODO:
            }

            else if (events[i].events & EPOLLIN)
            {
                if (!data->size_fetched())
                {
                    uint8_t buf[4];
                    int received = recv(data->fd, buf, sizeof(flatbuffers::uoffset_t), 0);
                    if (received == -1)
                    {
                        if (errno == EWOULDBLOCK || errno == EAGAIN)
                            continue;
                        else
                        {
                            puts(strerror(errno));
                            die("Socket error");
                        }
                    }
                    if (received == 0)
                        continue;

                    data->push_size_data(buf, received);
                }

                if (data->size_fetched())
                {
                    // Probably an error, clear the memory
                    if (data->size() == 0 || data->size() >= PACKET_SIZE_LIMIT)
                    {
                        data->clear();
                    }
                    else
                    {
                        std::vector<uint8_t> buffer(data->remaining());
                        int received = recv(data->fd, buffer.data(), data->remaining(), 0);
                        if (received == -1)
                        {
                            if (errno == EWOULDBLOCK || errno == EAGAIN)
                                continue;
                            else
                            {
                                puts(strerror(errno));
                                die("Socket error");
                            }
                        }

                        data->push_buf_data(buffer.data(), received);
                    }
                }
            }

            if (data->size_fetched() && data->remaining() == 0)
            {
                auto verifier = flatbuffers::Verifier(data->buffer(), data->size());
                if (Pad::VerifyMainPacketBuffer(verifier))
                {
                    auto pad_data = Pad::GetMainPacket(data->buffer());
                    emit_pad_data(pad_data, vita_dev);
                }
                data->clear();
            }
        }
    }
#elif defined _WIN32
    //TODO: Support for multiple devices
    fd_set rfds, wfds, efds;

    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);

    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);

    FD_ZERO(&efds);
    FD_SET(sock, &efds);

    std::vector<SocketData> socks_data;
    //TODO: config for x360/ds4
    socks_data.push_back(sock, SocketData(sock, DualShock4Wired));

    TIMEVAL timeval;
    timeval.tv_sec = 0;
    timeval.tv_usec = 200 * 10E3;

    while ((n = select(1, &rfds, &wfds, &efds, &timeval)) != SOCKET_ERROR)
    {
        for (size_t i = 0; i < n; i++)
        {
            SocketData data = socks_data[i];
            SOCKET fd = data.socket;

            if (FD_ISSET(fd, &rfds))
            {
                if (!data.size_fetched())
                {
                    uint8_t buf[4];
                    int received = recv(fd, buf, sizeof(flatbuffers::uoffset_t), 0);
                    if (received == -1)
                    {
                        if (errno == EWOULDBLOCK || errno == EAGAIN)
                            continue;
                    }
                    if (received == 0)
                        continue;

                    data.push_size_data(buf, received);
                }

                if (data.size_fetched())
                {
                    // Probably an error, clear the memory
                    if (data.size() == 0 || data.size() >= PACKET_SIZE_LIMIT)
                    {
                        data.clear();
                    }
                    else
                    {
                        std::vector<uint8_t> buffer(data.remaining());
                        int received = recv(data.fd, buffer.data(), data.remaining(), 0);
                        if (received == -1)
                        {
                            if (errno == EWOULDBLOCK || errno == EAGAIN)
                                continue;
                        }

                        data.push_buf_data(buffer.data(), received);
                    }
                }
            }

            if (data.size_fetched() && data.remaining() == 0 &&
                data.device_usable)
            {
                auto verifier = flatbuffers::Verifier(data.buffer(), data.size());
                if (Pad::VerifyMainPacketBuffer(verifier))
                {
                    auto pad_data = Pad::GetMainPacket(data.buffer());
                    emit_pad_data(pad_data, data.target);
                }
                data.clear();
            }
        }
    }
}
#endif

    return EXIT_SUCCESS;
}