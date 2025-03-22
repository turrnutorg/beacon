# Beacon
Beacon is an free, open source 64-bit operating system created by the Turrnut Open Source Organization. The OS source code is distributed under the [GPL v3 License](COPYING).

### How to build & run

To build Beacon, you need to either be running Linux, or Windows with WSL (Windows Subsystem for Linux).
1. If you do not have all dependencies, please run `install-packages.sh` first.
2. Run `make compile`, and `os.iso` will be in the `out` folder in the current directory.
3. Burn to CD-R for real hardware (8MB+ RAM), or virtualize it using tools such as QEMU or VirtualBox. Example QEMU command: ```qemu-system-x86_64 -cdrom out/os.iso```
4. Enter current date & time, or skip to use current RTC time.

### Screenshots
![Starting Screen of BeaconOS](screenshots/StartingScreen.png)

