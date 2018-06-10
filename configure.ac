AC_PREREQ(2.64)
AC_INIT([audiotag], [1.1.4], [https://github.com/whatdoineed2do/imgcat/issues], [https://github.com/whatdoineed2do/imgcat], [whatdoineed2do@nospam-github.com])
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_CXX
AC_HEADER_STDC
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(IMAGEMAGICK, [Magick++             >= 6.9.9])
PKG_CHECK_MODULES(EVIX,        [exiv2                >= 0.25])
PKG_CHECK_MODULES(AVFORMAT,    [libavformat          >= 57.71.100])
PKG_CHECK_MODULES(AVUTIL,      [libavutil            >= 55.58.100])
PKG_CHECK_MODULES(FFMPEGTHUMB, [libffmpegthumbnailer >= 2.2.0])
PKG_CHECK_MODULES(SAMPLEICC,   [sampleicc            >= 1.6.8])

AC_ARG_ENABLE([debug-code],
	[AS_HELP_STRING([--enable-debug],
	[enable debug (default is no)])],
	[no_debug=false],
	[no_debug=true])
if test "$no_debug" = true; then
        building_debug=no
        AC_DEFINE(NDEBUG,1,[debug code])
else
        building_debug=yes
        AC_DEFINE(DEBUG,1,[debug code])
fi


AC_OUTPUT([
Makefile
src/Makefile
])
