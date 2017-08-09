# $@	full target name
# $^	all dependacnies
# $?	out of date dependancies
# $<
# $*	target without suffix

%.o	: %.cc	;	$(CXX) -c $(CXXFLAGS) $<
%.o	: %.c	;	$(CC)  -c $(CFLAGS) $<


#DEBUGFLAGS+=-g -pg -DDBX_DEBUGGING_INFO -DXCOFF_DEBUGGING_INFO #-DDEBUG_LOG
CFLAGS=$(DEBUGFLAGS) -I/usr/include/libxml2 -I. -I/home/ray/tools/include -DNEED_UCHAR_UINT_T
CXXFLAGS=$(DEBUGFLAGS) -I/usr/include/ImageMagick-6 -I/usr/include/libxml2 -I. -DHAVE_SAMPLE_ICC -I/home/ray/tools -I/home/ray/tools/include -DNEED_UCHAR_UINT_T -g
LDFLAGS=$(DEBUGFLAGS) -lexiv2 -lMagick++-6.Q16 -lMagickCore-6.Q16 -L/home/ray/tools/lib -lSampleICC -L/home/ray/tools/ThreadPool -Wl,-rpath=/home/ray/tools/ThreadPool -lThreadPool -lffmpegthumbnailer

TARGETS=imgcat imgprextr diptych

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
	$(CXX)  $^ $(LDFLAGS) -o $@ -lffmpegthumbnailer
#$(CXX)  $^ ~/tmp/exiv2-0.21/src/.libs/libexiv2.a -lexpat -lz -o $@


imgprextr:	imgprextr.cc ICCprofiles.o
	$(CXX)  $(CXXFLAGS) $^ $(LDFLAGS) -o $@
#$(CXX)  $^ ~/tmp/exiv2-0.21/src/.libs/libexiv2.a -lexpat -lz -o $@

diptych:		diptych.o
	$(CXX)  $(CXXFLAGS) $^ -lMagick++-6.Q16 -lMagickCore-6.Q16 -o $@

mag:		mag.cc
	$(CXX)  $(CXXFLAGS) $^ ICCprofiles.o $(LDFLAGS) -o $@

imagickICCconvert:	imagickICCconvert.cc
	$(CXX)  $(CXXFLAGS) $^ $(LDFLAGS) -o $@


clean:
	rm -fr *.o core.* $(TARGETS) 
