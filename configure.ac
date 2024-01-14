AC_PREREQ([2.69])
AC_INIT([imgcat],[m4_esyscmd([git describe --tags --dirty --always | tr -d '\n'])],[https://github.com/whatdoineed2do/imgcat/issues],[https://github.com/whatdoineed2do/imgcat],[whatdoineed2do@nospam-github.com])
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
AC_CONFIG_MACRO_DIRS([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_CXX
AC_HEADER_STDC
AC_PROG_INSTALL
LT_INIT
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(IMAGEMAGICK, [Magick++ >= 6.9.9])
PKG_CHECK_MODULES(EXIV,        [exiv2 >= 0.25])
PKG_CHECK_MODULES(AVFORMAT,    [libavformat >= 57.71.100])
PKG_CHECK_MODULES(AVUTIL,      [libavutil >= 55.58.100])
PKG_CHECK_MODULES(AVCODEC,     [libavcodec >= 57.48.101])
PKG_CHECK_MODULES(AVFILTER,    [libavfilter >= 6.47.100])
PKG_CHECK_MODULES(FFMPEGTHUMB, [libffmpegthumbnailer >= 2.1.0])
PKG_CHECK_MODULES(SAMPLEICC,   [sampleicc >= 1.6.8])

AC_ARG_ENABLE(debug,
        AS_HELP_STRING([--enable-debug],[enable debug code (default is no)]),
        [ debug="${enableval}" ], [ debug=no ] )
if test "x$debug" = xno; then
        AC_DEFINE(NDEBUG,1,[debug code])
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
tests/Makefile
])
AC_OUTPUT
