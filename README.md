# Socket offloading (aka modem driver) for SARA N2 modules

Socket offloading is (relatively) straight forward but requires some extra
care. For the AQ project we are using an UART extender running on I2C so there
are a few extra hoops and loops we have to go through when talking to the module.

The socket offloading is suitable for UDP data through N2 modules.

N3 modules use different AT commands (AT+NSOCR for N2, AT+USOCR for N3) but should
be relatively simple to implement.


## Signing and flashing the image

There are a few steps that must be done before the image can be signed. Start by
generating a key (and keep it in safe place -- without it you can't build valid
binaries for the device)

Run `make flash` to build, sign and copy the image to the flash. This requires
MCUBoot to be installed.

### Create a key

(Read through [https://docs.zephyrproject.org/latest/guides/west/sign.html](https://docs.zephyrproject.org/latest/guides/west/sign.html) for more information)

`imgtool keygen -k n2_fota.pem -t rsa-2048`

There's a key included in this repo but for obvious reasons I recommend not
using it for anything but testing.

### Build MCUBoot

The boot loader lives in its own repository ([The Zephyr readme for MCUBoot is here](https://mcuboot.com/mcuboot/readme-zephyr.html)) and must be built separately:

...Then run `west build` and `west flash`:

```
git clone https://github.com/JuulLabs-OSS/mcuboot
cd mcuboot/boot/zephyr
git checkout v1.4.0
```

Edit the `prj.conf` file and set the `CONFIG_BOOT_SIGNATURE_KEY_FILE` to point
to the key you generated above, add RTT logging at the end (it won't matter but
it's nice to see the RTT output from both the bootloader and the image):

```
# RTT logging
CONFIG_HAS_SEGGER_RTT=y
CONFIG_USE_SEGGER_RTT=y
CONFIG_RTT_CONSOLE=y
CONFIG_UART_CONSOLE=n
```

Finally run `west flash` to write MCUBoot to the flash. When you reboot you'll see something along the lines of this in the RTT log:

```
[00:00:00.753,936] <err> mcuboot: Image in the primary slot is not valid!
[00:00:00.895,324] <err> mcuboot: Unable to find bootable image
```

### Build image and sign it

```
west build
west sign -t imgtool -- --key [signing key].pem
west flash --hex-file build/zephyr/zephyr.signed.hex
```

When you reconnect the RTT logger to the device yougi'll (hopefully) see the firmware boot normally.

### Upload image to Horde

```bash
$ make
$ curl -XPOST  -HX-API-Token:{token} https://api.nbiot.engineering/collections/{cid}/firmware -F $ image=@build/zephyr/zephyr.signed.bin
$ curl -XPATCH -d'{"version":"{version}"}'  -HX-API-Token:{token} https://api.nbiot.engineering/collections/{cid}/firmware/{fid}
```
Set the new version on the device with PATCH:

`curl -XPATCH -d'{"firmware":{"targetFirmwareId": "{fid}"}}'  -HX-API-Token:{token} https://api.nbiot.engineering/collections/{cid}/devices/{did}`

This will update the device the next time it checks in.

## What I've learned

+NSONMI and power saving modes works... not intuitively. I'm not sure if this is
the module, the network or both trying to make my life hard but if you ignore
NSONMI URCs you won't be able to send or receive data until something times out.

Rebooting the module or powering it down doesn't help. It still has to time out.
Turning off power save mode makes is behave predictably. There's probably a good
explanation to this (or rather *an* explanation) but I can't tell for sure. For
our current needs power isn't an issue so power save is turned off.

Also AT+NSORF responds with an undocumented response if you call it before the
+NSONMI URC is sent. Don't use AT+NSORF with lengths > 512. It will say "ERROR"
regardless. This probably made a lot of sense for the one writing the firmware
but I have some bad news for you: It does not make sense for anyone else, mmkay?

CoAP options in Zephyr is by default 12 bytes. Anything longer will make the
LwM2M client say "unexpected endpoint data received". If you look at the source
code you'll see that it gets an EINVAL error code. Fear not! It's the server that
uses options more than 12 bytes long. This is logged by NET_ERR so I'm pretty
sure you will miss it. The value isn't *invalid* or *unexpected*, just not long
enough. Why use your own logger when you can use another subsystem's logger?
