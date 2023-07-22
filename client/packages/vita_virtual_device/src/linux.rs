use std::{
    ffi::OsString,
    fs::{File, OpenOptions},
    os::fd::AsRawFd,
};

use input_linux::{
    sys::{input_event, BUS_VIRTUAL},
    AbsoluteAxis, AbsoluteEvent, AbsoluteInfo, AbsoluteInfoSetup, EventKind, EventTime, InputEvent,
    InputId, InputProperty, Key, KeyEvent, KeyState, SynchronizeEvent, UInputHandle,
};

use crate::VitaVirtualDevice;

type TrackingId = u8;

pub struct VitaDevice<F: AsRawFd> {
    main_handle: UInputHandle<F>,
    sensor_handle: UInputHandle<F>,
    previous_front_touches: [Option<TrackingId>; 6],
    previous_back_touches: [Option<TrackingId>; 4],
}

#[derive(thiserror::Error, Debug)]
#[non_exhaustive]
pub enum Error {
    #[error("Failed to create uinput device")]
    DeviceCreationFailed(#[source] std::io::Error),
    #[error("Failed to write uinput device event")]
    WriteEventFailed(#[source] std::io::Error),
}

impl<F: AsRawFd> VitaDevice<F> {
    pub fn new(uinput_file: F, uinput_sensor_file: F) -> std::io::Result<Self> {
        let main_handle = UInputHandle::new(uinput_file);

        main_handle.set_evbit(EventKind::Key)?;
        main_handle.set_keybit(Key::ButtonSouth)?;
        main_handle.set_keybit(Key::ButtonEast)?;
        main_handle.set_keybit(Key::ButtonNorth)?;
        main_handle.set_keybit(Key::ButtonWest)?;
        main_handle.set_keybit(Key::ButtonTL)?;
        main_handle.set_keybit(Key::ButtonTR)?;
        main_handle.set_keybit(Key::ButtonStart)?;
        main_handle.set_keybit(Key::ButtonSelect)?;
        main_handle.set_keybit(Key::ButtonDpadUp)?;
        main_handle.set_keybit(Key::ButtonDpadDown)?;
        main_handle.set_keybit(Key::ButtonDpadLeft)?;
        main_handle.set_keybit(Key::ButtonDpadRight)?;

        main_handle.set_evbit(EventKind::Absolute)?;

        let joystick_abs_info = AbsoluteInfo {
            flat: 128,
            fuzz: 0, // Already fuzzed
            maximum: 255,
            minimum: 0,
            resolution: 255,
            ..Default::default()
        };

        let joystick_x_info = AbsoluteInfoSetup {
            info: joystick_abs_info,
            axis: AbsoluteAxis::X,
        };
        let joystick_y_info = AbsoluteInfoSetup {
            info: joystick_abs_info,
            axis: AbsoluteAxis::Y,
        };
        let joystick_rx_info = AbsoluteInfoSetup {
            info: joystick_abs_info,
            axis: AbsoluteAxis::RX,
        };
        let joystick_ry_info = AbsoluteInfoSetup {
            info: joystick_abs_info,
            axis: AbsoluteAxis::RY,
        };

        // Touchscreen (front)
        let front_mt_x_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 0,
                maximum: 1919,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchPositionX,
        };
        let front_mt_y_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 0,
                maximum: 1087,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchPositionY,
        };
        let front_mt_id_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 0,
                maximum: 128,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchTrackingId,
        }; //TODO: Query infos
        let front_mt_slot_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 0,
                maximum: 5,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchSlot,
        }; // According to vitasdk docs
        let front_mt_pressure_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 1,
                maximum: 128,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchPressure,
        }; //TODO: Query infos

        let id = InputId {
            bustype: BUS_VIRTUAL,
            vendor: 0x54c,
            product: 0x2d2,
            version: 2,
        };

        main_handle.create(
            &id,
            b"PS Vita",
            0,
            &[
                joystick_x_info,
                joystick_y_info,
                joystick_rx_info,
                joystick_ry_info,
                front_mt_x_info,
                front_mt_y_info,
                front_mt_id_info,
                front_mt_slot_info,
                front_mt_pressure_info,
            ],
        )?;

        // Have to create another device because sensors can't be mixed with directional axes
        // and we can't assign the back touch surface along with the touchscreen.
        // So this second device contains info for the motion sensors and the back touch surface.
        let sensor_handle = UInputHandle::new(uinput_sensor_file);

        sensor_handle.set_evbit(EventKind::Absolute)?;
        sensor_handle.set_propbit(InputProperty::Accelerometer)?;

        let accel_abs_info = AbsoluteInfo {
            minimum: -16,
            maximum: 16,
            ..Default::default()
        };
        let accel_x_info = AbsoluteInfoSetup {
            info: accel_abs_info,
            axis: AbsoluteAxis::X,
        };
        let accel_y_info = AbsoluteInfoSetup {
            info: accel_abs_info,
            axis: AbsoluteAxis::Y,
        };
        let accel_z_info = AbsoluteInfoSetup {
            info: accel_abs_info,
            axis: AbsoluteAxis::Z,
        };

        let gyro_abs_info = AbsoluteInfo {
            minimum: -1,
            maximum: 1,
            ..Default::default()
        };
        let gyro_x_info = AbsoluteInfoSetup {
            info: gyro_abs_info,
            axis: AbsoluteAxis::RX,
        };
        let gyro_y_info = AbsoluteInfoSetup {
            info: gyro_abs_info,
            axis: AbsoluteAxis::RY,
        };
        let gyro_z_info = AbsoluteInfoSetup {
            info: gyro_abs_info,
            axis: AbsoluteAxis::RZ,
        };

        let mt_x_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 0,
                maximum: 1919,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchPositionX,
        };
        let mt_y_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 0,
                maximum: 1087,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchPositionY,
        };
        let mt_id_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 0,
                maximum: 128,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchTrackingId,
        };
        let mt_slot_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 0,
                maximum: 3,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchSlot,
        };
        let mt_pressure_info = AbsoluteInfoSetup {
            info: AbsoluteInfo {
                minimum: 1,
                maximum: 128,
                ..Default::default()
            },
            axis: AbsoluteAxis::MultitouchPressure,
        };

        let id = InputId {
            bustype: BUS_VIRTUAL,
            vendor: 0x54c,
            product: 0x2d3,
            version: 2,
        };

        sensor_handle.create(
            &id,
            b"PS Vita (Sensors)",
            0,
            &[
                accel_x_info,
                accel_y_info,
                accel_z_info,
                gyro_x_info,
                gyro_y_info,
                gyro_z_info,
                mt_x_info,
                mt_y_info,
                mt_id_info,
                mt_slot_info,
                mt_pressure_info,
            ],
        )?;

        Ok(VitaDevice {
            main_handle,
            sensor_handle,
            previous_front_touches: [None; 6],
            previous_back_touches: [None; 4],
        })
    }
}

impl VitaVirtualDevice for VitaDevice<File> {
    fn create() -> crate::Result<Self> {
        let uinput_file = OpenOptions::new()
            .read(true)
            .write(true)
            .open("/dev/uinput")
            .map_err(Error::DeviceCreationFailed)?;

        let uinput_sensor_file = OpenOptions::new()
            .read(true)
            .write(true)
            .open("/dev/uinput")
            .map_err(Error::DeviceCreationFailed)?;

        let device = VitaDevice::new(uinput_file, uinput_sensor_file)
            .map_err(Error::DeviceCreationFailed)?;

        Ok(device)
    }

    fn identifiers(&self) -> Option<Vec<OsString>> {
        self.main_handle
            .evdev_name()
            .ok()
            .zip(self.sensor_handle.evdev_name().ok())
            .map(|(main, sensor)| [main, sensor].to_vec())
    }

    fn send_report(&mut self, report: vita_reports::MainReport) -> crate::Result<()> {
        const EVENT_TIME_ZERO: EventTime = EventTime::new(0, 0);
        let syn_event = *SynchronizeEvent::report(EVENT_TIME_ZERO)
            .as_event()
            .as_raw();

        macro_rules! key_event {
            ($report:ident, $report_name:ident, $uinput_name:ident) => {
                KeyEvent::new(
                    EVENT_TIME_ZERO,
                    Key::$uinput_name,
                    KeyState::pressed($report.buttons.$report_name),
                )
            };
        }

        macro_rules! stick_event {
            ($report:ident, $report_name:ident, $uinput_name:ident) => {
                AbsoluteEvent::new(
                    EVENT_TIME_ZERO,
                    AbsoluteAxis::$uinput_name,
                    $report.$report_name.into(),
                )
            };
        }

        macro_rules! mt_event {
            ($report:ident, $report_name:ident, $uinput_name:ident) => {
                AbsoluteEvent::new(
                    EVENT_TIME_ZERO,
                    AbsoluteAxis::$uinput_name,
                    $report.$report_name.into(),
                )
            };
        }

        macro_rules! accel_event {
            ($report:ident, $report_name:ident, $uinput_name:ident) => {
                AbsoluteEvent::new(
                    EVENT_TIME_ZERO,
                    AbsoluteAxis::$uinput_name,
                    $report.motion.accelerometer.$report_name.round() as i32,
                )
            };
        }

        macro_rules! gyro_event {
            ($report:ident, $report_name:ident, $uinput_name:ident) => {
                AbsoluteEvent::new(
                    EVENT_TIME_ZERO,
                    AbsoluteAxis::$uinput_name,
                    $report.motion.gyro.$report_name.round() as i32,
                )
            };
        }

        // Main device events

        let buttons_events: &[InputEvent] = &[
            key_event!(report, triangle, ButtonNorth),
            key_event!(report, circle, ButtonEast),
            key_event!(report, cross, ButtonSouth),
            key_event!(report, square, ButtonWest),
            key_event!(report, lt, ButtonTL),
            key_event!(report, rt, ButtonTR),
            key_event!(report, select, ButtonSelect),
            key_event!(report, start, ButtonStart),
            key_event!(report, up, ButtonDpadUp),
            key_event!(report, right, ButtonDpadRight),
            key_event!(report, down, ButtonDpadDown),
            key_event!(report, left, ButtonDpadLeft),
        ]
        .map(|ev| ev.into());

        let sticks_events = &[
            stick_event!(report, lx, X),
            stick_event!(report, ly, Y),
            stick_event!(report, rx, RX),
            stick_event!(report, ry, RY),
        ]
        .map(|ev| ev.into());

        let front_touch_resets_events = self
            .previous_front_touches
            .iter()
            .enumerate()
            .filter_map(|(slot, id)| {
                let new_id = report.front_touch.reports.get(slot).map(|r| r.id);

                match (*id, new_id) {
                    (Some(_), None) => Some([
                        AbsoluteEvent::new(
                            EVENT_TIME_ZERO,
                            AbsoluteAxis::MultitouchSlot,
                            slot as i32,
                        ),
                        AbsoluteEvent::new(EVENT_TIME_ZERO, AbsoluteAxis::MultitouchTrackingId, -1),
                    ]),
                    _ => None,
                }
            })
            .flatten()
            .map(|ev| ev.into())
            .collect::<Vec<InputEvent>>();

        self.previous_front_touches = report
            .front_touch
            .reports
            .iter()
            .map(|report| Some(report.id))
            .chain(
                std::iter::repeat(None)
                    .take(self.previous_front_touches.len() - report.front_touch.reports.len()),
            )
            .collect::<Vec<Option<u8>>>()
            .try_into()
            .unwrap();

        let front_touch_events: Vec<_> = report
            .front_touch
            .reports
            .into_iter()
            .enumerate()
            .map(|(slot, report)| {
                [
                    AbsoluteEvent::new(EVENT_TIME_ZERO, AbsoluteAxis::MultitouchSlot, slot as i32),
                    mt_event!(report, x, MultitouchPositionX),
                    mt_event!(report, y, MultitouchPositionY),
                    mt_event!(report, id, MultitouchTrackingId),
                    mt_event!(report, force, MultitouchPressure),
                ]
                .map(|event| event.into())
            })
            .flatten()
            .collect::<Vec<InputEvent>>();

        let events: Vec<input_event> = [
            buttons_events,
            sticks_events,
            &front_touch_resets_events,
            &front_touch_events,
        ]
        .concat()
        .into_iter()
        .map(|ev| ev.into())
        .map(|ev: InputEvent| *ev.as_raw())
        .collect();

        self.main_handle
            .write(&events)
            .map_err(Error::WriteEventFailed)?;
        self.main_handle
            .write(&[syn_event])
            .map_err(Error::WriteEventFailed)?;

        // Sensors device events

        let motion_events: &[InputEvent] = &[
            accel_event!(report, x, X),
            accel_event!(report, y, Y),
            accel_event!(report, z, Z),
            gyro_event!(report, x, RX),
            gyro_event!(report, y, RY),
            gyro_event!(report, z, RZ),
        ]
        .map(|ev| ev.into());

        let back_touch_resets_events = self
            .previous_back_touches
            .iter()
            .enumerate()
            .filter_map(|(slot, id)| {
                let new_id = report.back_touch.reports.get(slot).map(|r| r.id);

                match (*id, new_id) {
                    (Some(_), None) => Some([
                        AbsoluteEvent::new(
                            EVENT_TIME_ZERO,
                            AbsoluteAxis::MultitouchSlot,
                            slot as i32,
                        ),
                        AbsoluteEvent::new(EVENT_TIME_ZERO, AbsoluteAxis::MultitouchTrackingId, -1),
                    ]),
                    _ => None,
                }
            })
            .flatten()
            .map(|ev| ev.into())
            .collect::<Vec<InputEvent>>();

        self.previous_back_touches = report
            .back_touch
            .reports
            .iter()
            .map(|report| Some(report.id))
            .chain(
                std::iter::repeat(None)
                    .take(self.previous_back_touches.len() - report.back_touch.reports.len()),
            )
            .collect::<Vec<Option<u8>>>()
            .try_into()
            .unwrap();

        let back_touch_events: Vec<_> = report
            .back_touch
            .reports
            .into_iter()
            .enumerate()
            .map(|(slot, report)| {
                [
                    AbsoluteEvent::new(EVENT_TIME_ZERO, AbsoluteAxis::MultitouchSlot, slot as i32),
                    mt_event!(report, x, MultitouchPositionX),
                    mt_event!(report, y, MultitouchPositionY),
                    mt_event!(report, id, MultitouchTrackingId),
                    mt_event!(report, force, MultitouchPressure),
                ]
                .map(|event| event.into())
            })
            .flatten()
            .collect::<Vec<InputEvent>>();

        let events: Vec<input_event> =
            [motion_events, &back_touch_resets_events, &back_touch_events]
                .concat()
                .into_iter()
                .map(|ev| ev.into())
                .map(|ev: InputEvent| *ev.as_raw())
                .collect();

        self.sensor_handle
            .write(&events)
            .map_err(Error::WriteEventFailed)?;
        self.sensor_handle
            .write(&[syn_event])
            .map_err(Error::WriteEventFailed)?;

        Ok(())
    }
}
