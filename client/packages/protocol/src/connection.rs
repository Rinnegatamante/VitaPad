use std::vec::Drain;

use flatbuffers_structs::{
    flatbuffers::{self, FlatBufferBuilder},
    net_protocol::{Handshake, Config, ConfigArgs, HandshakeArgs, Packet, PacketArgs, PacketContent},
};

use crate::events::Event;

pub struct Connection {
    incoming_buffer: Vec<u8>,
    outgoing_buffer: Vec<u8>,
}

impl Connection {
    pub fn new() -> Self {
        Self {
            incoming_buffer: Vec::with_capacity(256),
            outgoing_buffer: Vec::with_capacity(256),
        }
    }

    pub fn events(&mut self) -> ConnectionEvents {
        ConnectionEvents {
            data: &mut self.incoming_buffer,
        }
    }

    pub fn send_handshake(&mut self, handshake_args: HandshakeArgs) {
        let mut builder = FlatBufferBuilder::new();
        let handshake = Handshake::create(&mut builder, &handshake_args);
        let packet = Packet::create(
            &mut builder,
            &PacketArgs {
                content_type: PacketContent::Handshake,
                content: Some(handshake.as_union_value()),
            },
        );
        builder.finish_size_prefixed(packet, None);

        self.outgoing_buffer
            .extend_from_slice(builder.finished_data());
    }

    pub fn send_config(&mut self, config_args: ConfigArgs) {
        let mut builder = FlatBufferBuilder::new();
        let config = Config::create(&mut builder, &config_args);
        let packet = Packet::create(
            &mut builder,
            &PacketArgs {
                content_type: PacketContent::Config,
                content: Some(config.as_union_value()),
            },
        );
        builder.finish_size_prefixed(packet, None);

        self.outgoing_buffer
            .extend_from_slice(builder.finished_data());
    }

    pub fn send_heartbeat(&mut self) {
        self.outgoing_buffer
            .extend_from_slice(crate::HEARTBEAT_MAGIC);
    }

    pub fn receive_data(&mut self, data: &[u8]) {
        self.incoming_buffer.extend_from_slice(data);
    }

    pub fn retrieve_out_data(&mut self) -> Drain<'_, u8> {
        self.outgoing_buffer.drain(..)
    }
}

pub struct ConnectionEvents<'a> {
    data: &'a mut Vec<u8>,
}

#[derive(Debug, Clone, PartialEq, thiserror::Error)]
pub enum ConnectionEventsError {
    #[error("Invalid packet: {0}")]
    InvalidPacket(flatbuffers::InvalidFlatbuffer),
}

impl<'a> Iterator for ConnectionEvents<'a> {
    type Item = Result<Event, ConnectionEventsError>;

    fn next(&mut self) -> Option<Self::Item> {
        const OFFSET_SIZE: usize = std::mem::size_of::<flatbuffers::UOffsetT>();
        const HEARTBEAT_SIZE: usize = crate::HEARTBEAT_MAGIC.len();

        if self.data.len() < OFFSET_SIZE.min(HEARTBEAT_SIZE) {
            return None;
        }

        if self.data.get(..HEARTBEAT_SIZE) == Some(crate::HEARTBEAT_MAGIC) {
            self.data.drain(..HEARTBEAT_SIZE);
            return Some(Ok(Event::HeartbeatReceived));
        }

        let size = flatbuffers::UOffsetT::from_le_bytes(
            self.data.get(..OFFSET_SIZE).unwrap().try_into().unwrap(),
        ) as usize;

        if size == 0 || self.data[OFFSET_SIZE - 1..].len() < size {
            return None;
        }

        let buffer: Vec<_> = self.data.drain(..OFFSET_SIZE + size).collect();
        let packet = match flatbuffers_structs::net_protocol::size_prefixed_root_as_packet(&buffer)
            .map_err(ConnectionEventsError::InvalidPacket)
        {
            Ok(packet) => packet,
            Err(e) => return Some(Err(e)),
        };

        match packet.content_type() {
            PacketContent::Handshake => {
                let handshake = packet.content_as_handshake()?;
                Some(Ok(Event::HandshakeResponseReceived {
                    handshake: handshake.into(),
                }))
            }
            PacketContent::Pad => {
                let pad = packet.content_as_pad()?;
                Some(Ok(Event::PadDataReceived {
                    data: pad.try_into().ok()?,
                }))
            }
            _ => None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::events;
    use flatbuffers_structs::net_protocol::Endpoint;

    fn create_handshake(args: HandshakeArgs) -> Vec<u8> {
        let mut builder = FlatBufferBuilder::new();
        let handshake = Handshake::create(&mut builder, &args);
        let packet = Packet::create(
            &mut builder,
            &PacketArgs {
                content_type: PacketContent::Handshake,
                content: Some(handshake.as_union_value()),
            },
        );
        builder.finish_size_prefixed(packet, None);
        builder.finished_data().to_vec()
    }

    #[test]
    fn test_connection_out_handshake_heartbeat() {
        let mut connection = Connection::new();

        connection.send_handshake(HandshakeArgs {
            endpoint: Endpoint::Client,
            port: 1234,
            heartbeat_freq: 1000,
        });

        connection.send_heartbeat();

        let actual_buffer = connection.retrieve_out_data().collect::<Vec<_>>();

        let expected_handshake = create_handshake(HandshakeArgs {
            endpoint: Endpoint::Client,
            port: 1234,
            heartbeat_freq: 1000,
        });

        assert_eq!(
            actual_buffer,
            [&expected_handshake, crate::HEARTBEAT_MAGIC].concat(),
            "Handshake and heartbeat should be sent"
        );
    }

    #[test]
    fn test_connection_muliple_events_multiple_calls() {
        let mut connection = Connection::new();

        let handshake_client = create_handshake(HandshakeArgs {
            endpoint: Endpoint::Client,
            port: 1234,
            heartbeat_freq: 1000,
        });
        connection.receive_data(&handshake_client);

        {
            let mut events = connection.events();

            assert_eq!(
                events.next(),
                Some(Ok(Event::HandshakeResponseReceived {
                    handshake: events::Handshake {
                        endpoint: Endpoint::Client,
                        port: 1234,
                        heartbeat_freq: 1000,
                    }
                })),
                "HandshakeResponseReceived event should be emitted"
            );
        }

        connection.receive_data(crate::HEARTBEAT_MAGIC);

        let handshake_server = create_handshake(HandshakeArgs {
            endpoint: Endpoint::Server,
            port: 1234,
            heartbeat_freq: 1000,
        });
        connection.receive_data(&handshake_server);

        {
            let mut events = connection.events();

            assert_eq!(
                events.next(),
                Some(Ok(Event::HeartbeatReceived)),
                "HeartbeatReceived event should be emitted"
            );
            assert_eq!(
                events.next(),
                Some(Ok(Event::HandshakeResponseReceived {
                    handshake: events::Handshake {
                        endpoint: Endpoint::Server,
                        port: 1234,
                        heartbeat_freq: 1000,
                    }
                })),
                "HandshakeResponseReceived event should be emitted"
            );
            assert_eq!(events.next(), None, "No more events should be emitted");
        }
    }

    #[test]
    fn test_connection_multiple_events_single_call() {
        let mut connection = Connection::new();

        let handshake_client = create_handshake(HandshakeArgs {
            endpoint: Endpoint::Client,
            port: 1234,
            heartbeat_freq: 1000,
        });

        let handshake_server = create_handshake(HandshakeArgs {
            endpoint: Endpoint::Server,
            port: 1234,
            heartbeat_freq: 1000,
        });

        connection
            .receive_data(&[&handshake_client, crate::HEARTBEAT_MAGIC, &handshake_server].concat());

        let mut events = connection.events();

        assert_eq!(
            events.next(),
            Some(Ok(Event::HandshakeResponseReceived {
                handshake: events::Handshake {
                    endpoint: Endpoint::Client,
                    port: 1234,
                    heartbeat_freq: 1000,
                }
            })),
            "HandshakeResponseReceived event should be emitted"
        );

        assert_eq!(
            events.next(),
            Some(Ok(Event::HeartbeatReceived)),
            "HeartbeatReceived event should be emitted"
        );

        assert_eq!(
            events.next(),
            Some(Ok(Event::HandshakeResponseReceived {
                handshake: events::Handshake {
                    endpoint: Endpoint::Server,
                    port: 1234,
                    heartbeat_freq: 1000,
                }
            })),
            "HandshakeResponseReceived event should be emitted"
        );

        assert_eq!(events.next(), None, "No more events should be emitted");
    }
}
