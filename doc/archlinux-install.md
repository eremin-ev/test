# Installing Arch Linux with GRUB on an AMD UEFI System (No Secure Boot) via SSH

## Table of Contents

1. [Preparation](#1-preparation)<br>
   1.1. [Verify UEFI Boot Mode](#11-verify-uefi-boot-mode)<br>
   1.2. [Set Root Password and Start sshd](#12-set-root-password-and-start-sshd)<br>
   1.3. [Disable zsh `SHARE_HISTORY`](#13-disable-zsh-share_history)<br>
   1.4. [Enhance Security with SSH Keys](#14-enhance-security-with-ssh-keys)<br>
   1.5. [Connect from Local Machine](#15-connect-from-local-machine)<br>
2. [Partitioning with fdisk](#2-partitioning-with-fdisk)<br>
   2.1. [Identify the Target Disk](#21-identify-the-target-disk)<br>
   2.2. [Create or Modify the Partition Table](#22-create-or-modify-the-partition-table)<br>
   2.3. [Create the EFI System Partition (ESP)](#23-create-the-efi-system-partition-esp)<br>
   2.4. [Create the Root Partition](#24-create-the-root-partition)<br>
   2.5. [Write the Changes](#25-write-the-changes)<br>
3. [Formatting and Mounting](#3-formatting-and-mounting)<br>
   3.1. [Format the EFI System Partition](#31-format-the-efi-system-partition)<br>
   3.2. [Format the Root Partition](#32-format-the-root-partition)<br>
   3.3. [Mount the Root Partition](#33-mount-the-root-partition)<br>
   3.4. [Mount the EFI System Partition](#34-mount-the-efi-system-partition)<br>
4. [Cleanup Windows UEFI Entries](#4-cleanup-windows-uefi-entries)<br>
   4.1. [List Existing UEFI Boot Entries](#41-list-existing-uefi-boot-entries)<br>
   4.2. [Delete Windows Boot Entries](#42-delete-windows-boot-entries)<br>
   4.3. [Remove Microsoft Directory from ESP](#43-remove-microsoft-directory-from-esp)<br>
5. [Base Installation](#5-base-installation)<br>
   5.1. [Install Essential Packages with pacstrap](#51-install-essential-packages-with-pacstrap)<br>
   5.2. [Generate fstab](#52-generate-fstab)<br>
6. [Chroot and Configuration](#6-chroot-and-configuration)<br>
   6.1. [Change Root into the New System](#61-change-root-into-the-new-system)<br>
   6.2. [Set the Timezone](#62-set-the-timezone)<br>
   6.3. [Set the Locale](#63-set-the-locale)<br>
   6.4. [Set the Hostname](#64-set-the-hostname)<br>
   6.5. [Set the Root Password](#65-set-the-root-password)<br>
   6.6. [Install GRUB](#66-install-grub)<br>
   6.7. [Generate GRUB Configuration](#67-generate-grub-configuration)<br>
7. [Final Steps](#7-final-steps)<br>
   7.1. [Exit the Chroot](#71-exit-the-chroot)<br>
   7.2. [Unmount the Partitions](#72-unmount-the-partitions)<br>
   7.3. [Reboot the Target Machine](#73-reboot-the-target-machine)<br>
   7.4. [Verify Boot](#74-verify-boot)<br>
   7.5. [Post-Installation (Optional)](#75-post-installation-optional)<br>
8. [Summary of Arch Wiki Headings Cited](#8-summary-of-arch-wiki-headings-cited)<br>

---

This guide walks you through installing Arch Linux with GRUB on an AMD
UEFI system without Secure Boot, using SSH for remote administration. It
follows the official Arch Wiki [Installation Guide](https://wiki.archlinux.org/title/Installation_guide)
and cites the relevant headings throughout.

---

## 1. Preparation

### 1.1 Verify UEFI Boot Mode

Boot the target machine from the Arch Linux live USB. Confirm the live
environment has booted in UEFI mode:

```bash
root@archiso ~ # ls /sys/firmware/efi/efivars
```

If the directory exists and contains files, you are in UEFI mode. If the
command returns an error, you are in BIOS/legacy mode and must reboot in
UEFI mode.

> **Arch Wiki reference:** *Installation guide → Verify the boot mode*

### 1.2 Set Root Password and Start sshd

The default Arch root password is empty, which prevents SSH login. Set a root password:

```bash
root@archiso ~ # passwd
```

Ensure `PermitRootLogin yes` is set in `/etc/ssh/sshd_config`:

```bash
root@archiso ~ # grep PermitRootLogin /etc/ssh/sshd_config
```

If it is not set, edit the file and add or uncomment the line, then start the SSH daemon:

```bash
root@archiso ~ # systemctl start sshd.service
```

> **Arch Wiki reference:** *Install Arch Linux via SSH → On the remote (target) machine*

### 1.3 Disable zsh `SHARE_HISTORY`

The live environment uses zsh. `SHARE_HISTORY` can cause confusion when
typing commands in the TTY and via SSH simultaneously, as history is
shared between sessions. Disable it:

```bash
root@archiso ~ # setopt no_share_history

root@archiso ~ # setopt | grep shareh
```

Alternatively, add `unsetopt share_history` to `/etc/zsh/zshrc` or your user's `.zshrc`.

### 1.4 Enhance Security with SSH Keys

Using SSH keys is more secure than password authentication. Generate a
key pair on your local machine if you do not already have one:

```bash
user@host ~ $ ssh-keygen -t ed25519 -C "arch-install"
```

Copy the public key to the live environment's root account:

```bash
user@host ~ $ scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
    ~/.ssh/id_rsa.pub root@<target-IP>:~/.ssh/authorized_keys
```

After verifying key-based login works, prohibit password authentication
by editing `/etc/ssh/sshd_config` on the target:

```bash
root@archiso ~ # awk '! /^#/ && ! /^$/;' /etc/ssh/sshd_config
Include /etc/ssh/sshd_config.d/*.conf
PermitRootLogin prohibit-password
PubkeyAuthentication yes
PasswordAuthentication no
KbdInteractiveAuthentication no
AuthorizedKeysFile      .ssh/authorized_keys
Subsystem       sftp    /usr/lib/ssh/sftp-server
```

Remove a `.d` config-file allowing PasswordAuthentication on the target:

```bash
root@archiso ~ # rm /etc/ssh/sshd_config.d/10-archiso.conf
```

Reload the SSH daemon:

```bash
root@archiso ~ # systemctl reload sshd.service
```

> **Arch Wiki reference:** *SSH keys → Security*; *Install Arch Linux via SSH → On the remote (target) machine*

### 1.5 Connect from Local Machine

Connect to the target machine using SSH. Use the following options to
avoid polluting your local `known_hosts` file:

```bash
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@<target-IP>
```

These options prevent verification and writing of the live environment's
SSH host keys to `~/.ssh/known_hosts`, avoiding `REMOTE HOST
IDENTIFICATION HAS CHANGED` warnings on future connections.

> **Arch Wiki reference:** *Install Arch Linux via SSH → On the local machine*

---

## 2. Partitioning with fdisk

### 2.1 Identify the Target Disk

List available block devices:

```bash
root@archiso ~ # lsblk
```

Identify the disk you want to install Arch Linux on (e.g., `/dev/sda` or
`/dev/nvme0n1`).

### 2.2 Create or Modify the Partition Table

Open fdisk on the target disk:

```bash
root@archiso ~ # fdisk /dev/sdX
```

**If the disk uses an MBR/DOS partition table**, replace it with a new GPT partition table:

- Press `g` to create a new empty GPT partition table.
- Confirm when prompted.

**If an existing GPT is re-used**, remove all undesired partitions:

- Press `d` to delete a partition. Repeat for each partition you want to remove.
- Be careful not to delete partitions you intend to keep.

> **Arch Wiki reference:** *Partitioning → GUID Partition Table*; *fdisk → Create a new partition table*

### 2.3 Create the EFI System Partition (ESP)

The minimum size is calculated as: `sector_size x 65527` [1].  E.g.:

```
4 KiB x 65527 = 262108 KiB ~ 256 MiB
512 bytes x 65527 =  32 MiB
```

Use `lsblk` to get get physical (reported by the disk) and logical sector 
sizes:

```
lsblk -td
```

**But** please note that reported physical size most probably is an
emulated one.  Modern disks use 4096 sector size internally.  So to
align ESP (and the main root filesystem partition next to it), need
to use multiple of 4096.  So it's safer to choose

```
4 KiB x 65527 = 262108 KiB ~ 256 MiB
```

or even "round" 256 MiB:

```
echo $(( 256 * 1024 * 1024 ))
268435456
```

partition size.

Create a 512 MiB EFI system partition:

- Press `n` to create a new partition.
- Press `Enter` to accept the default first sector.
- Type `+512M` for the last sector.
- Press `t` to change the partition type.
- Select the partition number (e.g., `1`).
- Type `1` to set the type to "EFI System".

**Why 512 MiB is the recommended minimum:**

- Early and/or buggy UEFI firmware implementations may require at least 512 MiB.
- On drives with 4096 logical sector size (4K-native drives), the partition
  must be at least 260 MiB to be formatted as FAT32.

  [1] What is the absolute minimum size a UEFI system partition can be?<br>
  https://superuser.com/questions/1310927/what-is-the-absolute-minimum-size-a-uefi-system-partition-can-be

> **Arch Wiki reference:** *EFI system partition → Create the partition*

### 2.4 Create the Root Partition

Allocate all remaining disk space to the root partition (`/`):

- Press `n` to create a new partition.
- Press `Enter` to accept the default first sector.
- Press `Enter` to accept the default last sector (uses all remaining space).
- Press `t` and select the partition (e.g., `2`).
- Type `20` for "Linux filesystem" (or press `Enter` to use the default).

This root partition will include `/boot` (kernel and initramfs images).

### 2.5 Write the Changes

Press `w` to write the partition table and exit fdisk.

> **Arch Wiki reference:** *Partitioning → Partition scheme*; *fdisk → Create a partition*

---

## 3. Formatting and Mounting

### 3.1 Format the EFI System Partition

Format the ESP as FAT32:

```bash
root@archiso ~ # mkfs.fat -F 32 /dev/sdX1
```

### 3.2 Format the Root Partition

Format the root partition as ext4:

```bash
root@archiso ~ # mkfs.ext4 /dev/sdX2
```

### 3.3 Mount the Root Partition

Mount the root partition at `/mnt`:

```bash
root@archiso ~ # mount /dev/sdX2 /mnt
```

### 3.4 Mount the EFI System Partition

Create the mount point and mount the ESP:

```bash
root@archiso ~ # mkdir -p /mnt/boot/efi
root@archiso ~ # mount /dev/sdX1 /mnt/boot/efi
```

> **Arch Wiki reference:** *Installation guide → Format the partitions*; *Installation guide → Mount the file systems*

---

## 4. Cleanup Windows UEFI Entries

### 4.1 List Existing UEFI Boot Entries

Use `efibootmgr` to list all current boot entries:

```bash
root@archiso ~ # efibootmgr
```

This displays entries like `Boot0000`, `Boot0001`, etc., along with their descriptions.

### 4.2 Delete Windows Boot Entries

Identify any Windows boot entries (e.g., "Windows Boot Manager") and delete them by their boot number:

```bash
root@archiso ~ # efibootmgr -b <num> -B
```

Replace `<num>` with the hexadecimal boot number (e.g., `0` for `Boot0000`). Repeat for each Windows entry.

> **Arch Wiki reference:** *efibootmgr → Delete bootnum*

### 4.3 Remove Microsoft Directory from ESP

If the ESP was preserved from a previous Windows installation, remove the Microsoft directory:

```bash
root@archiso ~ # rm -rf /mnt/boot/efi/EFI/Microsoft
```

This ensures no Windows remnants remain on the ESP.

---

## 5. Base Installation

### 5.1 Install Essential Packages with pacstrap

Install the base system and necessary packages:

```bash
root@archiso ~ # pacstrap -K /mnt base base-devel linux linux-firmware amd-ucode vim grub efibootmgr networkmanager openssh
```

Package explanations:

- `base` – minimal base system
- `base-devel` – build tools for AUR and development
- `linux` – Linux kernel
- `linux-firmware` – firmware files for hardware
- `amd-ucode` – AMD CPU microcode updates
- `vim` – text editor
- `grub` – boot loader
- `efibootmgr` – UEFI boot manager utility
- `networkmanager` – network management
- `openssh` – SSH server and client

The `-K` flag initializes a fresh pacman keyring, which is recommended for a clean installation.

> **Arch Wiki reference:** *Installation guide → Install essential packages*; *pacstrap*

### 5.2 Generate fstab

Generate an fstab file using UUIDs:

```bash
root@archiso ~ # genfstab -U /mnt >> /mnt/etc/fstab
```

Verify the generated file:

```bash
root@archiso ~ # cat /mnt/etc/fstab
```

> **Arch Wiki reference:** *Installation guide → Generate an fstab file*; *genfstab*

---

## 6. Chroot and Configuration

### 6.1 Change Root into the New System

```bash
root@archiso ~ # arch-chroot /mnt
```

> **Arch Wiki reference:** *Installation guide → Chroot*; *Chroot → Using arch-chroot*

### 6.2 Set the Timezone

Create a symbolic link to your timezone:

```bash
ln -sf /usr/share/zoneinfo/Region/City /etc/localtime
```

Replace `Region/City` with your timezone (e.g., `America/New_York`). Then synchronize the hardware clock:

```bash
hwclock --systohc
```

> **Arch Wiki reference:** *Installation guide → Time zone*; *System time → Set time zone*

### 6.3 Set the Locale

Edit `/etc/locale.gen` and uncomment your desired locale(s):

```bash
vim /etc/locale.gen
```

For example, uncomment `en_US.UTF-8 UTF-8`. Then generate the locales:

```bash
locale-gen
```

Create `/etc/locale.conf` and set the `LANG` variable:

```bash
echo "LANG=en_US.UTF-8" > /etc/locale.conf
```

> **Arch Wiki reference:** *Installation guide → Localization*; *Locale → Generating locales*

### 6.4 Set the Hostname

Create `/etc/hostname` and write your desired hostname:

```bash
echo "myhostname" > /etc/hostname
```

Also update `/etc/hosts` for local hostname resolution:

```bash
cat >> /etc/hosts <<EOF
127.0.0.1   localhost
::1         localhost
127.0.1.1   myhostname.localdomain myhostname
EOF
```

> **Arch Wiki reference:** *Installation guide → Network configuration*; *Network configuration → Set the hostname*

### 6.5 Set the Root Password

```bash
passwd
```

Enter and confirm your new root password.

> **Arch Wiki reference:** *Installation guide → Root password*

### 6.6 Install GRUB

Install GRUB for x86_64 UEFI:

```bash
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=GRUB
```

- `--target=x86_64-efi` – specifies 64-bit UEFI
- `--efi-directory=/boot/efi` – specifies the ESP mount point
- `--bootloader-id=GRUB` – creates a `GRUB` directory in the ESP and a boot entry labeled "GRUB"

> **Arch Wiki reference:** *GRUB → UEFI systems → Installation*

### 6.7 Generate GRUB Configuration

```bash
grub-mkconfig -o /boot/grub/grub.cfg
```

This generates the main GRUB configuration file, detecting the installed kernel and creating boot menu entries.

> **Arch Wiki reference:** *GRUB → Generate the main configuration file*

---

## 7. Final Steps

### 7.1 Exit the Chroot

```bash
exit
```

### 7.2 Unmount the Partitions

Unmount the ESP and root partition in the correct order:

```bash
root@archiso ~ # umount -R /mnt
```

The `-R` flag recursively unmounts all nested mounts (including `/mnt/boot/efi`).

If you encounter a "device is busy" error, use:

```bash
root@archiso ~ # umount --lazy /mnt
```

> **Arch Wiki reference:** *Chroot → Exit the chroot*; *Installation guide → Unmount the file systems*

### 7.3 Reboot the Target Machine

```bash
root@archiso ~ # reboot
```

Remove the installation medium when prompted.

### 7.4 Verify Boot

After reboot, the GRUB boot menu should appear. Verify that:

- GRUB boots Arch Linux successfully.
- No Windows remnants appear in the boot menu.
- The system boots in UEFI mode (check with `ls /sys/firmware/efi/efivars`).

### 7.5 Post-Installation (Optional)

After first boot, enable essential services:

```bash
systemctl enable NetworkManager
systemctl enable sshd
```

Create a non-root user for daily use:

```bash
useradd -m -G wheel -s /bin/bash username
passwd username
```

Edit `/etc/sudoers` with `visudo` to uncomment the `%wheel ALL=(ALL:ALL) ALL` line.

---

## Summary of Arch Wiki Headings Cited

| Section | Arch Wiki Heading |
|---------|-------------------|
| Preparation | *Install Arch Linux via SSH*; *SSH keys* |
| Partitioning | *Partitioning*; *fdisk*; *EFI system partition* |
| Formatting & Mounting | *Installation guide → Format the partitions*; *Mount the file systems* |
| Windows Cleanup | *efibootmgr* |
| Base Installation | *Installation guide → Install essential packages*; *pacstrap*; *genfstab* |
| Chroot & Configuration | *Chroot*; *Installation guide → Time zone*; *Locale*; *Network configuration*; *GRUB* |
| Final Steps | *Installation guide → Unmount the file systems*; *Chroot → Exit the chroot* |

This guide provides a complete, Arch Wiki–referenced installation
workflow for an AMD UEFI system without Secure Boot, administered via
SSH.

---
<small>*Arch Linux is a trademark of the Arch Linux Project.
AMD is a trademark of Advanced Micro Devices, Inc.
Microsoft and Windows are trademarks of the Microsoft group of companies.
This independent guide is for informational purposes only and is
not affiliated with, sponsored by, or endorsed by the Arch Linux Project,
AMD, or Microsoft Corporation.*</small>
