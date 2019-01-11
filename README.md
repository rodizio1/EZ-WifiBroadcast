![img](https://github.com/richardbmx/EZ-WifiBroadcast/blob/develop/logo115.png?raw=true)

Affordable Digital HD Video Transmission made easy!



Ez-WifiBroadcast is a radically simple digital data transmission system. A bidirectional data link is established using commercial off-the-shelf hardware like WiFi modules and a pair of Raspberry Pi computers. Coupled with special software this unique system allowes transmission of low latency HD video, telemetry and control data between endpoints. In comparison to a classical wireless connection Ez-WifiBroadcast tries to *imitate* the famous properties of an analog link like graceful signal degradation and no association between the endpoints. 

EZ-WifiBroadcast system is based on [Befinitiv's Wifibroadcast](https://befinitiv.wordpress.com/wifibroadcast-analog-like-transmission-of-live-video-data/) project but since then has signicantly evolved and matured thanks to the active support from open source community  members.

## Downloads ##

* Release candidate(recommended): v1.6RC6  
[EZ-Wifibroadcast-1.6RC6.zip on Google Drive](https://drive.google.com/open?id=1OgKU4dQoQWsV4T4tVOjq_XM0VrXMXaxs) or [from mirror on Freehoster (beware of ads)](https://www.file-upload.net/en/download-13063079/EZ-Wifibroadcast-1.6RC6.zip.html)


* Stable version: v1.5  
[EZ-Wifibroadcast-1.5.zip on Gdrive](https://drive.google.com/uc?id=0B8ke2EKPqvORdDNkSTdwNDZQZnc&export=download)

_**IMPORTANT: Read and follow the [wiring instructions](https://github.com/bortek/EZ-WifiBroadcast/wiki/Hardware-~-Propper-Wiring)**_

## Main Features ##

- Supports Raspberry Pi1A+, Pi1B+, Pi2B, Pi3B (Pi3A+ and Pi3B+ currently not supported!), Pi Zero, Pi Zero W, Odroid-W
- Typical glass-to-glass talency of ~125ms. Minimum achieved latency ~110ms (FPV-VR app allows for lower latency)
- Raspberry Pi V1 and V2 cameras supported
- HDMI cameras supported via HDMI-CSI adapters
- Up to 1920x1080p 30fps Resolution and up to 12Mbit video bitrate achived
- Support for 2.3/2.4/2.5Ghz bands as well as 5.2Ghz to 5.8Ghz bands
- Ranges of 300m to 3km can be easily achived. In some special setups range of 30km was achived
- Bi-directional MAVLink telemetry support
- RC Control via MAVLink, SUMD (Graupner/JR), IBUS (FlySky), SRXL (Multiplex)
- Quick bootup/startup, up to 10 seconds until video link is up

Full list of features can be found [here](https://github.com/bortek/EZ-WifiBroadcast/wiki/General-~-Features)


## For Contributors ## 

Please use develop branch for making any changes. When code is tested and is ready to be pushed to master branch create a Pull Request and assign or invite other users to review and approve your changes. When the change is approved it can be merged to the master branch.

Afterwards a new release can be created and tagged. 

### Code of merit ###
If you are in some way contributing to the project development please read [Code of merit](CODE_OF_MERIT.md) document to familiarize yourself with the rules. 
  
## License ##
* EZ-WifiBroadcast is licensed under GPLv2, you can find it's contents in [this link](LICENSE)
* Raspbian, Linux Kernel, Drivers are licensed under GPLv2
* Original code (if found) is licensed as stipulated in respective source files or under the GPLv2 license otherwise

### Acknowledgements for contributed work ###
Read about it [here](ACKNOWLEDGMENTS.md)

### Logotype ####
Ez-WifiBroadcast logo was contributed by Richard Caseres (user @richardbmx) which we are all thankful for. 

### Warranty/Disclamer ###
This is a free software and comes with no Warranty as stated in parts 11 and 12 of [GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) license.
