#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <limits.h>

#include <string>
#include <iostream>
#include <cassert>

using namespace  std;

#include <exiv2/exiv2.hpp>
#include <Magick++.h>

#include "ICCprofiles.h"


int main(int argc, char* const argv[])
{
    if (argc != 2) {
	cerr << "what file?" << endl;
	return 1;
    }

    int  fd;
    if ( (fd = open(argv[1], O_RDONLY)) <0) {
	cerr << "unable to open data file - " << strerror(errno) << endl;
    }
    struct stat  st;
    fstat(fd, &st);
    const size_t&  datasz_ = st.st_size;
    char*  data_ = new char[datasz_];
    read(fd, data_, datasz_);


    try
    {
#if 0
	Magick::Blob   blob(data_, datasz_);
	Magick::Image  magick(blob);
#else
	Magick::Image  magick(argv[1]);
#endif

#if 0
	Magick::Blob   profile = magick.iccColorProfile();
	cout << "data size = " << datasz_ << endl;
	cout << "profile size = " << profile.length() << endl;
	if (profile.length() == 0) {
	    // ??? lets be trying ICC/ICM
	    const Magick::Blob   icc  = magick.profile("ICC");
	    const Magick::Blob   icm  = magick.profile("ICM");

	    cout << "  explicit icc len=" << icc.length() << endl
	         << "  explicit icm len=" << icm.length() << endl;

	mode_t  msk = umask(0);
	umask(msk);
	int  fd;
	if ( (fd = open("icc.icc", O_CREAT | O_TRUNC, 0666 & ~msk)) < 0) {
	    cerr << "  failed to create icc profile - " << strerror(errno) << endl;
	}
	else 
	{
	    if ( write(fd, icc.data(), icc.length()) != profile.length()) {
		cerr << "  failed to write profile data - " << strerror(errno) << endl;
	    }
	}
	close(fd);

	if ( (fd = open("icm.icc", O_CREAT | O_TRUNC, 0666 & ~msk)) < 0) {
	    cerr << "  failed to create icm profile - " << strerror(errno) << endl;
	}
	else 
	{
	    if ( write(fd, icm.data(), icm.length()) != profile.length()) {
		cerr << "  failed to write profile data - " << strerror(errno) << endl;
	    }
	}
	close(fd);
	    return 1;
	}

	mode_t  msk = umask(0);
	umask(msk);
	int  fd;
	if ( (fd = open("profile.icc", O_CREAT | O_TRUNC, 0666 & ~msk)) < 0) {
	    cerr << "  failed to create profile - " << strerror(errno) << endl;
	    return 1;
	}

	if ( write(fd, profile.data(), profile.length()) != profile.length()) {
	    cerr << "  failed to write profile data - " << strerror(errno) << endl;
	}
	close(fd);
#endif

	magick.profile("ICC", Magick::Blob(NULL, 0) );

	magick.renderingIntent(Magick::PerceptualIntent);

	magick.profile("ICC", Magick::Blob(NKAdobe, sizeof(NKAdobe)) );

	const Magick::Blob  targetICC(NKsRGB, sizeof(NKsRGB));
	magick.profile("ICC", targetICC);
	magick.iccColorProfile(targetICC);

	//magick.quality(100);
	char  path[1024];
	sprintf(path, "%s-converted.jpg", argv[1]);
	magick.write(path);
    }
    catch (const std::exception& ex)
    {
	cerr << "failed to magick - " << ex.what() << endl;
    }

    delete []  data_;
    return 0;
}
