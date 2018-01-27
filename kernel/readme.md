This directory contains kernel patches for kernel 4.9.28 included in Raspbian 2017-06-21.

Patches also apply to kernel 4.9.35 included in Raspbian 2017-07-05 (linux-4.9.28-dwc-otg.patch is already included in kernel 4.9.35 and later)

Raspberry 4.9.28 kernel sources:
https://github.com/raspberrypi/linux/archive/1423ac8bfbfb2a9d092b604c676e7a58a5fa3367.tar.gz

Raspberry 4.9.35 kernel sources:
https://github.com/raspberrypi/linux/archive/be2540e540f5442d7b372208787fb64100af0c54.tar.gz

Notes:
If you would like to compile your own kernel because you want to make changes, it's strongly recommended to use kernel 4.9.35, as this one has been extensively tested with EZ-Wifibroadcast and has turned out to be stable. Other kernel versions may have whatever issues that are not directly apparent, for example USB timing issues that lead to higher packetloss or instabilities that are hard to reproduce and may only show up under certain conditions or after many hours of running.

Quick "Howto" on compiling your own kernel:
- Download above linked kernel sources from the Raspberry Github repository
- Download EZ-Wifibroadcast kernel patches and kernel config files from this repository
- Apply patches and config to kernel, then make desired changes
- Build kernel
- Install kernel

Put these options (in exactly that order!) at the end of cmdline text:
dwc_otg.fiq_fsm_enable=0 dwc_otg.fiq_enable=0 dwc_otg.nak_holdoff=0 dwc_otg.int_ep_interval_min=0

Building the kernel on a Raspberry Pi1 or Pi0 is not recommended as it takes several hours to complete. On a Pi3 though, with good cooling and GPU overclocked to 500Mhz, compilation takes only about 20-30 minutes.
