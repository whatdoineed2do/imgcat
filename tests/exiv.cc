#include <unistd.h>
#include <string.h>
#include <iostream>

#include <exiv2/exiv2.hpp>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "what file?" << std::endl;
        return 1;
    }


    int  ret = -1;
    try
    {
        try
        {
            Exiv2::Image::AutoPtr  orig = Exiv2::ImageFactory::open(argv[1]);
            if (orig.get() == NULL) {
                std::cerr << "failed to open file - " << argv[1] << std::endl;
            }
            else
            {
                orig->readMetadata();
                Exiv2::PreviewManager  prevldr(*orig);
                Exiv2::PreviewPropertiesList  prevs = prevldr.getPreviewProperties();
                if (prevs.empty()) {
                    std::cout << argv[1] << " no preview images" << std::endl;
                }
                else
                {
                    const Exiv2::PreviewImage  preview = prevldr.getPreviewImage(prevs.back());
                    orig->exifData();
                }
                ret = 0;
            }
        }
        catch (const Exiv2::AnyError& e)
        {
            std::cerr << "unable to extract thumbnail from " << argv[1] << " - " << e << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "unexpected exception - " << e.what() << std::endl;
    }
    return ret;
}
