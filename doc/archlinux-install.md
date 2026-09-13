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
   6.8. [Configure systemd-networkd (Wired Ethernet)](#68-configure-systemd-networkd-wired-ethernet)<br>
7. [Final Steps](#7-final-steps)<br>
   7.1. [Exit the Chroot](#71-exit-the-chroot)<br>
   7.2. [Unmount the Partitions](#72-unmount-the-partitions)<br>
   7.3. [Reboot the Target Machine](#73-reboot-the-target-machine)<br>
   7.4. [Verify Boot](#74-verify-boot)<br>
   7.5. [Post-Installation (Optional)](#75-post-installation-optional)<br>

---

This guide walks you through installing Arch Linux with GRUB on an AMD
UEFI system without Secure Boot, using SSH for remote administration. It
follows the official Arch Wiki [Installation Guide](https://wiki.archlinux.org/title/Installation_guide)
and cites the relevant headings throughout.

---

## 1. Preparation

Disable Secure Boot in your UEFI firmware settings before booting the Arch ISO.

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

Temporarily ensure `PermitRootLogin yes` is set in `/etc/ssh/sshd_config`
(`yes` will be replaced by `prohibit-password` when a host public key is
added to `authorized_keys` later in section 1.4):

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

root@archiso ~ # setopt | grep sharehistory
```
(the second command should produce an empty output)

Alternatively, add `unsetopt share_history` to `/etc/zsh/zshrc` or your user's `.zshrc`.

### 1.4 Enhance Security with SSH Keys

Using SSH keys is more secure than password authentication. Generate a
key pair on your local machine if you do not already have one:

```bash
user@host ~ $ ssh-keygen -t ed25519 -C "arch-install"
```

Copy the public key to the live environment's root account:

```bash
user@host ~ $ ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
    root@<target-IP> 'mkdir -vp ~/.ssh && chmod 700 ~/.ssh'
user@host ~ $ scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
    ~/.ssh/id_ed25519.pub root@<target-IP>:~/.ssh/authorized_keys
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
root@archiso ~ # rm -vf /etc/ssh/sshd_config.d/10-archiso.conf
```

Reload the SSH daemon:

```bash
root@archiso ~ # systemctl reload sshd.service
```

Verify that sshd on the target machine won't accept the root password:

```bash
user@host ~ $ ssh -o PubkeyAuthentication=no root@<target-IP>
```

> **Arch Wiki reference:** *SSH keys → Security*; *Install Arch Linux via SSH → On the remote (target) machine*

### 1.5 Connect from Local Machine

Connect to the target machine using SSH. Temporarily disable host-key
verification to avoid polluting your local `known_hosts` file with
generated archiso keys:

```bash
user@host ~ $ ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@<target-IP>
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

ESP is required to be formatted as a FAT12/16/32.  FAT32 is preferable for
internal disks.  The minimum size of a FAT32 is calculated as [1, 2]:

  - Data Region: 65,525 sectors
  - Reserved: 2 sectors
  - File Allocation Table (FAT): 4 byte per 65,525 entries ≈ 512 sectors
    (assuming 1 FAT table, using the minimum possible data cluster size of 1 sector)

Total: 65,525 + 2 + 512 = 66,039 sectors

Which gives following figures depending on the disk logical
(physical?) sector size:

- 4096 * (65525 + 2 + 512) ≈ 258 MiB
- 512 * (65525 + 2 + 512) ≈ 33 MiB

Use `lsblk` and `fdisk` to get physical (reported by the disk) and
logical sector sizes:

```
lsblk -td
```

```
root@archiso ~ # fdisk -l
Disk /dev/nvme0n1: 1.82 TiB, 2000398934016 bytes, 3907029168 sectors
Disk model: Super Disk
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 4096 bytes / 4096 bytes
Disklabel type: gpt
```

Please note that reported physical size most probably is an
emulated one.  Modern disks use 4096 sector size internally.
Also, a typical internal flash erase block size is 1 MiB.  So to
align ESP (and the main root filesystem partition which is next to
it), need to use multiple of 4096, and even better 1 MiB.

If the reported `LOG-SEC` is 512 bytes, a 64 MiB ESP is acceptable.

If `LOG-SEC=4096`, 512 MiB is a safer choice.

Create an EFI system partition:

- Press `n` to create a new partition.
- Press `Enter` to accept the default first sector, e.g.:
  - 2048 (= 1 MiB) for the `LOG-SEC=512` logical sector size.
- If LOG-SEC=512 → type +64M for the last sector
- If LOG-SEC=4096 → type +512M
- Press `t` to change the partition type.
- Select the partition number (e.g., `1`).
- Type `1` to set the type to "EFI System".

**References:**

  [EFI system partition → Create the partition](https://wiki.archlinux.org/title/EFI_system_partition#Create_the_partition)

  [1] What is the absolute minimum size a UEFI system partition can be?<br>
      https://superuser.com/questions/1310927/what-is-the-absolute-minimum-size-a-uefi-system-partition-can-be

  [2] WARNING: Not enough clusters for a 32 bit FAT! [SOLVED]<br>
      https://bbs.archlinux.org/viewtopic.php?id=168014

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

Re-read altered partition table manually if necessary (but fdisk usually does 
this automatically):

```bash
[root@archiso ~]# partprobe /dev/nvme0n1
```

> **Arch Wiki reference:** *Partitioning → Partition scheme*; *fdisk → Create a partition*

---

## 3. Formatting and Mounting

### 3.1 Format the EFI System Partition

Format the ESP as FAT32:

```bash
[root@archiso ~]# mkfs.fat -F 32 /dev/nvme0n1p1
mkfs.fat 4.2 (2021-01-31)
```

### 3.2 Format the Root Partition

Format the root partition as ext4:

```bash
[root@archiso ~]# mkfs.ext4 /dev/nvme0n1p2
```

### 3.3 Mount the Root Partition

Mount the root partition at `/mnt`:

```bash
[root@archiso ~]# mount -o noatime /dev/nvme0n1p2 /mnt/
```

### 3.4 Mount the EFI System Partition

Create the mount point and mount the ESP:

```bash
[root@archiso ~]# mkdir -v /mnt/efi
mkdir: created directory '/mnt/efi'

[root@archiso ~]# mount /dev/nvme0n1p1 /mnt/efi/
```

> **Arch Wiki reference:** *Installation guide → Format the partitions*; *Installation guide → Mount the file systems*

---

## 4. Cleanup Windows UEFI Entries

> **Warning:** These steps remove Windows boot entries and Microsoft EFI files.
> Skip this section entirely if you intend to dual-boot with Windows.

### 4.1 List Existing UEFI Boot Entries

Use `efibootmgr` to list all current boot entries:

```bash
[root@archiso ~]# efibootmgr
```

This displays entries like `Boot0000`, `Boot0001`, etc., along with their descriptions.

### 4.2 Delete Windows Boot Entries

Identify any Windows boot entries (e.g., "Windows Boot Manager") and delete them by their boot number:

```bash
[root@archiso ~]# efibootmgr --delete-bootnum -b 0000
```

Replace `<num>` with the hexadecimal boot number (e.g., `0000` for `Boot0000`). Repeat for each Windows entry.

> **Arch Wiki reference:** *efibootmgr → Delete bootnum*

### 4.3 Remove Microsoft Directory from ESP

If the ESP was preserved from a previous Windows installation, remove the Microsoft directory:

```bash
root@archiso ~ # rm -rf /mnt/efi/EFI/Microsoft
```

This ensures no Windows remnants remain on the ESP.

---

## 5. Base Installation

### 5.0 Exclude undesired locales, etc.

Prepare `/etc/pacman.d/noextract.conf`:

```bash
# Prevent the installation of the various locales
NoExtract = usr/share/locale/*
NoExtract = !usr/share/locale/en_US*
NoExtract = !usr/share/locale/ru*
NoExtract = !usr/share/locale/locale.alias

# Prevent the installation of the translated man pages
NoExtract = usr/share/man/*
NoExtract = !usr/share/man/man*

# Prevent the installation of all but English content in Qt applications
NoExtract = usr/share/*/translations/*.qm
NoExtract = !usr/share/*/translations/*en.qm
NoExtract = usr/share/*/nls/*.qm
NoExtract = usr/share/qt*/phrasebooks/*.qph
NoExtract = usr/share/qt*/translations/*.pak
NoExtract = !*/en-US.pak

# Prevent the installation of all but English content in Chromium and
# Electron applications
NoExtract = usr/share/*/locales/*.pak
NoExtract = opt/*/locales/*.pak
NoExtract = usr/lib/*/locales/*.pak
NoExtract = !*/en-US.pak

# Prevent the installation of all but the English and Russian iBus
# dictionary for emojis
NoExtract = usr/share/ibus/dicts/emoji-*.dict
NoExtract = !usr/share/ibus/dicts/emoji-en.dict
NoExtract = !usr/share/ibus/dicts/emoji-ru.dict
```

Then copy this file to the fresh installation chroot:

```bash
[root@archiso ~]# mkdir -vp /mnt/etc/pacman.d/
mkdir: created directory '/mnt/etc'
mkdir: created directory '/mnt/etc/pacman.d/'

[root@archiso ~]# cp -v /etc/pacman.d/noextract.conf /mnt/etc/pacman.d/
'/etc/pacman.d/noextract.conf' -> '/mnt/etc/pacman.d/noextract.conf'
```

Fix the archiso's `/etc/pacman.conf` configuration file since pacstrap
uses it on the initial enrollment:

```bash
[options]
# ... your other global options (Color, ParallelDownloads, etc.) ...

# Custom NoExtract rules
Include = /etc/pacman.d/noextract.conf
```

**References**

- Pacman / Tips\_and\_tricks / [Installing only content in required languages](https://wiki.archlinux.org/title/Pacman/Tips_and_tricks#Installing_only_content_in_required_languages)

### 5.1 Install Essential Packages with pacstrap

Install the base system and necessary packages (assuming a
machine will be used for development):

```bash
root@archiso ~ # pacstrap -K /mnt base base-devel linux linux-firmware \
    amd-ucode vim grub efibootmgr openssh e2fsprogs util-linux less
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
- `openssh` – SSH server and client

The `-K` flag initializes a fresh pacman keyring, which is recommended for a clean installation.

> **Arch Wiki reference:** *Installation guide → Install essential packages*; *pacstrap*

### 5.2 Generate fstab

Verify the generated fstab file using UUIDs:

```bash
root@archiso ~ # genfstab -U /mnt
```

Write (append) it to the target system:

```bash
root@archiso ~ # genfstab -U /mnt >> /mnt/etc/fstab
```

By default `pacstrap` creates a stub fstab file containing a kinda useful header:

```
# Static information about the filesystems.
# See fstab(5) for details.

# <file system> <dir> <type> <options> <dump> <pass>
```

so it worth appending `genfstab` output rather than overwriting the file completely.

Also consider mounting `/efi` read-only in the `/etc/fstab`:

```bash
# /dev/nvme0n1p1
UUID=<filesystem-uuid> /efi vfat ro,relatime,...  0 2
```

Note: need to re-mount read-write next time `grub-install` is used.

Make sure it contains `noatime` mount option for all solid-state drives (SSD,
NAND), or at least `relatime`.  The option prevents writing a few bytes of a
file *access time timestamp* on every read operation.  This process is called
Write Amplification.

SSD cannot write data in single-byte increments.  The minimum write unit to
physical flash memory is a Page (typically 4 KiB to 16 KiB).

- An atime timestamp update is only a few bytes

- To save those few bytes, the SSD controller must write an entire
  4 KiB (or larger) physical page

- Writing tiny bits of data frequently forces the SSD to dirty many pages with
  very little data, wearing out the drive's NAND cells much faster than sequential
  bulk writes

**References**<br>
- [*Installation guide → Configure the system → fstab*](https://wiki.archlinux.org/title/Installation_guide#Fstab)
- [*genfstab(8)*](https://man.archlinux.org/man/genfstab.8)

---

## 6. Chroot and Configuration

### 6.1 Change Root into the New System

```bash
root@archiso ~ # arch-chroot /mnt
```

Fix the main `/etc/pacman.conf` to use `NoExtract` rules from the
`/etc/pacman.d/noextract.conf` drop-in:

```bash
[options]
# ... your other global options (Color, ParallelDownloads, etc.) ...

# Custom NoExtract rules
Include = /etc/pacman.d/noextract.conf
```

> **Arch Wiki reference:** *Installation guide → Chroot*; *Chroot → Using arch-chroot*

### 6.2 Set the Timezone

Create a symbolic link to your timezone:

```bash
[root@archiso /]# ln -sf /usr/share/zoneinfo/Europe/Moscow /etc/localtime
[root@archiso /]# readlink /etc/localtime
/usr/share/zoneinfo/Europe/Moscow
```

Replace `Europe/Moscow` with your timezone (e.g., `America/New_York`). Then 
synchronize the hardware clock, Arch Linux expects UTC by default:

```bash
[root@archiso /]# hwclock --systohc --utc
```

> **Arch Wiki reference:** *Installation guide → Time zone*; *System time → Set time zone*

### 6.3 Set the Locale

Edit `/etc/locale.gen` and uncomment your desired locale(s):

```bash
[root@archiso /]# vim /etc/locale.gen
```

For example, uncomment `en_US.UTF-8 UTF-8` and `ru_RU.UTF-8 UTF-8`. Then generate the locales:

```bash
[root@archiso /]# locale-gen
Generating locales...
  en_US.UTF-8... done
  ru_RU.UTF-8... done
Generation complete.
```

Create `/etc/locale.conf` and set the `LANG` variable:

```bash
[root@archiso /]# echo "LANG=en_US.UTF-8" > /etc/locale.conf
```

> **Arch Wiki reference:** *Installation guide → Localization*; *Locale → Generating locales*

### 6.4 Set the Hostname

Create `/etc/hostname` and write your desired hostname:

```bash
[root@archiso /]# echo 'ms-s1-max' > /etc/hostname
```

Also update `/etc/hosts` for local hostname resolution:

```bash
cat > /etc/hosts <<EOF
127.0.0.1   localhost
::1         localhost
127.0.1.1   ms-s1-max.localdomain   ms-s1-max
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

Install GRUB for x86\_64 UEFI.  Step 1:

```bash
[root@archiso /]# grub-install --target=x86_64-efi --efi-directory=/efi --bootloader-id=GRUB
Installing for x86_64-efi platform.
Installation finished. No error reported.
```

where

- `--target=x86_64-efi` – specifies 64-bit UEFI
- `--efi-directory=/efi` – specifies the ESP mount point
- `--bootloader-id=GRUB` – creates a `GRUB` directory in the ESP and a boot 
  entry labeled "GRUB"

This populates `/efi/EFI/grub/grubx64.efi` and creates a functional entry in 
efibootmgr.

Step 2:

```
[root@archiso /]# grub-install --target=x86_64-efi --efi-directory=/efi --bootloader-id=GRUB --removable
Installing for x86_64-efi platform.
Installation finished. No error reported.
```

where

- `--removable` – create the copy of `/EFI/grub/grubx64.efi` file to 
  `/EFI/Boot/bootx64.efi` to protect against motherboard resets: if a CMOS 
  battery dies or the BIOS get updateed, the motherboard will still instantly 
  find and boot GRUB via this fallback file

This populates `/efi/EFI/Boot/bootx64.efi` without touching the efibootmgr list:

```bash
[root@archiso /]# find /efi/
/efi/
/efi/EFI
/efi/EFI/GRUB
/efi/EFI/GRUB/grubx64.efi
/efi/EFI/BOOT
/efi/EFI/BOOT/BOOTX64.EFI

[root@archiso /]# cmp /efi/EFI/GRUB/grubx64.efi /efi/EFI/BOOT/BOOTX64.EFI ; echo $?
0
```

> **Arch Wiki reference:** *GRUB → UEFI systems → Installation*

### 6.7 Generate GRUB Configuration

Change the GRUB options in `/etc/default/grub` to suit your needs.
For example, to see what happens during boot, change

```
GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"
```
to
```
GRUB_CMDLINE_LINUX_DEFAULT=""
```

This removes `quiet` and `loglevel=3` from the default kernel
command line.

The Linux kernel uses eight standard log levels (numbered 0 through 7) based on 
the POSIX/syslog severity levels.

When you set a parameter like `loglevel=3`, you are setting a threshold: the 
kernel will only print messages with a severity level equal to or lower than 
that number (meaning more severe messages).

| Level | Name | Description |
|---|---|---|
| 0 | `EMERG` (Emergency) | System is completely unusable (e.g., severe kernel panic). |
| 1 | `ALERT` | Immediate action is required (e.g., corrupted database or hardware failure). |
| 2 | `CRIT` (Critical) | Critical hardware or software conditions. |
| 3 | `ERR` (Error) | Non-fatal error conditions (often used to troubleshoot malfunctioning drivers). |
| 4 | `WARNING` | Warning messages alerting you to potential issues that aren't errors yet. |
| 5 | `NOTICE` | Normal but significant conditions that might require monitoring. |
| 6 | `INFO` | Informational messages detailing standard, successful kernel actions. |
| 7 | `DEBUG` | Detailed debug-level messages, usually used only by developers. |

After all fixes to `/etc/default/grub` are applied, generate GRUB 
configuration:

```bash
[root@archiso /]# grub-mkconfig -o /boot/grub/grub.cfg
Generating grub configuration file ...
Found linux image: /boot/vmlinuz-linux
Found initrd image: /boot/amd-ucode.img /boot/initramfs-linux.img
Warning: os-prober will not be executed to detect other bootable partitions.
Systems on them will not be added to the GRUB boot configuration.
Check GRUB_DISABLE_OS_PROBER documentation entry.
Adding boot menu entry for UEFI Firmware Settings ...
done
```

This generates the main GRUB configuration file `/boot/grub/grub.cfg`, 
detecting the installed kernel and creating boot menu entries.

Note, creating a new initramfs is usually not required, because mkinitcpio was 
run on installation of the kernel package with pacstrap:

```bash
[root@archiso ~]# pacstrap -K /mnt base base-devel linux linux-firmware amd-ucode ...
...
(15/18) Updating linux initcpios...
==> Building image from preset: /etc/mkinitcpio.d/linux.preset: 'default'
==> Using default configuration file: '/etc/mkinitcpio.conf'
  -> -k /boot/vmlinuz-linux -g /boot/initramfs-linux.img
==> Starting build: '7.2.6-arch2-1'
  -> Running build hook: [base]
  -> Running build hook: [systemd]
  -> Running build hook: [autodetect]
  -> Running build hook: [microcode]
  -> Running build hook: [modconf]
  -> Running build hook: [kms]
  -> Running build hook: [keyboard]
  -> Running build hook: [sd-vconsole]
==> WARNING: sd-vconsole: "/etc/vconsole.conf" not found, will use default values
  -> Running build hook: [block]
  -> Running build hook: [filesystems]
  -> Running build hook: [fsck]
==> Generating module dependencies
==> Creating zstd-compressed initcpio image: '/boot/initramfs-linux.img'
  -> Early uncompressed CPIO image generation successful
==> Initcpio image generation successful
...
```

> **Arch Wiki reference:** *GRUB → Generate the main configuration file*

## 6.8 Configure systemd-networkd (Wired Ethernet)

This section configures a wired Ethernet connection using `systemd-networkd` 
and a static `/etc/resolv.conf`. The `systemd` package, already installed via 
`base`, provides `systemd-networkd`; no extra packages are required for basic 
DHCP operation.

> **Arch Wiki reference:** *systemd-networkd → Basic usage*; *systemd-networkd 
> → Required services and setup*

### 6.8.1 Identify the Wired Interface with `ip`

List the network interfaces:

```bash
ip link show
```

Look for the wired adapter. Typical names are `enp1s0`, `enp2s0`, `eno1`, or `eth0`. Example output:

```text
lo               UNKNOWN        00:00:00:00:00:00 <LOOPBACK,UP,LOWER_UP>
enp1s0           UP             52:54:00:12:34:56 <BROADCAST,MULTICAST,UP,LOWER_UP>
wlan0            DOWN           ...
```

In this example, the wired interface is `enp1s0`. Note the exact name for the configuration file.

### 6.8.2 Example Configurations in `/usr/lib/systemd/network/`

The `systemd` package ships default and example `.network` files under 
`/usr/lib/systemd/network/`. For wired Ethernet, a typical example is 
`/usr/lib/systemd/network/89-ethernet.network.example`:

```ini
# SPDX-License-Identifier: MIT-0
#
# This example config file is installed as part of systemd.
# It may be freely copied and edited (following the MIT No Attribution license).
#
# To use the file, one of the following methods may be used:
# 1. add a symlink from /etc/systemd/network to the current location of this file,
# 2. copy the file into /etc/systemd/network or one of the other paths checked
#    by systemd-networkd and edit it there.
# This file should not be edited in place, because it'll be overwritten on upgrades.

# Enable DHCPv4 and DHCPv6 on all physical ethernet links
[Match]
Kind=!*
Type=ether

[Network]
DHCP=yes
```

This matches all physical Ethernet interfaces, excluding virtual Ethernet 
interfaces.  `Kind=!*` translates to: "Match this configuration only if the 
network interface does not have any device kind assigned to it."  Files placed 
in `/etc/systemd/network/` take precedence over those in 
`/usr/lib/systemd/network/`, so create your own configuration there.

> **Arch Wiki reference:** *systemd-networkd → Configuration examples → Devices 
> can also be matched by their type*

### 6.8.3 Create a `.network` File for DHCP

Create `/etc/systemd/network/20-wired.network`:

```bash
mkdir -vp /etc/systemd/network/
cat > /etc/systemd/network/20-wired.network <<EOF
[Match]
Name=enp1s0

[Network]
DHCP=yes
EOF
```

Replace `enp1s0` with your actual interface name. If you prefer to match any wired interface, use:

```ini
[Match]
Type=ether
Kind=!*

[Network]
DHCP=yes
```

> **Arch Wiki reference:** *systemd-networkd → Configuration examples → Wired adapter using DHCP*

### 6.8.4 Optional Static IP Configuration

If you need a static IP instead of DHCP, replace the `[Network]` section with:

```ini
[Network]
Address=10.1.10.9/24
Gateway=10.1.10.1
```

`Address=` can be used more than once to configure multiple IPv4 or IPv6 
addresses. Do not add `DNS=` here if you want the static `/etc/resolv.conf` 
below to remain untouched.

> **Arch Wiki reference:** *systemd-networkd → Configuration examples → Wired adapter using a static IP*

### 6.8.5 Configure a Static `/etc/resolv.conf` and Prevent `systemd-resolved` Overrides

`systemd-resolved` is the systemd component that normally takes over 
`/etc/resolv.conf`. If it is enabled, it replaces the file with a symlink to 
`/run/systemd/resolve/stub-resolv.conf` or `/run/systemd/resolve/resolv.conf`.  
To use a static file, ensure `systemd-resolved` is disabled:

```bash
systemctl is-enabled systemd-resolved
```

If it indicates `enabled`, disable it:

```bash
systemctl disable systemd-resolved
```

Next, create a static regular file:

```bash
cat > /etc/resolv.conf <<EOF
nameserver 9.9.9.9
nameserver 1.1.1.1
nameserver 2606:4700:4700::1111
EOF
```

Replace the addresses with the DNS servers you want to use. Do not add `DNS=` 
entries to the `.network` file. With `systemd-resolved` disabled, 
and no `DNS=` in `.network`, `systemd-networkd` will not rewrite 
`/etc/resolv.conf`.

> **Arch Wiki reference:** *resolv.conf*

### 6.8.6 Enable systemd-networkd

Enable the service to ensure it starts automatically at boot:

```bash
[root@archiso /]# systemctl enable systemd-networkd
Created symlink '/etc/systemd/system/dbus-org.freedesktop.network1.service' → '/usr/lib/systemd/system/systemd-networkd.service'.
Created symlink '/etc/systemd/system/multi-user.target.wants/systemd-networkd.service' → '/usr/lib/systemd/system/systemd-networkd.service'.
Created symlink '/etc/systemd/system/sockets.target.wants/systemd-networkd.socket' → '/usr/lib/systemd/system/systemd-networkd.socket'.
Created symlink '/etc/systemd/system/sockets.target.wants/systemd-networkd-varlink.socket' → '/usr/lib/systemd/system/systemd-networkd-varlink.socket'.
Created symlink '/etc/systemd/system/sockets.target.wants/systemd-networkd-varlink-metrics.socket' → '/usr/lib/systemd/system/systemd-networkd-varlink-metrics.socket'.
Created symlink '/etc/systemd/system/sockets.target.wants/systemd-networkd-resolve-hook.socket' → '/usr/lib/systemd/system/systemd-networkd-resolve-hook.socket'.
Created symlink '/etc/systemd/system/sysinit.target.wants/systemd-network-generator.service' → '/usr/lib/systemd/system/systemd-network-generator.service'.
Created symlink '/etc/systemd/system/network-online.target.wants/systemd-networkd-wait-online.service' → '/usr/lib/systemd/system/systemd-networkd-wait-online.service'.
```

(Hmm, why so many changes, I just enabled systemd-networkd...)

```bash
[root@archiso /]# systemctl is-enabled systemd-networkd
enabled
```

> **Arch Wiki reference:** *systemd-networkd → Required services and setup*

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

The `-R` flag recursively unmounts all nested mounts (including `/mnt/efi`).

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

After first boot, verify the connection: check that the interface has
obtained an address and that the service is active:

```bash
networkctl status enp1s0
ip addr show enp1s0
```

You should see an `inet` address (DHCP or static) and the link status should be 
`configured` or `routable`. Test connectivity and DNS resolution:

```bash
ping -c 3 ping.archlinux.org
getent hosts ping.archlinux.org
```

Enable essential services:

```bash
systemctl enable sshd
systemctl start sshd
```

Create a non-root user for daily use:

```bash
useradd -m -G wheel -s /bin/bash username
passwd username
```

Edit `/etc/sudoers` with `visudo` to uncomment the `%wheel ALL=(ALL:ALL) ALL` line.

Set the "screensaver" to blank the monitor after 1 minute of idle:
```bash
setterm --blank 1 </dev/tty1
```

Use the `poke` to recover from a unexpected behaviour of
`setterm --blank force </dev/tty1` command.  It just disables any keyboard
input so the monitor won't turn on:

```bash
TERM=linux setterm --blank poke </dev/tty1
```

---
<small>*Arch Linux is a trademark of the Arch Linux Project.
AMD is a trademark of Advanced Micro Devices, Inc.
Microsoft and Windows are trademarks of the Microsoft group of companies.
This independent guide is for informational purposes only and is
not affiliated with, sponsored by, or endorsed by the Arch Linux Project,
AMD, or Microsoft Corporation.*</small>
