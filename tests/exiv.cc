#include <string.h>

#include <iostream>
#include <algorithm>

#include <exiv2/exiv2.hpp>


namespace ImgExtract
{
template <typename Container>
class back_insert_iterator {
  public:
    using container_type = Container;
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;

    explicit back_insert_iterator(Container& container_) : _container(&container_) { }

    back_insert_iterator& operator=(const Exiv2::Exifdatum& value) {
        _container->add(value);
        return *this;
    }

    // no-ops
    back_insert_iterator& operator++() { return *this; }
    back_insert_iterator& operator*() { return *this; }

  private:
    Container*  _container;
};
}

namespace Exiv2 {
bool operator<(const Exiv2::Exifdatum& l_, const Exiv2::Exifdatum& r_)
{
    return l_.key() < r_.key() || ((l_.key() == r_.key() && l_.toString() < r_.toString()) );
}
}


int main(int argc, char** argv)
{
    int  ret = -1;
    int  arg = 1;

    Exiv2::ExifMetadata  common;

    while (arg < argc)
    {
	const char*  filename = argv[arg++];
	try
	{
	    try
	    {
		auto  orig = Exiv2::ImageFactory::open(filename);
		if (orig.get() == NULL) {
		    std::cerr << "failed to open file - " << filename << std::endl;
		    continue;
		}

		orig->readMetadata();
		if (common.empty()) {
		    const auto&  e = orig->exifData();
		    std::copy(e.begin(), e.end(), std::back_inserter(common));
		}
		else {
		    auto&  e = orig->exifData();
		    e.sortByKey();

		    for (const auto& x : common) {
		        std::cout << "common:" << x.key() << "=" << x.toString() << "\n";
		    }

		    for (const auto& x : e) {
		        std::cout << filename << ":" << x.key() << "=" << x.toString() << "\n";
		    }

		    Exiv2::ExifMetadata  out;
		    std::set_intersection(common.begin(), common.end(),
		                          e.begin(), e.end(), std::back_inserter(out));

		    common = std::ref(out);
		}
		common.sort();


		Exiv2::PreviewManager  prevldr(*orig);
		Exiv2::PreviewPropertiesList  prevs = prevldr.getPreviewProperties();
		if (prevs.empty()) {
		    std::cout << filename << " no preview images" << std::endl;
		}
		else
		{
		    const Exiv2::PreviewImage  preview = prevldr.getPreviewImage(prevs.back());
		}
		ret = 0;
	    }
	    catch (
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
	    const Exiv2::Error&
#else
	    const Exiv2::AnyError&
#endif
	        e)
	    {
		std::cerr << "unable to extract thumbnail from " << filename << " - " << e << std::endl;
	    }
	}
	catch (const std::exception& e)
	{
	    std::cerr << "unexpected exception - " << e.what() << std::endl;
	}
    }

    std::cout << "common exif (" << common.size() << "):\n";
    for (const auto& e : common) {
        std::cout << e.key() << "=" << e.toString() << "\n";
    }

    return ret;
}
