pub use flatbuffers;

#[allow(unused)]
pub mod config {
    include!(concat!(env!("OUT_DIR"), "/config_generated.rs"));
}

#[allow(unused)]
pub mod handshake {
    include!(concat!(env!("OUT_DIR"), "/handshake_generated.rs"));
}

#[allow(unused)]
pub mod pad {
    include!(concat!(env!("OUT_DIR"), "/pad_generated.rs"));
}
