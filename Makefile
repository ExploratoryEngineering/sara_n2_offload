all:
	west build  --board nrf52_pca10040
	west sign -t imgtool -- --key n2_fota.pem

clean:
	rm -fR build

flash:
	west build --board nrf52_pca10040
#	west flash
	west sign -t imgtool -- --key n2_fota.pem
	west flash --hex-file build/zephyr/zephyr.signed.hex
