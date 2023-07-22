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

void net::handle_ingoing_data(ClientData &client) {
  constexpr size_t buffer_size = 1024;
  typedef bool (ClientData::*buffer_handler)(void);

  uint8_t buffer[buffer_size];
  int received;
  while ((received = sceNetRecv(client.ctrl_fd(), buffer, buffer_size, 0)) >
         0) {
    client.add_to_buffer(buffer, received);
  }

  // TODO: refactor this

  std::vector<buffer_handler> handlers;
  switch (client.state()) {
  case ClientData::State::WaitingForHandshake:
    handlers.insert(handlers.end(), {&ClientData::handle_heartbeat,
                                     &ClientData::handle_handshake});
    break;
  case ClientData::State::WaitingForServerConfirm:
    handlers.insert(handlers.end(), {
                                        &ClientData::handle_heartbeat,
                                    });
    break;
  case ClientData::State::Connected:
    handlers.insert(handlers.end(), {
                                        &ClientData::handle_heartbeat,
                                        &ClientData::handle_handshake,
                                        &ClientData::handle_config,
                                    });
    break;
  default:
    break;
  }

  while (std::any_of(
      std::begin(handlers), std::end(handlers),
      [&](buffer_handler handler) { return std::invoke(handler, client); }))
    ;

  if (received <= 0) {
    switch ((unsigned)received) {
    case SCE_NET_ERROR_EWOULDBLOCK:
      break;
    default:
      throw NetException(received);
    }
  }
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
