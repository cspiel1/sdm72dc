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


