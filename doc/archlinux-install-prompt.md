Write a comprehensive guide for installing Arch Linux with GRUB on an AMD
UEFI system (no Secure Boot) via SSH.  Cite the relevant Arch Wiki
headings (e.g., “Partition the disks”, “Install essential packages”,
“Install a boot loader”).  Cover the following:

1. **Preparation**
   - Check that live environment has booted on a UEFI system
   - Set a root password and start `sshd` systemd service in the live environment.
   - Disable zsh `SHARE_HISTORY` feature to avoid confusing when typing commands in the TTY and via SSH
   - Describe how to enhance security by using SSH keys on the live environment
     - Prohibit password authentication after key-based login to the live environment is verified
   - Connect from the local machine using `ssh root@<target‑IP>`.
     Add options to avoid polluting a user's `known_hosts` file.

2. **Partitioning with `fdisk`**
   - If the disk uses an MBR/DOS partition table, replace it with a new GPT partition table.
   - If an existing GPT is re-used, remove all undesired partitions
   - Create a 512 MiB EFI system partition (to be mounted at `/boot/efi`) if necessary
     - Explain why 512 MiB is the recommended minimum: early/buggy UEFI
     firmware may require it, and 4K‑native drives need ≥ 260 MiB for FAT32 compliance.
   - Allocate all remaining disk space to the root partition (`/`), which
   includes `/boot` (kernel and initramfs images go here).

3. **Formatting and Mounting**
   - Format ESP as FAT32 (if it was created on the previous step), the root partition as ext4.
   - Mount the root partition at `/mnt` and the ESP at `/mnt/boot/efi`.

4. **Cleanup Windows UEFI Entries**
   - Use `efibootmgr` to list (`efibootmgr`) and delete (`efibootmgr -b <num> -B`) any remaining Windows boot entries.
   - Remove `/mnt/boot/efi/EFI/Microsoft` subdirectory on the ESP if it was preserved at the previous steps

5. **Base Installation**
   - Use `pacstrap` to install the base system and necessary packages (e.g., `base`, `base-devel`, `linux`,
     `linux-firmware`, `amd-ucode`, `vim`, `grub`, `efibootmgr`, `networkmanager`, `openssh`).
   - Generate an `fstab` file with `genfstab -U /mnt >> /mnt/etc/fstab`.

6. **Chroot and Configuration**
   - `arch-chroot /mnt`.
   - Set the timezone, locale, hostname, and root password of the target system.
   - Install GRUB: `grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=GRUB`.
   - Generate GRUB config: `grub-mkconfig -o /boot/grub/grub.cfg`.

7. **Final Steps**
   - Exit the chroot, unmount the partitions, and reboot the target machine.
   - Verify that GRUB boots Arch Linux without Windows remnants.

---
<small>*Arch Linux is a trademark of the Arch Linux Project.
AMD is a trademark of Advanced Micro Devices, Inc.
Microsoft and Windows are trademarks of the Microsoft group of companies.
This independent guide is for informational purposes only and is
not affiliated with, sponsored by, or endorsed by the Arch Linux Project,
AMD, or Microsoft Corporation.*</small>
