#include <arpa/inet.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/motion.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <psp2/touch.h>
#include <psp2/types.h>
#include <vita2d.h>

#include <cassert>
#include <climits>

#include "ctrl.hpp"
#include "epoll.hpp"
#include "net.hpp"
#include <common.h>
#include <handshake_generated.h>

#ifdef DEBUG_IP
#include <debugnet.h>
#else
#define debugNetPrintf(...)
constexpr size_t NET_INIT_SIZE = 1 * 1024 * 1024;
#endif

constexpr size_t MAX_EPOLL_EVENTS = 10;
constexpr time_t MAX_HEARTBEAT_INTERVAL = 60;

typedef struct _MainThreadMessage {
  SceUID ev_flag_connect_state;
} MainThreadMessage;

typedef struct _ControlThreadMessage {
  SceUID ev_flag_ctrl_out;
  SceUID control_msg_pipe;
} ControlThreadMessage;

enum ConnectionState {
  DISCONNECT = 1,
  CONNECT = 2,
};

enum CtrlEvents {
  DATA_OUT = 1,
};

using offset_t = flatbuffers::uoffset_t;

void disconnect_client(std::shared_ptr<ClientData> client, SceUID ev_flag) {
  client->mark_for_removal();
  sceKernelSetEventFlag(ev_flag, DISCONNECT);
}

static int control_thread(__attribute__((unused)) unsigned int arglen,
                          void *argp) {
  assert(arglen == sizeof(ControlThreadMessage));

  ControlThreadMessage *message = static_cast<ControlThreadMessage *>(argp);
  SceCtrlData pad;
  SceMotionSensorState motion_data; // TODO: Needs calibration
  SceTouchData touch_data_front, touch_data_back;

  unsigned int state = 0;
  SceUInt TIMEOUT = UINT32_MAX;
  flatbuffers::FlatBufferBuilder builder;

  while (sceKernelWaitEventFlag(message->ev_flag_ctrl_out, DATA_OUT,
                                SCE_EVENT_WAITCLEAR, &state, &TIMEOUT) == 0) {
    sceCtrlPeekBufferPositive(0, &pad, 1);
    auto buttons = convert_pad_data(pad);

    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch_data_front, 1);
    auto data_front = convert_touch_data(builder, touch_data_front);

    sceTouchPeek(SCE_TOUCH_PORT_BACK, &touch_data_back, 1);
    auto data_back = convert_touch_data(builder, touch_data_back);

    sceMotionGetSensorState(&motion_data, 1);
    Pad::Vector3 accel(motion_data.accelerometer.x, motion_data.accelerometer.y,
                       motion_data.accelerometer.z);
    Pad::Vector3 gyro(motion_data.gyro.x, motion_data.gyro.y,
                      motion_data.gyro.z);
    Pad::MotionData motion(accel, gyro);

    auto packet =
        Pad::CreateMainPacket(builder, &buttons, pad.lx, pad.ly, pad.rx, pad.ry,
                              data_front, data_back, &motion, pad.timeStamp);
    builder.FinishSizePrefixed(packet);

    offset_t size = builder.GetSize();

    sceKernelSendMsgPipe(message->control_msg_pipe, &size, sizeof(size), 0,
                         NULL, NULL);
    sceKernelSendMsgPipe(message->control_msg_pipe, builder.GetBufferPointer(),
                         size, 0, NULL, NULL);

    builder.Clear();
  }

  return 0;
}

void add_client(int server_tcp_fd, SceUID epoll,
                ClientsManager &clients_manager, SceUID ev_flag_connect_state) {
  SceNetSockaddrIn clientaddr;
  unsigned int addrlen = sizeof(clientaddr);
  int client_fd =
      sceNetAccept(server_tcp_fd, (SceNetSockaddr *)&clientaddr, &addrlen);
  if (client_fd >= 0) {
    auto client_data = std::make_shared<ClientData>(client_fd, epoll);
    clients_manager.add_client(client_data);
    auto member_ptr = std::make_unique<EpollMember>(client_data);
    client_data->set_member_ptr(std::move(member_ptr));

    SceNetEpollEvent ev = {};
    ev.events = SCE_NET_EPOLLIN | SCE_NET_EPOLLOUT | SCE_NET_EPOLLHUP |
                SCE_NET_EPOLLERR;
    ev.data.ptr = client_data->member_ptr().get();
    auto nbio = 1;
    sceNetSetsockopt(client_data->ctrl_fd(), SCE_NET_SOL_SOCKET,
                     SCE_NET_SO_NBIO, &nbio, sizeof(nbio));

    sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_ADD, client_data->ctrl_fd(),
                       &ev);
    sceKernelSetEventFlag(ev_flag_connect_state, CONNECT);
  }
}

static int main_thread(__attribute__((unused)) unsigned int arglen,
                       void *argp) {
  assert(arglen == sizeof(MainThreadMessage));

  MainThreadMessage *message = static_cast<MainThreadMessage *>(argp);

  // Create the thread which manages the control part
  auto flag_ctrl_out = sceKernelCreateEventFlag("flag_ctrl", 0, 0, NULL);
  auto ctrl_msg_pipe =
      sceKernelCreateMsgPipe("ctrl_msg_pipe", 0x40, 12, 0x1000, NULL);
  ControlThreadMessage ctrl_message = {
      flag_ctrl_out,
      ctrl_msg_pipe,
  };
  auto ctrl_thread_id = sceKernelCreateThread("CtrlThread", &control_thread,
                                              0x10000100, 0x10000, 0, 0, NULL);
  sceKernelStartThread(ctrl_thread_id, sizeof(ctrl_message), &ctrl_message);

  auto server_tcp_fd =
      sceNetSocket("SERVER_SOCKET", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
  SceNetSockaddrIn serveraddr;
  serveraddr.sin_family = SCE_NET_AF_INET;
  serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
  serveraddr.sin_port = sceNetHtons(NET_PORT);
  sceNetBind(server_tcp_fd, (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));

  auto nbio = 1;
  sceNetSetsockopt(server_tcp_fd, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &nbio,
                   sizeof(nbio));
  sceNetListen(server_tcp_fd, 2);

  auto server_udp_fd =
      sceNetSocket("SERVER_UDP_SOCKET", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);
  sceNetBind(server_udp_fd, (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));

  ClientsManager clients_manager;
  SceUID epoll = sceNetEpollCreate("SERVER_EPOLL", 0);

  EpollMember server_ptr;
  SceNetEpollEvent ev = {};
  ev.events = SCE_NET_EPOLLIN;
  ev.data.ptr = &server_ptr;
  sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_ADD, server_tcp_fd, &ev);

  SceNetEpollEvent events[MAX_EPOLL_EVENTS];
  int n;

  while ((n = sceNetEpollWait(epoll, events, MAX_EPOLL_EVENTS,
                              MIN_POLLING_INTERVAL_MICROS)) >= 0) {
    for (size_t i = 0; i < (unsigned)n; i++) {
      auto ev = events[i];
      EpollMember *data = static_cast<EpollMember *>(ev.data.ptr);

      if (ev.events & SCE_NET_EPOLLHUP || ev.events & SCE_NET_EPOLLERR) {
        if (data->type == SocketType::CLIENT) {
          debugNetPrintf(ERROR, "Client %d closed connection\n", data->fd());
          disconnect_client(data->client(), message->ev_flag_connect_state);
        }
      } else if (ev.events & SCE_NET_EPOLLIN) {
        if (data->type == SocketType::SERVER) {
          add_client(server_tcp_fd, epoll, clients_manager,
                     message->ev_flag_connect_state);
          continue;
        }

        auto client = data->client();
        try {
          debugNetPrintf(DEBUG, "Handling ingoing data\n");
          net::handle_ingoing_data(*client);
        } catch (const net::NetException &e) {
          debugNetPrintf(ERROR, "Network exception: %s\n", e.what());
          if (e.error_code() == SCE_NET_ECONNRESET || e.error_code() == 0) {
            disconnect_client(client, message->ev_flag_connect_state);
          }
        } catch (const std::exception &e) {
          debugNetPrintf(ERROR, "Exception: %s\n", e.what());
          disconnect_client(client, message->ev_flag_connect_state);
        }
      } else if (ev.events & SCE_NET_EPOLLOUT) {
        if (data->type == SocketType::SERVER) {
          continue;
        }

        auto client = data->client();

        switch (client->state()) {
        case ClientData::State::WaitingForServerConfirm: {
          try {
            net::send_handshake_response(*client, NET_PORT,
                                         MAX_HEARTBEAT_INTERVAL);
            debugNetPrintf(DEBUG, "Sending handshake response\n");

            SceNetEpollEvent ev = {};
            ev.events = SCE_NET_EPOLLIN | SCE_NET_EPOLLHUP | SCE_NET_EPOLLERR;
            ev.data.ptr = client->member_ptr().get();
            sceNetEpollControl(epoll, SCE_NET_EPOLL_CTL_MOD, client->ctrl_fd(),
                               &ev);
          } catch (const net::NetException &e) {
            debugNetPrintf(ERROR, "Network exception: %s\n", e.what());
            if (e.error_code() == SCE_NET_ECONNRESET) {
              disconnect_client(client, message->ev_flag_connect_state);
            }
          } catch (const std::exception &e) {
            debugNetPrintf(ERROR, "Exception: %s\n", e.what());
            disconnect_client(client, message->ev_flag_connect_state);
          }

          break;
        }

        default:
          break;
        }
      }
    }

    auto clients = clients_manager.clients();

    if (clients.empty())
      continue;

    for (auto &client : clients) {
      debugNetPrintf(DEBUG, "Checking heartbeat: %lu for %lu\n",
                     client->time_since_last_heartbeat(), client->ctrl_fd());
      if (client->time_since_last_heartbeat() > MAX_HEARTBEAT_INTERVAL) {
        debugNetPrintf(INFO, "Delay expired, disconnecting client: %lu",
                       client->ctrl_fd());
        disconnect_client(client, message->ev_flag_connect_state);
      }
      client->shrink_buffer();
    }

    clients_manager.remove_marked_clients();

    if (std::none_of(clients.begin(), clients.end(),
                     [](const std::shared_ptr<ClientData> &client) {
                       return client->state() == ClientData::State::Connected &&
                              client->is_polling_time_elapsed();
                     })) {
#ifdef DEBUG_IP
      for (auto &client : clients) {
        auto client_addr = client->data_conn_info();
        auto client_addr_in =
            reinterpret_cast<SceNetSockaddrIn *>(&client_addr);
        char ip[INET_ADDRSTRLEN];
        sceNetInetNtop(SCE_NET_AF_INET,
                       static_cast<void *>(&client_addr_in->sin_addr), ip,
                       sizeof(ip));

        debugNetPrintf(DEBUG, "Address: %s:%d, state: %d, last polling: %ld\n",
                       ip, sceNetNtohs(client_addr_in->sin_port),
                       client->state(), client->time_since_last_sent_data());
      }
#endif
      continue;
    }

    sceKernelSetEventFlag(flag_ctrl_out, DATA_OUT);

    offset_t buffer_size;
    sceKernelReceiveMsgPipe(ctrl_msg_pipe, &buffer_size, sizeof(buffer_size), 0,
                            NULL, NULL);

    uint8_t *buffer = new uint8_t[buffer_size];
    sceKernelReceiveMsgPipe(ctrl_msg_pipe, buffer, buffer_size, 0, NULL, NULL);

    for (auto &client : clients) {
      if (client->state() == ClientData::State::Connected &&
          client->is_polling_time_elapsed()) {
        client->update_sent_data_time();
        auto client_addr = client->data_conn_info();
        auto addrlen = sizeof(client_addr);
#ifdef DEBUG_IP
        auto client_addr_in =
            reinterpret_cast<SceNetSockaddrIn *>(&client_addr);
        char ip[INET_ADDRSTRLEN];
        sceNetInetNtop(SCE_NET_AF_INET,
                       static_cast<void *>(&client_addr_in->sin_addr), ip,
                       sizeof(ip));

        debugNetPrintf(DEBUG, "Sending data to %s:%d\n", ip,
                       sceNetNtohs(client_addr_in->sin_port));
#endif
        sceNetSendto(server_udp_fd, buffer, buffer_size, 0, &client_addr,
                     addrlen);
      }
    }

    delete[] buffer;
  }

  sceKernelSetEventFlag(message->ev_flag_connect_state, DISCONNECT);

  return 0;
}

vita2d_pgf *debug_font;
uint32_t text_color;

int main() {
  // Enabling analog, motion and touch support
  sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
  sceMotionStartSampling();
  sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                           SCE_TOUCH_SAMPLING_STATE_START);
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
  if ((unsigned)ret == SCE_NET_ERROR_ENOTINIT) {
    SceNetInitParam initparam;
    initparam.memory = malloc(NET_INIT_SIZE);
    initparam.size = NET_INIT_SIZE;
    initparam.flags = 0;
    ret = sceNetInit(&initparam);
  }
  sceNetCtlInit();
  SceNetCtlInfo info;
  sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
  sprintf(vita_ip, "%s", info.ip_address);
  SceNetInAddr vita_addr;
  sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &vita_addr);

#ifdef DEBUG_IP
  // DEBUG
  debugNetInit(DEBUG_IP, 8174, DEBUG);
#endif

  SceUID ev_connect = sceKernelCreateEventFlag("ev_con", 0, 0, NULL);
  MainThreadMessage main_message = {ev_connect};
  // Open the main thread with an event flag in argument to write the
  // connection state
  SceUID main_thread_id = sceKernelCreateThread(
      "MainThread", &main_thread, 0x10000100, 0x10000, 0, 0, NULL);
  sceKernelStartThread(main_thread_id, sizeof(main_message), &main_message);

  unsigned int state = 0;
  SceUInt TIMEOUT = (SceUInt)UINT32_MAX;
  do {
    vita2d_start_drawing();
    vita2d_clear_screen();
    vita2d_pgf_draw_text(debug_font, 2, 20, text_color, 1.0,
                         "VitaPad build from " __DATE__ ", " __TIME__);
    vita2d_pgf_draw_textf(debug_font, 2, 60, text_color, 1.0,
                          "Listening on:\nIP: %s\nPort: %d", vita_ip, NET_PORT);
    vita2d_pgf_draw_textf(debug_font, 2, 200, text_color, 1.0, "Status: %s",
                          state & CONNECT ? "Connected" : "Not connected");
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
  } while (sceKernelWaitEventFlag(ev_connect, CONNECT | DISCONNECT,
                                  SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR,
                                  &state, &TIMEOUT) == 0);

#ifdef DEBUG_IP
  debugNetFinish();
#endif // DEBUG_IP

  sceNetCtlTerm();
  sceNetTerm();
  sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);

  vita2d_fini();
  vita2d_free_pgf(debug_font);
  return 1;
}
