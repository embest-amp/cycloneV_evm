how to compile the kernel and u-boot?

The cross tool :
It can be downloaded from the http://www.rocketboards.org/foswiki/Documentation/GsrdGitTrees.


u-boot:
export PATH=/opt/altera-linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7-2012.11-20121123_linux/bin/:$PATH
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- socfpga_cyclone5_config
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- all

kernel:
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-  LOADADDR=0x8000 menuconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-  LOADADDR=0x8000 socfpga_amp_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-  LOADADDR=0x8000 uImage 
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-  LOADADDR=0x8000 dtbs






