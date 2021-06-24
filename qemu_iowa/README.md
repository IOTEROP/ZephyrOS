# Zephyr IOWA Client

Sample IOWA QEMU client for Zephyr (https://docs.zephyrproject.org/latest/getting_started/index.html)

OS: Zephyr / Qemu  (tested on zephyr-sdk 0.12.4-x86_64)

** this setup runs on WSL2/Ubuntu 20.04 **

## Setup 
Install & configure Zephyr OS build system as specified here:

[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html)

Copy the source code of this sample on your dev folder:

`git clone --recurse-submodules https://github.com/IOTEROP/IOWA.git`

(!: _--recurse-submodules_ retrieves the iowa directory)

Compile and run the sample code:
```
west build -b qemu_x86_64 -c zephyr-iowa-client && west build -t run
```

## Networking
To see your device sending information on a non-local server (like Ioterop CONNECTicut server: [https://iowa-server.ioterop.com/](https://iowa-server.ioterop.com/)), you can follow the steps below:

(Setup network redirection for qemu taken from [https://docs.zephyrproject.org/latest/guides/networking/qemu_setup.html](https://docs.zephyrproject.org/latest/guides/networking/qemu_setup.html))


In different Linux terminals (4 terminals, including running client):

Terminal #1
```
./loop-socat.sh
```

Terminal #2
```
sudo ./loop-slip-tap.sh
```

Terminal #3
```
socat TCP-LISTEN:5683,fork TCP:iowa-server.ioterop.com:5683 # redirect local connection attempts to Connecticut
```

Terminal #4
Build and run IOWA client:

```
west build -b qemu_x86_64 -c
west build -t run
```
