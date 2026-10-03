```bash
tar -xf ../qemu-user-static-11.1.1-4-x86_64.pkg.tar.zst usr/bin/qemu-aarch64-static
tar -xf ../qemu-user-static-binfmt-11.1.1-4-x86_64.pkg.tar.zst usr/lib/binfmt.d/qemu-aarch64_be-static.conf
mkdir -v sdk/
cd sdk/
tar -xf ../../sdk-aarch64.tar.xz
```
