use flatbuffers_structs::net_protocol::Endpoint;
use vita_reports::MainReport;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Handshake {
    pub endpoint: Endpoint,
    pub port: u16,
    pub heartbeat_freq: u32,
}

impl<'a> From<flatbuffers_structs::net_protocol::Handshake<'a>> for Handshake {
    fn from(handshake: flatbuffers_structs::net_protocol::Handshake) -> Self {
        Self {
            endpoint: handshake.endpoint(),
            port: handshake.port(),
            heartbeat_freq: handshake.heartbeat_freq(),
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum Event {
    HandshakeRequestSent,
    HandshakeResponseReceived { handshake: Handshake },
    HeartbeatSent,
    HeartbeatReceived,
    PadDataSent,
    PadDataReceived { data: MainReport },
}
