# Beacon
Beacon is an free, open source 64-bit operating system created by the Turrnut Open Source Organization. The OS source code is distributed under the [GPL v3 License](COPYING).

### How to build & run

To build Beacon, you need to either be running Linux, or Windows with WSL (Windows Subsystem for Linux).
Steps:
1. If you do not have all dependencies, please run install-packages.sh first.
2. Simply put "make" into terminal, and it should output "os.iso" into the "out" folder in the Beacon directory.
3. Simply either burn to CD-R to run on real hardware (8mb or more RAM needed), or use virtualization. Example QEMU command: qemu-system-i386 -cdrom out/os.iso
4. Press "Enter" when GRUB prompts you. Then just enter current date & time, or skip to use whatever your current RTC is set to.

### Screenshots
![Starting Screen of BeaconOS](screenshots/StartingScreen.png)

