#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>

#include <iostream>
using namespace  std;

#include <Magick++.h>



int main()
{
    struct stat  st;
    int  fd;

    if ( (fd = open("NKsRGB.icm", O_RDONLY)) <0) {
	cerr << "unable to open original ICC file - " << strerror(errno) << endl;
	return 1;
    }
    fstat(fd, &st);
    const size_t  sRGBsz = st.st_size;
    char*  sRGB = new char[sRGBsz];
    read(fd, sRGB, sRGBsz);
    close(fd);


    if ( (fd = open("NKAdobe.icm", O_RDONLY)) <0) {
	cerr << "unable to open target ICC file - " << strerror(errno) << endl;
	return 1;
    }
    fstat(fd, &st);
    const size_t  aRGBsz = st.st_size;
    char*  aRGB = new char[aRGBsz];
    read(fd, aRGB, aRGBsz);
    close(fd);


    try
    {
	Magick::Image  magick("adobe.jpg");

	magick.renderingIntent(Magick::PerceptualIntent);

	magick.profile("ICC", Magick::Blob(aRGB, aRGBsz));

	const Magick::Blob  targetICC(sRGB, sRGBsz);
	magick.profile("ICC", targetICC);
	magick.iccColorProfile(targetICC);

	magick.write("srgb.jpg");
    }
    catch (const std::exception& ex)
    {
	cerr << "failed to magick - " << ex.what() << endl;
    }

    delete []  sRGB;
    delete []  aRGB;

    return 0;
}
