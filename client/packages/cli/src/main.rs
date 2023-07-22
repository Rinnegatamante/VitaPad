use std::{
    ffi::OsStr,
    io::{Read, Write},
    net::{IpAddr, Ipv4Addr, SocketAddr, SocketAddrV4, TcpStream, UdpSocket},
    time::{Duration, SystemTime},
};

use argh::FromArgs;
use color_eyre::eyre::WrapErr;

use flatbuffers_structs::handshake::net_protocol::handshake::{Endpoint, HandshakeArgs};
use protocol::connection::Connection;
use vita_virtual_device::{VitaDevice, VitaVirtualDevice};

/// Create a virtual controller and fetch its data from a Vita
/// over the network.
#[derive(FromArgs)]
struct Args {
    #[argh(option, short = 'p')]
    /// port to connect to
    /// (default: 5000)
    port: Option<u16>,
    #[argh(option)]
    /// polling rate
    polling_rate: Option<u64>,
    /// IP address of the Vita to connect to
    #[argh(positional)]
    ip: String,
}

fn filter_udp_nonblocking_error(
    err: std::io::Error,
) -> Result<(usize, SocketAddr), std::io::Error> {
    if err.kind() == std::io::ErrorKind::WouldBlock {
        Ok((0, SocketAddr::new(IpAddr::V4(Ipv4Addr::UNSPECIFIED), 0)))
    } else {
        Err(err)
    }
}

fn main() -> color_eyre::Result<()> {
    const NET_PORT: u16 = 5000;
    const TIMEOUT: Duration = Duration::from_secs(25);
    const BUFFER_SIZE: usize = 2048;
    const POLLING_RATE: u64 = 1 * 1000 / 250;

    color_eyre::install()?;
    pretty_env_logger::init();

    let args: Args = argh::from_env();
    let remote_port = args.port.unwrap_or(NET_PORT);
    let polling_rate = args.polling_rate.unwrap_or(POLLING_RATE);

    let addr = SocketAddr::V4(SocketAddrV4::new(
        args.ip.parse().wrap_err("invalid IPv4 address")?,
        remote_port,
    ));
    let mut conn = Connection::new();

    let mut ctrl_socket = TcpStream::connect_timeout(&addr, TIMEOUT).wrap_err(
        "Failed to connect to device, please check that the IP address and port are correct",
    )?;

    let pad_socket =
        UdpSocket::bind((Ipv4Addr::UNSPECIFIED, 0)).wrap_err("Failed to bind UDP socket")?;

    pad_socket
        .set_nonblocking(true)
        .wrap_err("Failed to set non-blocking mode on socket")?;

    let bound_port = pad_socket
        .local_addr()
        .expect("Failed to get connection info for data socket")
        .port();

    conn.send_handshake(HandshakeArgs {
        endpoint: Endpoint::Client,
        port: bound_port,
        ..Default::default()
    });

    ctrl_socket
        .write_all(conn.retrieve_out_data().as_slice())
        .wrap_err("Failed to send handshake to Vita")?;

    log::info!("Handshake sent to Vita");

    log::info!("Waiting for handshake response from Vita");

    let mut buf = [0; BUFFER_SIZE];

    ctrl_socket
        .read(&mut buf)
        .wrap_err("Failed to read handshake response from Vita")?;

    log::info!("Handshake response received from Vita");

    conn.receive_data(&buf);
    let event = conn
        .events()
        .next()
        .expect("No handshake response received");
    let handshake_response = match event {
        protocol::events::Event::HandshakeResponseReceived { handshake } => handshake,
        _ => unimplemented!("Unexpected event received"),
    };
    let heartbeat_freq = handshake_response.heartbeat_freq;
    log::debug!("Heartbeat frequency: {}", heartbeat_freq);

    conn.send_heartbeat();
    pad_socket
        .send_to(conn.retrieve_out_data().as_slice(), addr)
        .wrap_err("Failed to send heartbeat to Vita")?;

    log::info!("Opened port for data on {}", bound_port);

    let mut last_time = SystemTime::now();

    let mut device = VitaDevice::create().wrap_err(
        "Failed to create virtual device, \
        please check that you have permissions on uinput device",
    )?;

    println!(
        "Device identifiers: {}",
        device
            .identifiers()
            .expect("No identifier found")
            .join(OsStr::new(", "))
            .to_string_lossy()
    );
    println!("Connection established, press Ctrl+C to exit");

    loop {
        std::thread::sleep(Duration::from_millis(polling_rate));
        log::trace!("Polling");

        if last_time
            .elapsed()
            .expect("Cannot get elapsed time")
            .as_secs()
            >= (heartbeat_freq - 5).into()
        {
            log::debug!("Sending heartbeat to Vita");
            conn.send_heartbeat();
            ctrl_socket
                .write_all(conn.retrieve_out_data().as_slice())
                .wrap_err("Failed to send heartbeat to Vita")?;
            log::debug!("Heartbeat sent to Vita");
            last_time = SystemTime::now();
            log::trace!("Last time updated to {last_time:?}");
        }

        let (len, _) = pad_socket
            .recv_from(&mut buf)
            .or_else(filter_udp_nonblocking_error)
            .wrap_err("Failed to receive data from Vita")?;
        log::debug!("Received {len} bytes from Vita");

        if len == 0 {
            continue;
        }

        let received_data = &buf[..len];

        log::trace!("Received {len} bytes from Vita: {received_data:?}");

        conn.receive_data(received_data);

        for event in conn.events() {
            log::debug!("Event received: {event:?}");
            if let protocol::events::Event::PadDataReceived { data } = event {
                let report = vita_reports::MainReport::from(data);
                log::trace!("Sending report to virtual device: {report:?}");
                device
                    .send_report(report)
                    .wrap_err("Failed to send report to virtual device")?;
            }
        }
    }
}
