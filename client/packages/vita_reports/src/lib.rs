#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ButtonsData {
    pub select: bool,
    pub start: bool,
    pub up: bool,
    pub right: bool,
    pub down: bool,
    pub left: bool,
    pub lt: bool,
    pub rt: bool,
    pub triangle: bool,
    pub circle: bool,
    pub cross: bool,
    pub square: bool,
    // timestamp: u64;
}

impl From<flatbuffers_structs::pad::pad::ButtonsData> for ButtonsData {
    fn from(buttons: flatbuffers_structs::pad::pad::ButtonsData) -> Self {
        Self {
            select: buttons.select(),
            start: buttons.start(),
            up: buttons.up(),
            right: buttons.right(),
            down: buttons.down(),
            left: buttons.left(),
            lt: buttons.lt(),
            rt: buttons.rt(),
            triangle: buttons.triangle(),
            circle: buttons.circle(),
            cross: buttons.cross(),
            square: buttons.square(),
            // timestamp: buttons.timestamp(),
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct Vector3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

impl From<&flatbuffers_structs::pad::pad::Vector3> for Vector3 {
    fn from(vector: &flatbuffers_structs::pad::pad::Vector3) -> Self {
        Self {
            x: vector.x(),
            y: vector.y(),
            z: vector.z(),
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct MotionData {
    pub gyro: Vector3,
    pub accelerometer: Vector3,
    // timestamp: u64,
}

impl From<flatbuffers_structs::pad::pad::MotionData> for MotionData {
    fn from(motion: flatbuffers_structs::pad::pad::MotionData) -> Self {
        Self {
            gyro: motion.gyro().into(),
            accelerometer: motion.accelerometer().into(),
            // timestamp: motion.timestamp(),
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct TouchReport {
    pub x: i16,
    pub y: i16,
    pub id: u8,
    pub force: u8,
}

impl From<flatbuffers_structs::pad::pad::TouchReport> for TouchReport {
    fn from(touch: flatbuffers_structs::pad::pad::TouchReport) -> Self {
        Self {
            x: touch.x(),
            y: touch.y(),
            id: touch.id(),
            force: touch.pressure(),
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct TouchData {
    pub reports: Vec<TouchReport>,
}

impl<'a> From<flatbuffers_structs::pad::pad::TouchData<'a>> for TouchData {
    fn from(touch: flatbuffers_structs::pad::pad::TouchData) -> Self {
        Self {
            reports: touch
                .reports()
                .unwrap_or_default()
                .iter()
                .cloned()
                .map(Into::into)
                .collect(),
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct MainReport {
    pub buttons: ButtonsData,
    pub lx: u8,
    pub ly: u8,
    pub rx: u8,
    pub ry: u8,
    pub front_touch: TouchData,
    pub back_touch: TouchData,
    pub motion: MotionData,
    pub timestamp: u64,
}

// TODO: Fallible conversion?
impl<'a> From<flatbuffers_structs::pad::pad::MainPacket<'a>> for MainReport {
    fn from(packet: flatbuffers_structs::pad::pad::MainPacket) -> Self {
        let buttons = *packet.buttons().expect("Buttons data is missing");
        let front_touch = packet.front_touch().expect("Front touch data is missing");
        let back_touch = packet.back_touch().expect("Back touch data is missing");
        let motion = *packet.motion().expect("Motion data is missing");

        Self {
            buttons: buttons.into(),
            front_touch: front_touch.into(),
            back_touch: back_touch.into(),
            motion: motion.into(),
            timestamp: packet.timestamp(),
            lx: packet.lx(),
            ly: packet.ly(),
            rx: packet.rx(),
            ry: packet.ry(),
        }
    }
}
