pub mod connection;
pub mod events;
pub mod state;

const HEARTBEAT_MAGIC: &[u8] = &[0x45, 0x4e, 0x44, 0x48, 0x42, 0x54];
