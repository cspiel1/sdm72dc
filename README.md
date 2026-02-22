sdm72dc README
=============

A modbus/RTU controller and daemon for the SDM72D Smart Meter.


Configuration
-------------

On first start sdm72dc writes a configuration template to ~/.config/sdm72dc

```
ttydev     /dev/ttyUSB0
stopbits   1
slaveid    1
mqtthost   mqtt.host.org
mqttport   1883
mqttuser   user
mqttpass   ***
capath     /etc/ssl/certs
publish0   0x34 smartmeter/power/total
publish1   0x0156 smartmeter/energie/total
publish2   0x0180 smartmeter/energie/resetable
reset_hh   7
```

Requirements
-------------
E.g. on Debian/Ubuntu:
```
sudo apt install cmake libmodbus-dev libmosquitto-dev libssl-dev
```

Building
--------
```
mkdir build
cd build
cmake ..
make
```

Cross Compile for ARM
---------------------

- Download toolchain for ARM architecture from
https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
- Install toolchain
```
tar -xf arm-gnu-toolchain-*-x86_64-arm-none-linux-gnueabihf.tar.xz
# move it to your preferred location and create a symlink,
# e.g. ~/dev/arm-toolchain
```
- scp the sysroot to your build machine
```
rsync -ac root@<target>:/{lib,usr} ~/dev/arm-sysroot
```
- Edit arm.cmake to point to the toolchain and sysroot

- Cross compile for ARM
```
./arm.sh
```
