# Beacon
![Beacon](images/BeaconBanner.png)
Beacon is an free, open source 64-bit operating system created by the Turrnut Open Source Organization. The OS source code is distributed under the [GPL v3 License](COPYING).

### How to build & run

To build Beacon, you need to either be running Linux, or Windows with WSL (Windows Subsystem for Linux).
1. If you do not have all dependencies, please run `install-packages.sh` first.
2. Run `make compile`, and `os.iso` will be in the `out` folder in the current directory.
3. Burn to bootable USB real hardware (8MB+ RAM, Intel 386, VGA Graphics req'd), or virtualize it using tools such as QEMU or VirtualBox. Example QEMU command: ```qemu-system-x86_64 -cdrom out/os.iso -serial pty```

### Writing applications

Please see ./CSA for more information on how to create an application.

### Screenshots
![Starting Screen of BeaconOS](images/StartingScreen.png)

