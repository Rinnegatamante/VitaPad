pub mod connection;
pub mod events;
pub mod state;

const HEARTBEAT_MAGIC: &[u8] = &[0xff, 0xff, 0xff, 0xff, 0x42, 0x54];
