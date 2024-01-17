ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = -DHAVE_SAMPLE_ICC -DNEED_UCHAR_UINT_T
AM_CXXFLAGS = $(IMAGEMAGICK_CFLAGS) $(EXIV_CFLAGS) $(SAMPLEICC_CFLAGS) -Wno-deprecated -Wno-pointer-arith -fopenmp -pthread
AM_LDFLAGS = $(IMAGEMAGICK_LIBS) $(EXIV_LIBS)

noinst_LTLIBRARIES = libimgcat_helper.la

libimgcat_helper_la_CPPFLAGS = $(AM_CPPFLAGS)
libimgcat_helper_la_CXXFLAGS = $(AM_CXXFLAGS)
libimgcat_helper_la_LDFLAGS = $(AM_LDFLAGS)
libimgcat_helper_la_LIBADD = $(SAMPLEICC_LIBS)
libimgcat_helper_la_SOURCES = \
    ICCprofiles.c \
    version.cc
