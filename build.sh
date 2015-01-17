#!/bin/bash

export ARCH=arm

export CROSS_COMPILE=~/android/toolchain/arm-eabi-4.8/bin/arm-eabi-

make cm_alto45_defconfig

START=$(date +%s)

make -j16

make cm_alto45_defconfig

END=$(date +%s)
BUILDTIME=$((END - START))
B_MIN=$((BUILDTIME / 60))
B_SEC=$((BUILDTIME - E_MIN * 60))
echo -ne "\033[32mBuildtime: "
[ $B_MIN != 0 ] && echo -ne "$B_MIN min(s) "
echo -e "$B_SEC sec(s)\033[0m"
BUILD_START=$(date +"%s")
blue='\033[0;34m'
cyan='\033[0;36m'
yellow='\033[0;33m'
red='\033[0;31m'
nocol='\033[0m'
