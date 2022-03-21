#!/bin/bash

CPUID=0
ORIG_ASLR=`cat /proc/sys/kernel/randomize_va_space`
ORIG_TURBO=`cat /sys/devices/system/cpu/intel_pstate/no_turbo`

sudo sh -c "echo 0 > /proc/sys/kernel/randomize_va_space"
sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"

make unload
make load
sudo taskset -c $CPUID make time
make unload
