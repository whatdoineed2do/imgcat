#include <iostream>
#include <array>

#include <Magick++.h>
#include <exiv2/exiv2.hpp>


int main(int argc, char* const argv[])
{
    try
    {
	Magick::InitializeMagick(nullptr);
	std::cout << "using " << MagickCore::GetMagickVersion(nullptr) << "\n";

	Magick::Image  img;
	if (argc == 2) {
	    img.read(argv[1]);
	    std::cout << "make=" << img.attribute("EXIF:Make") << "\n";
	}
	else {
	    img = Magick::Image("640x400", "blue");
	}

	img.attribute("EXIF:Make",  "Foo Inc");
	img.attribute("EXIF:Model", "D9000");
	std::cout << "updated make=" << img.attribute("EXIF:Make") << "\n";

	constexpr std::array  output {
	    "exif.png",
	    "exif.jpg"
	};
	std::for_each(output.begin(), output.end(), [&img](const auto& path) {
	    Magick::Image(img).write(path);
	});

	{

	    img.magick("PNG");
	    Magick::Blob  blob;
	    img.write(&blob);

	    try
	    {
		std::cout << "creating meta via exiv2" << std::endl;
		auto  exiv = Exiv2::ImageFactory::open((const Exiv2::byte*)blob.data(), blob.length());

		Exiv2::ExifData  exif;
		exif["Exif.Image.Orientation"] = 2;

		exiv->setExifData(exif);
		//exiv->exifData()["Exif.Image.PageNumber"] = 1; // also ok
		exiv->writeMetadata();

		blob.update(exiv->io().mmap(), exiv->io().size());
		img.read(blob);
		img.magick("JPEG");
		img.write("exiv2.jpg");
	    }
	    catch (const Exiv2::Error& ex) {
		std::cerr << "failed to exiv2 - " << ex.what() << "\n";
	    }
	}
    }
    catch (const std::exception& ex)
    {
	std::cerr << "failed to magick - " << ex.what() << std::endl;
    }

    return 0;
}
