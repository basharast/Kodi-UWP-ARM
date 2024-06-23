AC_DEFUN([XBMC_SETUP_ARCH_DEFINES],[

# build detection and setup - this is the native arch
case $build in
  i*86*-linux-gnu*|i*86*-*-linux-uclibc*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX")
     ;;
  x86_64-*-linux-gnu*|x86_64-*-linux-uclibc*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX")
     ;;
  i386-*-freebsd*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_FREEBSD")
     ;;
  amd64-*-freebsd*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_FREEBSD")
     ;;
  arm-apple-darwin*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX")
     ;;
  x86_64-apple-darwin*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX")
     ;;
  powerpc-*-linux-gnu*|powerpc-*-linux-uclibc*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX -D_POWERPC")
     ;;
  powerpc64-*-linux-gnu*|powerpc64-*-linux-uclibc*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX -D_POWERPC64")
     ;;
  arm*-*-linux-gnu*|arm*-*-linux-uclibc*|aarch64*-*-linux-gnu*|aarch64*-*-linux-uclibc*)
     AC_SUBST(NATIVE_ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX")
     ;;
  *)
     AC_MSG_ERROR(unsupported native build platform: $build)
esac


# host detection and setup - this is the target arch
case $host in
  i*86*-linux-gnu*|i*86*-*-linux-uclibc*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX")
     ;;
  x86_64-*-linux-gnu*|x86_64-*-linux-uclibc*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX")
     ;;
  i386-*-freebsd*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_FREEBSD")
     ;;
  amd64-*-freebsd*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_FREEBSD")
     ;;
  aarch64-apple-darwin*)
     if test "$target_platform" = "macosx" ; then
        AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX")
     else
        AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_EMBEDDED")
     fi
     ;;
  x86_64-apple-darwin*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_DARWIN -DTARGET_DARWIN_OSX")
     ;;
  powerpc-*-linux-gnu*|powerpc-*-linux-uclibc*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX -D_POWERPC")
     ;;
  powerpc64*-*-linux-gnu*|powerpc64*-*-linux-uclibc*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX -D_POWERPC64")
     ;;
  arm*-*-linux-gnu*|arm*-*-linux-uclibc*|aarch64*-*-linux-gnu*|aarch64*-*-linux-uclibc*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX")
     ;;
  mips*-*-linux-gnu*|mips*-*-linux-uclibc*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX")
     ;;
  *-*linux-android*)
     AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX -DTARGET_ANDROID")
     ;;
  *)
     AC_MSG_ERROR(unsupported build target: $host)
esac

if test "$target_platform" = "target_android" ; then
  AC_SUBST(ARCH_DEFINES, "-DTARGET_POSIX -DTARGET_LINUX -DTARGET_ANDROID")
fi

])
