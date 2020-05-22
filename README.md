# VITAPAD


VitaPad allows you to use your PSVITA as a wireless PC controller. It supports Windows (both 32 and 64 bit) and Linux (port made by nyorem). In the future i'm planning to add also Mac Os X support and also add a proper fiddler for vJoy.

## Usage

* Install VPK file on PSVITA
* Open VitaPad on PSVITA
* Optional: [Install vJoy driver](https://github.com/njz3/vJoy/releases/download/v2.2.0.0/vJoySetup.2.2.0.signed.exe) on Windows PC for vJoy functionality. Need to set `VJOY_MODE` to 1 in windows.xml.
* Optional: [Install ViGEm driver](https://github.com/ViGEm/ViGEmBus/releases/download/setup-v1.16.116/ViGEmBus_Setup_1.16.116.exe) on Windows PC for DualShock 4 emulation. Need to set `VIGEM_MODE` to 1 in windows.xml.
* Open VitaPad on PC
* Insert the IP showed on PSVITA on PC

## Controls Mapping

You can edit your controls mapping by editing the XML file inside the client folder (windows.xml / linux.xml)

Default mapping:

- DPAD = WASD
- Cross,Square,Triangle,Circle = IJKL
- L Trigger = Ctrl
- R Trigger = Spacebar
- Start = Enter
- Select = Shift
- Left Analog = DKeys
- Right Analog = 8,6,4,2
- Touchscreen = Mouse movement
- Retrotouch = Left/Right click

Default ViGEm mapping:

- PS button unmapped
- Select = Share
- Start = Option
- Left Front Touchscreen = L1
- Right Front Touchscreen = R1
- Left shoulder = L2
- Right shoulder = R2
- Rest of buttons are mapped to the expected buttons on DualShock 4

## Footage

You can see the VitaPad in action thanks to this footage from koog k:

[![VitaPad v.1.0 - PPSSPP - PSVITA wireless PC controller -](http://img.youtube.com/vi/CQ5wUMOpXoM/0.jpg)](http://www.youtube.com/watch?v=CQ5wUMOpXoM "VitaPad v.1.0 - PPSSPP - PSVITA wireless PC controller -")
