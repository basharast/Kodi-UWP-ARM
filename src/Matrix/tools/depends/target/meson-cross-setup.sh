#!/bin/sh

cat > $1 << EOF
[binaries]
$($NATIVEPREFIX/bin/python3 -c "print('c = {}'.format('$CC'.split()))")
$($NATIVEPREFIX/bin/python3 -c "print('cpp = {}'.format('$CXX'.split()))")
ar = '$AR'
strip = '$STRIP'
pkgconfig = '$NATIVEPREFIX/bin/pkg-config'

[host_machine]
system = '$MESON_SYSTEM'
cpu_family = '$MESON_CPU'
cpu = '$CPU'
endian = 'little'

[properties]
$($NATIVEPREFIX/bin/python3 -c "print('c_args = {}'.format([x for x in '$CFLAGS'.split() if x not in ['-g', '-gdwarf-2']]))")
$($NATIVEPREFIX/bin/python3 -c "print('c_link_args = {}'.format([x for x in '$LDFLAGS'.split()]))")
$($NATIVEPREFIX/bin/python3 -c "print('cpp_args = {}'.format([x for x in '$CXXFLAGS'.split() if x not in ['-g', '-gdwarf-2']]))")
$($NATIVEPREFIX/bin/python3 -c "print('cpp_link_args = {}'.format([x for x in '$LDFLAGS'.split()]))")

[paths]
prefix = '$PREFIX'
libdir = 'lib'
bindir = 'bin'
EOF
