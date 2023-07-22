# VitaPad

<center>
<img src="./server/sce_sys/icon0.png" width="128" height="128" />
<p>Turn your PS Vita into a gamepad for your PC!</p>
</center>


## Installation

The server has to be installed on the PS Vita and the client on the PC.

### Requirements

- [VitaSDK](https://vitasdk.org/)
- [CMake](https://cmake.org/)

#### Windows

- [ViGEmBus](https://github.com/ViGEm/ViGEmBus/releases)

### Server

```bash
mkdir -p build
cd build
cmake -G Ninja ../server
ninja VitaPad.vpk
```

Then, install the generated `VitaPad.vpk` file on your PS Vita.


### Client

```bash
cd client
cargo build --release --bin cli
```

Then, run the generated executable (target/release/cli{.exe}) on your PC.