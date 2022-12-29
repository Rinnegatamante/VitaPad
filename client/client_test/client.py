from socket import socket, AF_INET, SOCK_STREAM, SOCK_DGRAM
from time import time
import flatbuffers
from NetProtocol.Handshake import Handshake, Endpoint
from Pad import MainPacket, Vector3

sock = socket(AF_INET, SOCK_STREAM)

sock.connect(("192.168.1.24", 5000))

print("Connected")

udp_sock = socket(AF_INET, SOCK_DGRAM)

udp_sock.bind(("0.0.0.0", 0))
_, port = udp_sock.getsockname()

builder = flatbuffers.Builder()
Handshake.HandshakeStart(builder)
Handshake.HandshakeAddEndpoint(builder, Endpoint.Endpoint.Client)
Handshake.HandshakeAddPort(builder, port)
Handshake.HandshakeAddHeartbeatFreq(builder, 0)
handshake = Handshake.HandshakeEnd(builder)
builder.FinishSizePrefixed(handshake)

sock.sendall(builder.Output())

response = sock.recv(1024)

buf, prefix = flatbuffers.util.RemoveSizePrefix(response, 0)
handshake = Handshake.Handshake.GetRootAs(buf[prefix:])
delay = handshake.HeartbeatFreq()

magic = bytes([0x45, 0x4e, 0x44, 0x48, 0x42, 0x54])

udp_sock.sendto(magic, ("192.168.1.24", 5000))

last_sent = int(time())

while True:
    if int(time()) - last_sent >= delay - (delay // 2):
        sock.sendall(magic)
        last_sent = int(time())

    data, addr = udp_sock.recvfrom(1024)
    buf, prefix = flatbuffers.util.RemoveSizePrefix(data, 0)
    main = MainPacket.MainPacket.GetRootAs(buf[prefix:])
    vec = Vector3.Vector3()
    print(main.Motion().Accelerometer(vec).X())
