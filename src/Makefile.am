ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CXXFLAGS = $(IMAGEMAGICK_CFLAGS) $(EXIV_CFLAGS) $(SAMPLEICC_CFLAGS) $(AVFORMAT_CFLAGS) $(AVUTIL_CFLAGS) $(FFMPEGTHUMB_CFLAGS) -DNEED_UCHAR_UINT_T -Wno-deprecated
AM_LDFLAGS = $(IMAGEMAGICK_LDFLAGS) $(EXIV_LDFLAGS) $(SAMPLEICC_LDFLAGS) $(AVFORMAT_LDFLAGS) $(AVUTIL_LDFLAGS) $(FFMPEGTHUMB_LDFLAGS)

bin_PROGRAMS = imgcat imgprextr

imgcat_CPPFLAGS = -I$(top_srcdir)/lib
imgcat_CXXFLAGS = $(AM_CXXFLAGS)
imgcat_LDFLAGS = $(AM_LDFLAGS)
imgcat_LDADD =
imgcat_SOURCES = \
    ICCprofiles.c ICCprofiles.h \
    ImgData.cc ImgData.h \
    imgcat.cc Img.h ImgKey.h ImgData.h ImgIdx.h ImgExifParser.h ImgHtml.h ImgThumbGen.h \
    ImgMetaParser.cc ImgMetaParser.h Img.h ImgKey.h ImgData.h \
    ImgExifParser.cc ImgExifParser.h ImgMetaParser.h Img.h ImgKey.h ImgData.h \
    ImgHtml.cc ImgHtml.h ImgIdx.h ImgKey.h ImgData.h ImgThumbGen.h \
    ImgIdx.cc ImgIdx.h ImgKey.h ImgData.h \
    ImgKey.cc ImgKey.h \
    ImgThumbGen.cc ImgThumbGen.h ImgIdx.h ImgKey.h ImgData.h ICCprofiles.h


imgprextr_CPPFLAGS = -I$(top_srcdir)/lib
imgprextr_CXXFLAGS = $(AM_CXXFLAGS)
imgprextr_LDFLAGS = $(AM_LDFLAGS)
imgprextr_LDADD =
imgprextr_SOURCES = \
    ICCprofiles.c ICCprofiles.h \
    imgprextr.cc
