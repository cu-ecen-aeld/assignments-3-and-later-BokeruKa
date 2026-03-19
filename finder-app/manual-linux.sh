#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

#Already fulfills implementation task 1a)
if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

#Fulfill Implementation task 1b)
if ! mkdir -p "${OUTDIR}"; then
    echo "Error: could not create directory ${OUTDIR}" >&2
    exit 1
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    #Fulfill Implementation task 1c)
    #Build a kernel image using instructions in Module 2 lecture series.

    #deep clean the kernel build tree
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    #configure for our "virt" arm device which we will simulate in QEMU
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    #build a kernel image for booting with QEMU
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    #build any kernel modules
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    #build the device tree
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone https://github.com/mirror/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

# Go back to rootfs to check dependencies
cd "${OUTDIR}/rootfs"

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Add library dependencies to rootfs
echo "Copying library dependencies to rootfs"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
# copy program interpreter 
cp -a "${SYSROOT}/lib/ld-linux-aarch64.so.1" lib/
# copy shared libraries
cp -a "${SYSROOT}/lib64/libm.so.6" lib64/
cp -a "${SYSROOT}/lib64/libresolv.so.2" lib64/
cp -a "${SYSROOT}/lib64/libc.so.6" lib64/

# Make device nodes
echo "Creating device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# Clean and build the writer utility
echo "clean and build the writer utility"
cd "${FINDER_APP_DIR}"
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copying finder related scripts and executables to the /home directory on the target rootfs"

cp writer "${OUTDIR}/rootfs/home/"
cp finder.sh "${OUTDIR}/rootfs/home/"
cp finder-test.sh "${OUTDIR}/rootfs/home/"
cp autorun-qemu.sh "${OUTDIR}/rootfs/home/"
mkdir -p "${OUTDIR}/rootfs/home/conf"
cp conf/username.txt "${OUTDIR}/rootfs/home/conf/"
cp conf/assignment.txt "${OUTDIR}/rootfs/home/conf/"
sed -i 's|\.\./conf/assignment.txt|conf/assignment.txt|g' "${OUTDIR}/rootfs/home/finder-test.sh"

# Chown the root directory
echo "Changing ownership of root directory"
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *

# Create initramfs.cpio.gz
echo "Creating initramfs.cpio.gz"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio

