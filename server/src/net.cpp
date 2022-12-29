#include "net.hpp"

int send_all(int fd, const void *buf, unsigned int size) {
  const char *buf_ptr = static_cast<const char *>(buf);
  int bytes_sent;

  while (size > 0) {
    bytes_sent = sceNetSend(fd, buf_ptr, size, 0);
    if (bytes_sent < 0)
      return bytes_sent;

    buf_ptr += bytes_sent;
    size -= bytes_sent;
  }

  return bytes_sent;
}

bool net::handle_handshake(ClientData &client) {
  constexpr size_t buffer_size = 64;

  uint8_t buffer[buffer_size];
  int received;
  while ((received = sceNetRecv(client.ctrl_fd(), buffer, buffer_size, 0)) >
         0) {
    client.add_to_buffer(buffer, received);

    auto has_run = client.with_handshake_callback([&](auto &handshake) {
      SceNetSockaddrIn clientaddr;
      unsigned int addrlen = sizeof(clientaddr);
      sceNetGetpeername(client.ctrl_fd(),
                        reinterpret_cast<SceNetSockaddr *>(&clientaddr),
                        &addrlen);
      clientaddr.sin_port = sceNetHtons(handshake.port());

      auto addr = reinterpret_cast<SceNetSockaddr *>(&clientaddr);
      client.set_data_conn_info(*addr);
      client.set_state(ClientData::State::WaitingForServerConfirm);
    });

    if (has_run)
      return true;
  }

  if (received <= 0) {
    switch ((unsigned)received) {
    case SCE_NET_ERROR_EWOULDBLOCK:
      break;
    default:
      throw NetException(received);
    }
  }

  return false;
}

bool net::handle_heartbeat(ClientData &client) {
  constexpr size_t buffer_size = sizeof(heartbeat_magic);

  uint8_t buffer[buffer_size];
  int received;

  while ((received = sceNetRecv(client.ctrl_fd(), buffer, buffer_size, 0)) >
         0) {
    client.add_to_buffer(buffer, received);
    auto heartbeat_opt = client.retrieve_heartbeat_buffer();
    if (heartbeat_opt) {
      return true;
    }
  }

  if (received <= 0) {
    switch ((unsigned)received) {
    case SCE_NET_ERROR_EWOULDBLOCK:
      break;
    default:
      throw NetException(received);
    }
  }

  return false;
}

void net::send_handshake_response(ClientData &client, uint16_t port,
                                  uint32_t heartbeat_interval) {
  flatbuffers::FlatBufferBuilder builder;
  auto handshake_confirm = NetProtocol::Handshake::CreateHandshake(
      builder, NetProtocol::Handshake::Endpoint::Server, port,
      heartbeat_interval);
  builder.FinishSizePrefixed(handshake_confirm);

  int sent =
      send_all(client.ctrl_fd(), builder.GetBufferPointer(), builder.GetSize());

  if (sent <= 0) {
    throw NetException(sent);
  }
}
