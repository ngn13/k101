#!/bin/bash -e
source vars.sh

check_hash() {
  if ! echo "$2 $1" | md5sum -c > /dev/null; then
    echo ">> Hash verification for $1 failed!"
    exit 1
  fi
  echo ">> Hash verification success!"
}

rm -rf linux
echo ">> Downloading kernel $KERNEL_VERSION"
wget "https://www.kernel.org/pub/linux/kernel/v$KERNEL_MAJOR.x/linux-$KERNEL_VERSION.tar.xz"
check_hash "linux-$KERNEL_VERSION.tar.xz" $KERNEL_SUM
echo ">> Extracting kernel archive"
tar xf "linux-$KERNEL_VERSION.tar.xz" && rm "linux-$KERNEL_VERSION.tar.xz"
mv "linux-$KERNEL_VERSION" linux && cd linux 

echo ">> Using x86_64_defconfig and kvmconfig"
make x86_64_defconfig
sed -i "s/CONFIG_DEBUG_INFO=n/CONFIG_DEBUG_INFO=y/g" .config
echo ">> Compiling the kernel"
make -j$(nproc)
echo ">> Kernel is ready to go!" 
