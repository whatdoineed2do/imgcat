# $@	full target name
# $^	all dependacnies
# $?	out of date dependancies
# $<
# $*	target without suffix

%.o	: %.cc	;	$(CXX) -c $(CXXFLAGS) $<
%.o	: %.c	;	$(CC)  -c $(CFLAGS) $<


# uses sampleICC to perform ICC conversions; not in Fedora - get and install
# https://www.color.org/sample.icc.xalter
# https://sampleicc.sourceforge.net
#
# ./configure --prefix=$(pwd)/../SampleICC --disable-shared --enable-static
# make install -k

SAMPLEICC_HOME=SampleICC

#DEBUGFLAGS+=-g -pg -DDBX_DEBUGGING_INFO -DXCOFF_DEBUGGING_INFO #-DDEBUG_LOG

CXXFLAGS=$(DEBUGFLAGS) $(shell pkg-config Magick++ --cflags) $(pkg-config exiv2 --cflags) -I. -DHAVE_SAMPLE_ICC $(shell PKG_CONFIG_PATH=$(SAMPLEICC_HOME)/lib/pkgconfig pkg-config sampleicc --cflags) -DNEED_UCHAR_UINT_T -g

LDFLAGS=$(DEBUGFLAGS) $(shell pkg-config Magick++ --libs) $(shell pkg-config exiv2 --libs) $(shell PKG_CONFIG_PATH=$(SAMPLEICC_HOME)/lib/pkgconfig pkg-config sampleicc --libs)


TARGETS=imgcat imgprextr

all:	objs $(TARGETS)
objs:	ICCprofiles.o ImgKey.o ImgIdx.o ImgExifParser.o imgcat.o diptych.o


ICCprofiles.o:	ICCprofiles.c ICCprofiles.h

imgcat.o:	imgcat.cc ICCprofiles.h ImgData.h ImgExifParser.h Img.h ImgIdx.h ImgKey.h
ImgExifParser.o:	ImgData.h ImgExifParser.cc ImgExifParser.h Img.h ImgKey.h
ImgIdx.o:	ImgData.h ImgIdx.cc ImgIdx.h ImgKey.h
ImgKey.o:	ImgKey.cc ImgKey.h
imgprextr.o:	imgprextr.cc

imgcat.debug:		ImgKey.o ImgIdx.o ImgExifParser.o ICCprofiles.o
	$(CXX)  -g -DDEBUG_LOG $(DEBUGFLAGS) $(CXXFLAGS) $^ imgcat.cc $(LDFLAGS) -o $@

imgcat:		ImgKey.o ImgIdx.o ImgExifParser.o ICCprofiles.o imgcat.o
	$(CXX)  $^ $(LDFLAGS) -o $@ -lffmpegthumbnailer -lpthread

imgcat-lp:		ImgKey.o ImgIdx.o ImgExifParser.o ICCprofiles.o imgcat.o
	$(CXX)  $^ $(LDFLAGS) -o $@ -lffmpegthumbnailer  -L/home/ray/tools/ThreadPool -Wl,-rpath=/home/ray/tools/ThreadPool -lThreadPool -lffmpegthumbnailer

imgprextr:	imgprextr.cc ICCprofiles.o
	$(CXX)  $(CXXFLAGS) $^ $(LDFLAGS) -o $@


imagickICCconvert:	imagickICCconvert.cc
	$(CXX)  $(CXXFLAGS) $^ $(LDFLAGS) -o $@


clean:
	rm -fr *.o core.* $(TARGETS) 
