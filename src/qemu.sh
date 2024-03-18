#!/bin/bash -e
source vars.sh

qemu-system-x86_64 -hda root.raw -m 4G -nographic                                         \
        -kernel linux/arch/x86/boot/bzImage                                               \
        -append "root=/dev/sda rw console=ttyS0 loglevel=5 $PARAMS"                       \
        -netdev user,id=user.0,hostfwd=tcp::2222-:22 -device e1000,netdev=user.0          \
        -cpu kvm64,+smep,+smap                                                            \
        --enable-kvm -s
