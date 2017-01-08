export ARCH=arm
export CROSS_COMPILE=~/android/toolchain/arm-eabi-4.8/bin/arm-eabi-
make cm_alto45_defconfig
make -j16
