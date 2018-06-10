ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = -DHAVE_SAMPLE_ICC -DNEED_UCHAR_UINT_T
AM_CXXFLAGS = -Wno-deprecated -fpermissive
AM_LDFLAGS = 

bin_PROGRAMS = imagickICCconvert avinfo

imagickICCconvert_CPPFLAGS = $(AM_CPPFLAGS)
imagickICCconvert_CXXFLAGS = $(AM_CXXFLAGS) $(IMAGEMAGICK_CFLAGS)
imagickICCconvert_LDFLAGS = $(IMAGEMAGICK_LIBS)
imagickICCconvert_LDADD = ../src/libimgcat.la
imagickICCconvert_SOURCES = imagickICCconvert.cc
    
avinfo_CPPFLAGS = $(AM_CPPFLAGS)
avinfo_CXXFLAGS = $(AM_CXXFLAGS) $(AVUTIL_CFLAGS) $(AVFORMAT_CFLAGS)
avinfo_LDFLAGS = $(AM_LDFLAGS) $(AVUTIL_LIBS) $(AVFORMAT_LIBS)
avinfo_LDADD = ../src/libimgcat.la
avinfo_SOURCES = avinfo.cc
