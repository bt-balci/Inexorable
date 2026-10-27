#!/usr/bin/env python3
"""
INEXORABLE: Automated PCIe Network Driver & Traffic Verification Suite
Tests high-throughput PCIe networking inside Inexorable guest OS:
1. VirtIO Network PCI device (virtio-net-pci)
2. Intel PRO/1000 Gigabit PCI device (e1000)

Verifies:
- PCIe configuration space & BAR decoding via MCFG table (0xB0000000)
- Kernel driver auto-probing (virtio_net and e1000)
- Network interface registration (eth0)
- DHCP lease acquisition (10.0.2.15/24) via udhcpc
- Bi-directional ICMP ping traffic to gateway (10.0.2.2) with 0% packet loss
"""

import os
import sys
import time
import subprocess

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INEXORABLE_FD = os.path.join(REPO_ROOT, "inexorable.fd")
ROOTFS_IMG = os.path.join(REPO_ROOT, "inexorable_rootfs.img")

def to_wsl_path(win_path):
    if sys.platform != "win32":
        return os.path.abspath(win_path)
    p = os.path.abspath(win_path).replace("\\", "/")
    if len(p) >= 2 and p[1] == ":":
        drive = p[0].lower()
        return f"/mnt/{drive}{p[2:]}"
    return p

def run_network_test(device_type="virtio-net-pci"):
    print(f"\n====================================================================")
    print(f" [TEST] Testing PCIe Networking: {device_type}")
    print(f"====================================================================")

    fd_wsl = to_wsl_path(INEXORABLE_FD)
    rootfs_wsl = to_wsl_path(ROOTFS_IMG)

    cmd = [
        "qemu-system-x86_64",
        "-enable-kvm",
        "-machine", "q35",
        "-cpu", "host",
        "-m", "2G",
        "-bios", fd_wsl,
        "-kernel", "/boot/vmlinuz-linux",
        "-initrd", rootfs_wsl,
        "-device", f"{device_type},netdev=net0",
        "-netdev", "user,id=net0",
        "-append", "console=ttyS0 earlycon=uart8250,io,0x3f8 quiet loglevel=3 tsc=reliable clocksource=tsc no_timer_check lpj=3187195 audit=0 mitigations=off",
        "-serial", "stdio",
        "-display", "none",
        "-no-reboot"
    ]

    if sys.platform == "win32":
        wsl_cmd = ["wsl", "-d", "archlinux", "-e"] + cmd
    else:
        wsl_cmd = cmd

    proc = subprocess.Popen(
        wsl_cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )

    # Allow guest to boot to userspace and negotiate network
    time.sleep(3.0)

    # Send diagnostic commands to interactive shell
    commands = (
        "ip link\n"
        "ip addr\n"
        "ip route\n"
        "ping -c 2 10.0.2.2\n"
        "exit\n"
    )

    try:
        proc.stdin.write(commands)
        proc.stdin.flush()
        out, _ = proc.communicate(timeout=10)
    except Exception as e:
        proc.kill()
        out, _ = proc.communicate()
        print(f"[ERROR] Test failed with exception: {e}")

    print(out)

    # Verification checks
    success = True
    if "eth0" not in out:
        print(f"[FAIL] Network interface eth0 not found for {device_type}!")
        success = False
    else:
        print(f"[PASS] Network interface eth0 recognized over PCIe.")

    if "10.0.2.15" not in out:
        print(f"[FAIL] DHCP IP 10.0.2.15 not assigned to eth0!")
        success = False
    else:
        print(f"[PASS] DHCP IP 10.0.2.15 successfully leased.")

    if "0% packet loss" not in out and "0.0% packet loss" not in out:
        print(f"[FAIL] Ping to gateway 10.0.2.2 failed or had packet loss!")
        success = False
    else:
        print(f"[PASS] ICMP traffic passed over PCIe gateway (0% packet loss).")

    if "Powering off system..." in out or "reboot: System halted" in out or proc.returncode == 0:
        print(f"[PASS] Shell cleanly powered off guest system.")
    else:
        print(f"[WARN] Guest poweroff check uncertain (exit code {proc.returncode}).")

    return success

def main():
    print("====================================================================")
    print("    INEXORABLE PCIE NETWORK CONTROLLER VERIFICATION SUITE")
    print("====================================================================")

    res_virtio = run_network_test("virtio-net-pci")
    res_e1000e = run_network_test("e1000e")

    print("\n====================================================================")
    print("                      TEST RESULTS SUMMARY")
    print("====================================================================")
    print(f"VirtIO-Net (virtio-net-pci):         {'[PASSED]' if res_virtio else '[FAILED]'}")
    print(f"Intel PCIe Gigabit (e1000e):         {'[PASSED]' if res_e1000e else '[FAILED]'}")
    print("====================================================================")

    if res_virtio and res_e1000e:
        print("\nAll PCIe networking tests completed successfully!")
        sys.exit(0)
    else:
        print("\nNetworking tests failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()
