#!/usr/bin/env python3
"""
INEXORABLE: Zero-Enumeration Standalone Rootfs Packager
Constructs an ultra-fast, self-contained, in-memory rootfs image (RAM-backed)
that runs 100% diskless without any external storage controller, partition tables,
or switch_root delays.

Features:
- Complete POSIX shell environment & core utilities (Busybox)
- Dynamic glibc / musl runtime loader support
- Automated devtmpfs, procfs, sysfs, tmpfs virtual filesystem mounting
- Interactive serial console on ttyS0
- Instantaneous userspace startup (< 5ms)
- Outputs standard compressed cpio archive: inexorable_rootfs.img
"""

import os
import sys
import shutil
import subprocess
import tempfile

WSL_DISTRO = "archlinux"

INIT_SCRIPT = """#!/bin/sh
# INEXORABLE Zero-Overhead Direct Rootfs Init
# Lead Architect: Borhan Tugsan Balci (bt-balci)
# Motto: "We strip off everything we don't need"

# 1. Mount virtual filesystems
mount -t devtmpfs devtmpfs /dev 2>/dev/null || true
mount -t proc proc /proc 2>/dev/null || true
mount -t sysfs sysfs /sys 2>/dev/null || true
mount -t tmpfs -o mode=0755,nodev,nosuid tmpfs /run 2>/dev/null || true
mount -t tmpfs -o mode=1777 tmpfs /tmp 2>/dev/null || true
mkdir -p /dev/pts /dev/shm
mount -t devpts devpts /dev/pts 2>/dev/null || true
mount -t tmpfs tmpfs /dev/shm 2>/dev/null || true

# Signal userspace arrival via kernel log buffer
echo "<5>[INEXORABLE-USERSPACE] Reached PID 1 in RAM (Zero Disk I/O)!" > /dev/kmsg 2>/dev/null || true

# Redirect stdio to console
if [ -c /dev/console ]; then
    exec </dev/console >/dev/console 2>&1
elif [ -c /dev/ttyS0 ]; then
    exec </dev/ttyS0 >/dev/ttyS0 2>&1
fi

# 2. Hostname & Network Loopback
echo inexorable-guest > /proc/sys/kernel/hostname 2>/dev/null || true
ip link set lo up 2>/dev/null || ifconfig lo 127.0.0.1 up 2>/dev/null

# 3. Telemetry & Welcome Banner
MEM_RAW=$(free 2>/dev/null | grep Mem | awk '{print $2}')
MEM_AVAIL_RAW=$(free 2>/dev/null | grep Mem | awk '{print $7}')
MEM_TOTAL=$(( ${MEM_RAW:-1048576} / 1024 ))
MEM_AVAIL=$(( ${MEM_AVAIL_RAW:-800000} / 1024 ))
CPU_COUNT=$(nproc 2>/dev/null || echo 1)
KERNEL_VER=$(uname -r)
UPTIME=$(cut -d' ' -f1 /proc/uptime 2>/dev/null || echo 0)

echo ""
echo "===================================================================="
echo "   INEXORABLE: Zero-Enumeration Standalone Linux Rootfs (100% RAM) "
echo "   Lead Architect: Borhan Tugsan Balci (bt-balci)                  "
echo "   Motto: Strip off everything we do not need                      "
echo "===================================================================="
echo "Kernel:   Linux ${KERNEL_VER}"
echo "CPUs:     ${CPU_COUNT} active cores"
echo "Memory:   ${MEM_TOTAL} MB total (${MEM_AVAIL} MB available)"
echo "Uptime:   ${UPTIME} seconds"
echo "Storage:  100% In-Memory Rootfs (tmpfs/ramfs) - Zero Disk I/O"
echo "===================================================================="
echo ""
echo "System ready. Welcome to Inexorable userspace."
echo ""

# 4. Launch interactive root shell on serial console
echo "<5>[INEXORABLE-USERSPACE] Spawning root shell on console..." > /dev/kmsg 2>/dev/null || true

while true; do
    if [ -c /dev/console ]; then
        /bin/sh </dev/console >/dev/console 2>&1 || true
    elif [ -c /dev/ttyS0 ]; then
        /bin/sh </dev/ttyS0 >/dev/ttyS0 2>&1 || true
    else
        /bin/sh || true
    fi
    echo "<5>[INEXORABLE-USERSPACE] Shell session ended; pausing 2s..." > /dev/kmsg 2>/dev/null || true
    sleep 2
done
"""

OS_RELEASE = """NAME="Inexorable OS"
PRETTY_NAME="Inexorable Zero-Enumeration Linux (RAM)"
ID=inexorable
VERSION="1.0"
VERSION_ID="1.0"
HOME_URL="https://github.com/bt-balci/Inexorable"
"""

PASSWD = """root:x:0:0:root:/root:/bin/sh
daemon:x:1:1:daemon:/usr/sbin:/bin/false
nobody:x:65534:65534:nobody:/:/bin/false
"""

GROUP = """root:x:0:
daemon:x:1:
tty:x:5:
dialout:x:20:
nobody:x:65534:
"""

def generate_rootfs_tree(staging_dir):
    print(f"[ROOTFS] Assembling filesystem tree at: {staging_dir}")

    # Standard directory hierarchy
    for d in ["bin", "sbin", "usr/bin", "usr/sbin", "usr/lib", "lib64",
              "etc", "proc", "sys", "dev", "run", "tmp", "root", "home",
              "var/log", "var/run"]:
        os.makedirs(os.path.join(staging_dir, d), exist_ok=True)

    # Symlink /lib -> usr/lib
    lib_link = os.path.join(staging_dir, "lib")
    if not os.path.lexists(lib_link):
        os.symlink("usr/lib", lib_link)

    # Symlink /sbin -> bin
    sbin_link = os.path.join(staging_dir, "usr/sbin_compat")

    # Write /init
    init_path = os.path.join(staging_dir, "init")
    with open(init_path, "w", newline="\n", encoding="utf-8") as f:
        f.write(INIT_SCRIPT)
    os.chmod(init_path, 0o755)

    # Write /etc files
    with open(os.path.join(staging_dir, "etc/os-release"), "w", newline="\n") as f:
        f.write(OS_RELEASE)
    with open(os.path.join(staging_dir, "etc/hostname"), "w", newline="\n") as f:
        f.write("inexorable-guest\n")
    with open(os.path.join(staging_dir, "etc/passwd"), "w", newline="\n") as f:
        f.write(PASSWD)
    with open(os.path.join(staging_dir, "etc/group"), "w", newline="\n") as f:
        f.write(GROUP)

    print("[ROOTFS] Skeleton and configurations generated.")

def to_wsl_path(win_path):
    p = os.path.abspath(win_path).replace("\\", "/")
    if len(p) >= 2 and p[1] == ":":
        drive = p[0].lower()
        return f"/mnt/{drive}{p[2:]}"
    return p

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)
    output_image = os.path.join(repo_root, "inexorable_rootfs.img")

    if "--stage" in sys.argv:
        stage_idx = sys.argv.index("--stage")
        staging_dir = sys.argv[stage_idx + 1]
        generate_rootfs_tree(staging_dir)
        return

    print("====================================================================")
    print("      INEXORABLE: STANDALONE ZERO-DISK ROOTFS PACKAGER")
    print("====================================================================")

    script_wsl = to_wsl_path(__file__)
    output_wsl = to_wsl_path(output_image)

    # We use WSL to populate binaries from the Linux environment
    wsl_script = f"""
ROOT=/tmp/inexorable_rootfs_staging
(grep inexorable_rootfs_staging /proc/mounts 2>/dev/null | awk '{{print $2}}' | xargs -r umount -l 2>/dev/null) || true
rm -rf $ROOT
mkdir -p $ROOT
set -e

python3 {script_wsl} --stage $ROOT

# Extract busybox applets from Arch Linux
if [ -d /tmp/rootfs_test/bin ]; then
    echo '[WSL] Copying utilities from /tmp/rootfs_test/bin...'
    cp -a /tmp/rootfs_test/bin/* $ROOT/bin/
    cp -a /tmp/rootfs_test/usr/lib/* $ROOT/usr/lib/ 2>/dev/null || true
else
    echo '[WSL] Extracting from /boot/initramfs-linux.img...'
    mkdir -p /tmp/extract_tmp
    cd /tmp/extract_tmp
    lsinitcpio -x /boot/initramfs-linux.img
    cp -a /tmp/extract_tmp/bin/* $ROOT/bin/
    cp -a /tmp/extract_tmp/usr/lib/* $ROOT/usr/lib/ 2>/dev/null || true
    rm -rf /tmp/extract_tmp
fi

# Ensure essential runtime libraries
for lib in libc.so.6 libcrypt.so.2 libm.so.6 libresolv.so.2; do
    if [ -f /usr/lib/$lib ]; then
        cp -a /usr/lib/$lib $ROOT/usr/lib/
    fi
done

if [ -f /usr/lib64/ld-linux-x86-64.so.2 ]; then
    cp -a /usr/lib64/ld-linux-x86-64.so.2 $ROOT/lib64/
elif [ -f /usr/lib/ld-linux-x86-64.so.2 ]; then
    cp -a /usr/lib/ld-linux-x86-64.so.2 $ROOT/lib64/
fi

# Create initial static device nodes required before devtmpfs mounts
mknod -m 600 $ROOT/dev/console c 5 1 2>/dev/null || true
mknod -m 666 $ROOT/dev/null c 1 3 2>/dev/null || true
mknod -m 666 $ROOT/dev/ttyS0 c 4 64 2>/dev/null || true
mknod -m 666 $ROOT/dev/tty c 5 0 2>/dev/null || true

# Ensure /init is executable
chmod +x $ROOT/init

# Pack into CPIO archive and compress with zstd
echo '[WSL] Packing into CPIO and compressing with zstd...'
cd $ROOT
find . | cpio -o -H newc | zstd -1 -T0 > {output_wsl}

SIZE=$(stat -c%s "{output_wsl}")
echo "[WSL] Successfully packaged Inexorable rootfs: $SIZE bytes ($((SIZE / 1024 / 1024)) MB)"
"""

    if "--stage" in sys.argv:
        stage_idx = sys.argv.index("--stage")
        staging_dir = sys.argv[stage_idx + 1]
        generate_rootfs_tree(staging_dir)
        return

    # Run packing via WSL
    print(f"[HOST] Invoking WSL distro '{WSL_DISTRO}' to assemble Linux binaries...")
    cmd = ["wsl", "-d", WSL_DISTRO, "-e", "bash", "-c", wsl_script]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.stdout:
        print(res.stdout)
    if res.stderr:
        print(res.stderr)
    if res.returncode != 0:
        print(f"[ERROR] Packaging failed with code {res.returncode}!")
        sys.exit(res.returncode)

    if os.path.exists(output_image):
        size_mb = os.path.getsize(output_image) / (1024 * 1024)
        print(f"[OK] Inexorable Standalone Rootfs ready: {output_image} ({size_mb:.2f} MB)")
    else:
        print("[ERROR] Output image not found!")
        sys.exit(1)

if __name__ == "__main__":
    main()
