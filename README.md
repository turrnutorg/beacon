# Beacon
![Beacon](images/BeaconBanner.png)
Beacon is an free, open source operating system created by the Turrnut Open Source Organization. The OS source code is distributed under the [GPL v3 License](COPYING).

### How to build & run

To build Beacon, you need to either be running Linux, or Windows with WSL (Windows Subsystem for Linux).
1. If you do not have all dependencies, please run `install-packages.sh` first.
2. Run `make compile`, and `os.iso` will be in the `out` folder in the current directory.
3. Burn to bootable media to run on real hardware (Any x86 cpu, at LEAST 16mb of RAM, and VGA Graphics are req'd; IDE HDD reccomended), or virtualize it using tools such as QEMU or VirtualBox. 
Example QEMU command: ```qemu-system-i386 -cdrom out/os.iso -hda [your image file here] -serial pty```

Note: If a valid Fat32 image file / Hard Disk is not attached, the OS may not boot.

Hard Disks are officially supported on real hw, but experimental, so when it comes to your data, here be dragons.

### Writing applications

Please see ./CSA for more information on how to create an application.

### Screenshots
![Starting Screen of BeaconOS](images/StartingScreen.png)

