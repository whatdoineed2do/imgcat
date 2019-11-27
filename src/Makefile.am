ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = -DHAVE_SAMPLE_ICC -DNEED_UCHAR_UINT_T
AM_CXXFLAGS = $(IMAGEMAGICK_CFLAGS) $(EXIV_CFLAGS) $(SAMPLEICC_CFLAGS) -Wno-deprecated -Wno-pointer-arith
AM_LDFLAGS = $(IMAGEMAGICK_LIBS) $(EXIV_LIBS) 


noinst_LTLIBRARIES = libimgcat.la

bin_PROGRAMS = imgcat imgprextr

libimgcat_la_CPPFLAGS = $(AM_CPPFLAGS)
libimgcat_la_CXXFLAGS = $(AM_CXXFLAGS) $(AVFORMAT_CFLAGS) $(AVUTIL_CFLAGS) $(FFMPEGTHUMB_CFLAGS)
libimgcat_la_LDFLAGS = $(AM_LDFLAGS) $(AVFORMAT_LIBS) $(AVUTIL_LIBS) $(FFMPEGTHUMB_LIBS)
libimgcat_la_LIBADD = $(SAMPLEICC_LIBS)
libimgcat_la_SOURCES = \
    ICCprofiles.c \
    version.cc \
    ImgData.cc  \
    ImgMetaParser.cc  \
    ImgExifParser.cc  \
    ImgOut.cc  \
    ImgOutJson.cc  \
    ImgHtml.cc  \
    ImgHtmlJG.cc \
    ImgOutJson.cc \
    ImgAVFmtParser.cc \
    ImgIdx.cc  \
    ImgKey.cc \
    ImgThumbGen.cc 

imgcat_CPPFLAGS = $(AM_CPPFLAGS)
imgcat_CXXFLAGS = $(AM_CXXFLAGS) $(AVFORMAT_CFLAGS) $(AVUTIL_CFLAGS) $(FFMPEGTHUMB_CFLAGS)
imgcat_LDFLAGS = $(AM_LDFLAGS) $(AVFORMAT_LIBS) $(AVUTIL_LIBS) $(FFMPEGTHUMB_LIBS)
imgcat_LDADD = $(SAMPLEICC_LIBS) libimgcat.la
imgcat_SOURCES = imgcat.cc 
    
imgprextr_CPPFLAGS = $(AM_CPPFLAGS)
imgprextr_CXXFLAGS = $(AM_CXXFLAGS)
imgprextr_LDFLAGS = $(AM_LDFLAGS)
imgprextr_LDADD = $(SAMPLEICC_LIBS) libimgcat.la
imgprextr_SOURCES = imgprextr.cc
