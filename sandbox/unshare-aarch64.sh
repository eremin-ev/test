#!/bin/bash
#
# Enter the aarch64 SDK with qemu-user + unshare only (no bwrap).
#
#   ./unshare-aarch64.sh                  # interactive bash
#   ./unshare-aarch64.sh uname -m
#   NET=1 ...                             # keep host network
#   RO=1 ...                              # SDK read-only
#   WORK=<dir> ...                        # bind host dir to /work
#   QEMU_CPU=<model>                      # exported into the sandbox, e.g.
#                                         #   QEMU_CPU=cortex-a72 - 10-30%
#                                         #   faster than the default 'max'
#
# Everything bwrap does declaratively is hand-written here: the SDK bind,
# proc, binfmt_misc, a minimal /dev (mknod is EPERM in a user namespace, so
# the few device files are bind-mounted from the host), /work, and
# pivot_root + lazy umount of the old root.
#
# Two things that are easy to get wrong:
#   * binfmt_misc must be mounted INSIDE the new root
#     ($sdk/proc/sys/fs/binfmt_misc), otherwise the lazy umount of the old
#     root kills the superblock and with it the qemu-aarch64 registration -
#     every exec then fails with "Exec format error".
#   * the interpreter path is resolved at registration time (the "P" flag
#     makes the kernel keep the file open), so it is a host path, valid only
#     before the pivot.
#
set -euo pipefail

DIR=$(dirname "$(readlink -f "$0")")
SDK=${SDK:-$DIR/sdk}
QEMU=${QEMU:-$DIR/qemu-aarch64-static}
CONF=${CONF:-$DIR/qemu-aarch64-static.conf}
NET=${NET:-0}
RO=${RO:-0}
WORK=${WORK:-}
QEMU_CPU=${QEMU_CPU:-}

# binfmt registration line, interpreter pointed at the real qemu binary
REG=$(awk -F: -v OFS=: -v q="$QEMU" '{ $7 = q; print }' "$CONF")

netarg=()
[ "$NET" = 1 ] || netarg=(--net)

# NOTE: the inner shell runs BEFORE the pivot, so it is the host /bin/sh and
# every path below is a host path.
exec unshare --map-root-user --mount --pid --fork "${netarg[@]}" \
	--propagation private --kill-child -- \
	/bin/sh -eu -c '
		sdk=$1; reg=$2; ro=$3; work=$4; net=$5; term=$6; qcpu=$7; shift 7

		if [ -n "$work" ]; then mkdir -p "$work"; fi

		mount --bind "$sdk" "$sdk"
		# put_old for pivot_root has to exist before the read-only remount
		mkdir -p "$sdk/tmp/.oldroot"
		if [ "$ro" = 1 ]; then mount -o remount,ro,bind "$sdk"; fi

		mount -t proc proc "$sdk/proc"
		mount -t binfmt_misc none "$sdk/proc/sys/fs/binfmt_misc"
		printf "%s\n" "$reg" > "$sdk/proc/sys/fs/binfmt_misc/register"

		mount -t tmpfs none "$sdk/dev"
		for d in null zero full random urandom tty; do
			touch "$sdk/dev/$d"
			mount --bind "/dev/$d" "$sdk/dev/$d"
		done
		mkdir -p "$sdk/dev/pts" "$sdk/dev/shm"
		mount -t devpts -o mode=620,ptmxmode=666,newinstance devpts "$sdk/dev/pts"
		touch "$sdk/dev/ptmx"
		mount --bind "$sdk/dev/pts/ptmx" "$sdk/dev/ptmx"
		mount -t tmpfs shm "$sdk/dev/shm"

		if [ -n "$work" ]; then
			mkdir -p "$sdk/work"
			mount --bind "$work" "$sdk/work"
		fi
		if [ "$net" = 1 ] && [ -r /etc/resolv.conf ]; then
			[ -e "$sdk/etc/resolv.conf" ] || : >"$sdk/etc/resolv.conf"
			mount --bind /etc/resolv.conf "$sdk/etc/resolv.conf"
		fi

		cd "$sdk"
		pivot_root . tmp/.oldroot
		PATH=/usr/bin:/bin
		umount -l /tmp/.oldroot
		# the dir itself cannot be removed when the SDK is mounted ro
		if [ "$ro" != 1 ]; then rmdir /tmp/.oldroot; fi

		# QEMU_CPU reaches qemu through the environment (binfmt passes no
		# interpreter args), so it has to be exported for every exec inside.
		qemuenv=QEMU_LD_PREFIX=/
		if [ -n "$qcpu" ]; then qemuenv="$qemuenv QEMU_CPU=$qcpu"; fi

		cd /root
		if [ $# -gt 0 ]; then
			exec env -i HOME=/root SHELL=/bin/bash USER=root LOGNAME=root \
				PATH=/usr/bin:/bin:/usr/sbin:/sbin "TERM=$term" $qemuenv "$@"
		fi
		exec env -i HOME=/root SHELL=/bin/bash USER=root LOGNAME=root \
			PATH=/usr/bin:/bin:/usr/sbin:/sbin "TERM=$term" $qemuenv /bin/bash -l
	' sh "$SDK" "$REG" "$RO" "$WORK" "$NET" "${TERM:-linux}" "$QEMU_CPU" "$@"
