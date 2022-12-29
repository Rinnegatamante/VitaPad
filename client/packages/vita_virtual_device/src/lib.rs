use std::ffi::OsString;

#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[cfg(target_os = "linux")]
    #[error(transparent)]
    Linux(#[from] linux::Error),
}

type Result<T> = std::result::Result<T, Error>;

/// A trait for creating and using a virtual device.
pub trait VitaVirtualDevice: Sized {
    fn create() -> Result<Self>;
    fn identifiers(&self) -> Option<Vec<OsString>>;
    fn send_report(&mut self, report: vita_reports::MainReport) -> Result<()>;
}

#[cfg(target_os = "linux")]
mod linux;

#[cfg(target_os = "linux")]
pub use linux::VitaDevice;
