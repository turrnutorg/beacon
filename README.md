# Beacon
![Beacon](images/BeaconBanner.png)
Beacon is an free, open source 64-bit operating system created by the Turrnut Open Source Organization. The OS source code is distributed under the [GPL v3 License](COPYING).

### How to build & run

To build Beacon, you need to either be running Linux, or Windows with WSL (Windows Subsystem for Linux).
1. If you do not have all dependencies, please run `install-packages.sh` first.
2. Run `make`, and `os.iso` and `floppy.img` will be in the `out` folder in the current directory.
3. Burn to CD-R (optionally write a floppy) for real hardware (8MB+ RAM, Intel 386, VGA Graphics req'd), or virtualize it using tools such as QEMU or VirtualBox. Example QEMU command: ```qemu-system-i386 -fda out/floppy.img -cdrom out/os.iso -boot d```
4. Note - most people will *NOT* need to use a floppy. Its intended purpose is to allow you to use the same disk, rather than continuously burning CDs for each new version. A real diskette is NOT needed in most use cases - for the time being.
5. Enter current date & time, or skip to use current RTC time.

### Screenshots
![Starting Screen of BeaconOS](images/StartingScreen.png)

