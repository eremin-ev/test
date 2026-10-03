#!/bin/sh

#
# Create a clean chroot environment with CAP_SYS_ADMIN
#
# Some administrative commands work, e.g.
#
# $ bin/bwrap.sh
# bash-5.3# mount -t binfmt_misc none /proc/sys/fs/binfmt_misc/
#
# bash-5.3# cat /proc/sys/fs/binfmt_misc/status
# enabled
#

# or /bin/bash -c "$@"

bwrap \
  --clearenv \
  --unshare-pid \
  --unshare-net \
  --unshare-ipc \
  --unshare-user \
  --unshare-uts --hostname sandbox \
  --unshare-cgroup-try \
  --new-session \
  --uid 0 \
  --gid 0 \
  --size $(( 512 * 1024 * 1024 )) --tmpfs '/' \
  --dir /usr \
  --dir /tmp \
  --dir /home \
  --proc /proc \
  --dev /dev \
  --ro-bind /sys /sys \
  --disable-userns \
  --die-with-parent \
  --symlink usr/bin /bin \
  --symlink usr/lib /lib \
  --symlink usr/lib /lib64 \
  --symlink lib /usr/lib64 \
  --ro-bind /usr/bin/sleep /usr/bin/sleep \
  --ro-bind /usr/bin/bash /usr/bin/bash \
  --ro-bind /usr/bin/cat /usr/bin/cat \
  --ro-bind /usr/bin/id /usr/bin/id \
  --ro-bind /usr/bin/ip /usr/bin/ip \
  --ro-bind /usr/bin/ls /usr/bin/ls \
  --ro-bind /usr/bin/mkdir /usr/bin/mkdir \
  --ro-bind /usr/bin/mount /usr/bin/mount \
  --ro-bind /usr/bin/ps /usr/bin/ps \
  --ro-bind /usr/lib/ld-linux-x86-64.so.2 /usr/lib/ld-linux-x86-64.so.2 \
  --ro-bind /usr/lib/libblkid.so.1 /usr/lib/libblkid.so.1 \
  --ro-bind /usr/lib/libbpf.so.1 /usr/lib/libbpf.so.1 \
  --ro-bind /usr/lib/libcap.so.2 /usr/lib/libcap.so.2 \
  --ro-bind /usr/lib/libc.so.6 /usr/lib/libc.so.6 \
  --ro-bind /usr/lib/libelf.so.1 /usr/lib/libelf.so.1 \
  --ro-bind /usr/lib/libgcc_s.so.1 /usr/lib/libgcc_s.so.1 \
  --ro-bind /usr/lib/libmnl.so.0 /usr/lib/libmnl.so.0 \
  --ro-bind /usr/lib/libmount.so.1 /usr/lib/libmount.so.1 \
  --ro-bind /usr/lib/libm.so.6 /usr/lib/libm.so.6 \
  --ro-bind /usr/lib/libncursesw.so.6 /usr/lib/libncursesw.so.6 \
  --ro-bind /usr/lib/libproc2.so.1 /usr/lib/libproc2.so.1 \
  --ro-bind /usr/lib/libreadline.so.8 /usr/lib/libreadline.so.8 \
  --ro-bind /usr/lib/libsystemd.so.0 /usr/lib/libsystemd.so.0 \
  --ro-bind /usr/lib/libz.so.1 /usr/lib/libz.so.1 \
  --ro-bind /usr/lib/libzstd.so.1 /usr/lib/libzstd.so.1 \
  /bin/bash
