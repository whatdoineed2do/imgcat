ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = -DHAVE_SAMPLE_ICC -DNEED_UCHAR_UINT_T
AM_CXXFLAGS = $(IMAGEMAGICK_CFLAGS) $(EXIV_CFLAGS) $(SAMPLEICC_CFLAGS) -I../lib -Wno-deprecated -Wno-pointer-arith -fopenmp -pthread
AM_LDFLAGS = $(IMAGEMAGICK_LIBS) $(EXIV_LIBS) -L../lib -fopenmp -pthread

noinst_LTLIBRARIES = 

bin_PROGRAMS = imgprextr

imgprextr_CPPFLAGS = $(AM_CPPFLAGS)
imgprextr_CXXFLAGS = $(AM_CXXFLAGS)
imgprextr_LDFLAGS = $(AM_LDFLAGS)
imgprextr_LDADD = $(SAMPLEICC_LIBS) -limgcat_helper
imgprextr_SOURCES = imgprextr.cc
