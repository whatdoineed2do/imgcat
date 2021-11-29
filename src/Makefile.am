ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = -DHAVE_SAMPLE_ICC -DNEED_UCHAR_UINT_T
AM_CXXFLAGS = -std=c++17 $(IMAGEMAGICK_CFLAGS) $(EXIV_CFLAGS) $(SAMPLEICC_CFLAGS) -Wno-deprecated -Wno-pointer-arith -fopenmp -pthread
AM_LDFLAGS = $(IMAGEMAGICK_LIBS) $(EXIV_LIBS) -fopenmp -pthread

noinst_LTLIBRARIES = libimgcat_helper.la libimgcat.la

bin_PROGRAMS = imgcat imgprextr

libimgcat_helper_la_CPPFLAGS = $(AM_CPPFLAGS)
libimgcat_helper_la_CXXFLAGS = $(AM_CXXFLAGS)
libimgcat_helper_la_LDFLAGS = $(AM_LDFLAGS)
libimgcat_helper_la_LIBADD = $(SAMPLEICC_LIBS)
libimgcat_helper_la_SOURCES = \
    ICCprofiles.c \
    version.cc

libimgcat_la_CPPFLAGS = $(AM_CPPFLAGS)
libimgcat_la_CXXFLAGS = $(AM_CXXFLAGS) $(FFMPEGTHUMB_CFLAGS) $(AVFORMAT_CFLAGS) $(AVUTIL_CFLAGS) $(AVCODEC_CFLAGS) $(AVFILTER_CFLAGS)
libimgcat_la_LDFLAGS = $(AM_LDFLAGS) $(FFMPEGTHUMB_LIBS) $(AVFORMAT_LIBS) $(AVFILTER_LIBS) $(AVUTIL_LIBS) $(AVCODEC_LIBS)
libimgcat_la_LIBADD = $(SAMPLEICC_LIBS)
libimgcat_la_SOURCES = \
    ImgData.cc  \
    ImgMetaParser.cc  \
    ImgExifParser.cc  \
    ImgOut.cc  \
    ImgOutJson.cc  \
    ImgHtml.cc  \
    ImgHtmlJG.cc \
    ImgAVFmtParser.cc \
    ImgIdx.cc  \
    ImgKey.cc \
    ImgThumbGen.cc 

imgcat_CPPFLAGS = $(AM_CPPFLAGS)
imgcat_CXXFLAGS = $(AM_CXXFLAGS) $(AVFORMAT_CFLAGS) $(AVUTIL_CFLAGS) $(FFMPEGTHUMB_CFLAGS)
imgcat_LDFLAGS = $(AM_LDFLAGS) $(FFMPEGTHUMB_LIBS) $(AVCODEC_LIBS) $(AVFORMAT_LIBS) $(AVUTIL_LIBS) -lpng -ljpeg
imgcat_LDADD = $(SAMPLEICC_LIBS) libimgcat.la libimgcat_helper.la
imgcat_SOURCES = imgcat.cc 

imgprextr_CPPFLAGS = $(AM_CPPFLAGS)
imgprextr_CXXFLAGS = $(AM_CXXFLAGS)
imgprextr_LDFLAGS = $(AM_LDFLAGS)
imgprextr_LDADD = $(SAMPLEICC_LIBS) libimgcat_helper.la
imgprextr_SOURCES = imgprextr.cc
