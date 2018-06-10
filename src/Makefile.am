ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = -DHAVE_SAMPLE_ICC -DNEED_UCHAR_UINT_T
AM_CXXFLAGS = $(IMAGEMAGICK_CFLAGS) $(EXIV_CFLAGS) $(SAMPLEICC_CFLAGS) -Wno-deprecated
AM_LDFLAGS = $(IMAGEMAGICK_LIBS) $(EXIV_LIBS) 


noinst_LTLIBRARIES = libimgcat.la

bin_PROGRAMS = imgcat imgprextr

libimgcat_la_CPPFLAGS = $(AM_CPPFLAGS)
libimgcat_la_CXXFLAGS = $(AM_CXXFLAGS) $(AVFORMAT_CFLAGS) $(AVUTIL_CFLAGS) $(FFMPEGTHUMB_CFLAGS)
libimgcat_la_LDFLAGS = $(AM_LDFLAGS) $(AVFORMAT_LIBS) $(AVUTIL_LIBS) $(FFMPEGTHUMB_LIBS)
libimgcat_la_LIBADD = $(SAMPLEICC_LIBS)
libimgcat_la_SOURCES = \
    ICCprofiles.c ICCprofiles.h \
    ImgData.cc ImgData.h \
    Img.h ImgKey.h ImgData.h ImgIdx.h ImgExifParser.h ImgHtml.h ImgThumbGen.h \
    ImgMetaParser.cc ImgMetaParser.h Img.h ImgKey.h ImgData.h \
    ImgExifParser.cc ImgExifParser.h ImgMetaParser.h Img.h ImgKey.h ImgData.h \
    ImgHtml.cc ImgHtml.h ImgIdx.h ImgKey.h ImgData.h ImgThumbGen.h \
    ImgAVFmtParser.cc \
    ImgIdx.cc ImgIdx.h ImgKey.h ImgData.h \
    ImgKey.cc ImgKey.h \
    ImgThumbGen.cc ImgThumbGen.h ImgIdx.h ImgKey.h ImgData.h ICCprofiles.h

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
