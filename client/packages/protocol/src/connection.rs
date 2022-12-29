use std::vec::Drain;

use flatbuffers_structs::{
    flatbuffers::{self, FlatBufferBuilder},
    handshake::net_protocol::handshake::{self, Handshake, HandshakeArgs},
    pad::pad,
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
        builder.finish_size_prefixed(handshake, None);

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

impl<'a> Iterator for ConnectionEvents<'a> {
    type Item = Event;

    fn next(&mut self) -> Option<Self::Item> {
        const OFFSET_SIZE: usize = std::mem::size_of::<flatbuffers::UOffsetT>();
        const HEARTBEAT_SIZE: usize = crate::HEARTBEAT_MAGIC.len();

        if self.data.len() < OFFSET_SIZE.min(HEARTBEAT_SIZE) {
            return None;
        }

        if self.data.get(..HEARTBEAT_SIZE) == Some(crate::HEARTBEAT_MAGIC) {
            self.data.drain(..HEARTBEAT_SIZE);
            return Some(Event::HeartbeatReceived);
        }

        let size = flatbuffers::UOffsetT::from_le_bytes(
            self.data.get(..OFFSET_SIZE).unwrap().try_into().unwrap(),
        ) as usize;

        if self.data[OFFSET_SIZE..].len() < size {
            return None;
        }

        let buffer: Vec<_> = self.data.drain(..OFFSET_SIZE + size).collect();

        pad::size_prefixed_root_as_main_packet(&buffer)
            .map(|data| Event::PadDataReceived { data: data.into() })
            .or_else(|_| {
                handshake::size_prefixed_root_as_handshake(&buffer).map(|handshake| {
                    Event::HandshakeResponseReceived {
                        handshake: handshake.into(),
                    }
                })
            })
            .ok()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::events;

    fn create_handshake(args: HandshakeArgs) -> Vec<u8> {
        let mut builder = FlatBufferBuilder::new();
        let handshake = Handshake::create(&mut builder, &args);
        builder.finish_size_prefixed(handshake, None);
        builder.finished_data().to_vec()
    }

    #[test]
    fn test_connection_out_handshake_heartbeat() {
        let mut connection = Connection::new();

        connection.send_handshake(HandshakeArgs {
            endpoint: handshake::Endpoint::Client,
            port: 1234,
            heartbeat_freq: 1000,
        });

        connection.send_heartbeat();

        let actual_buffer = connection.retrieve_out_data().collect::<Vec<_>>();

        let expected_handshake = create_handshake(HandshakeArgs {
            endpoint: handshake::Endpoint::Client,
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
            endpoint: handshake::Endpoint::Client,
            port: 1234,
            heartbeat_freq: 1000,
        });
        connection.receive_data(&handshake_client);

        {
            let mut events = connection.events();

            assert_eq!(
                events.next(),
                Some(Event::HandshakeResponseReceived {
                    handshake: events::Handshake {
                        endpoint: handshake::Endpoint::Client,
                        port: 1234,
                        heartbeat_freq: 1000,
                    }
                }),
                "HandshakeResponseReceived event should be emitted"
            );
        }

        connection.receive_data(crate::HEARTBEAT_MAGIC);

        let handshake_server = create_handshake(HandshakeArgs {
            endpoint: handshake::Endpoint::Server,
            port: 1234,
            heartbeat_freq: 1000,
        });
        connection.receive_data(&handshake_server);

        {
            let mut events = connection.events();

            assert_eq!(
                events.next(),
                Some(Event::HeartbeatReceived),
                "HeartbeatReceived event should be emitted"
            );
            assert_eq!(
                events.next(),
                Some(Event::HandshakeResponseReceived {
                    handshake: events::Handshake {
                        endpoint: handshake::Endpoint::Server,
                        port: 1234,
                        heartbeat_freq: 1000,
                    }
                }),
                "HandshakeResponseReceived event should be emitted"
            );
            assert_eq!(events.next(), None, "No more events should be emitted");
        }
    }

    #[test]
    fn test_connection_multiple_events_single_call() {
        let mut connection = Connection::new();

        let handshake_client = create_handshake(HandshakeArgs {
            endpoint: handshake::Endpoint::Client,
            port: 1234,
            heartbeat_freq: 1000,
        });

        let handshake_server = create_handshake(HandshakeArgs {
            endpoint: handshake::Endpoint::Server,
            port: 1234,
            heartbeat_freq: 1000,
        });

        connection
            .receive_data(&[&handshake_client, crate::HEARTBEAT_MAGIC, &handshake_server].concat());

        let mut events = connection.events();

        assert_eq!(
            events.next(),
            Some(Event::HandshakeResponseReceived {
                handshake: events::Handshake {
                    endpoint: handshake::Endpoint::Client,
                    port: 1234,
                    heartbeat_freq: 1000,
                }
            }),
            "HandshakeResponseReceived event should be emitted"
        );

        assert_eq!(
            events.next(),
            Some(Event::HeartbeatReceived),
            "HeartbeatReceived event should be emitted"
        );

        assert_eq!(
            events.next(),
            Some(Event::HandshakeResponseReceived {
                handshake: events::Handshake {
                    endpoint: handshake::Endpoint::Server,
                    port: 1234,
                    heartbeat_freq: 1000,
                }
            }),
            "HandshakeResponseReceived event should be emitted"
        );

        assert_eq!(events.next(), None, "No more events should be emitted");
    }
}
