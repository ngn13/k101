#!/bin/bash -e 
source vars.sh

if [ "$EUID" -eq 0 ]; then
  SUDO=""
elif type "doas" > /dev/null; then
  SUDO="doas"
elif type "sudo" > /dev/null; then
  SUDO="sudo"
else 
  echo ">> Install sudo, doas or run the script as root"
  exit 1
fi

if ! type "pacstrap" > /dev/null; then
  echo ">> Please install arch-install-scripts"
  exit 1
fi

echo ">> Creating the root disk image"
rm -rf root.raw
truncate -s 6G root.raw
mkfs.ext4 root.raw
mkdir -p mnt 
$SUDO mount root.raw mnt

echo ">> Installing base system"
$SUDO pacstrap mnt base base-devel vim cowsay dhcpcd openssh linux-headers tmux
$SUDO cp -r module mnt/root/module
echo ">> Running chroot commands"
$SUDO chroot mnt /bin/bash -c "echo root:k101root | chpasswd"
$SUDO chroot mnt /bin/bash -c "echo k101 > /etc/hostname"
$SUDO chroot mnt /bin/bash -c "systemctl enable dhcpcd"
$SUDO chroot mnt /bin/bash -c "systemctl enable sshd"
$SUDO sh -c 'echo "cowsay welcome to k101!" >> mnt/root/.profile'
$SUDO cp files/vimrc mnt/root/.vimrc
$SUDO cp files/sshd  mnt/etc/ssh/sshd_config

echo ">> Installing kernel headers"
pushd linux 
  $SUDO make INSTALL_MOD_PATH="${PWD}/../mnt" modules_install
  $SUDO rm -rf "../mnt/lib/modules/${KERNEL_VERSION}/build"
popd
$SUDO cp -r linux "mnt/lib/modules/${KERNEL_VERSION}/build"
$SUDO rm -r "mnt/lib/modules/${KERNEL_VERSION}/build"/vmlinux*
$SUDO rm -r "mnt/lib/modules/${KERNEL_VERSION}/build"/arch/*/boot

$SUDO umount mnt
echo ">> Installation completed"
