#include "PreviewInfo.h"

#include <array>


std::ostream& operator<<(std::ostream& os_, const PreviewInfo& obj_)
{
    const char*  s = obj_.seperator ? obj_.seperator : " ";
    std::string  tmp;
    auto  fmt = [&s](const std::string& s_, bool first_=false) {
        std::ostringstream  f;
        if (s_.empty()) {
            return s_;
        }
        if (first_) {
            f << s_;
        }
        else {
            f << s << s_;
        }
        return f.str();
    };
    return os_ << fmt(obj_.dateorig, true) << fmt(obj_.mftr) << fmt(obj_.camera) << fmt(obj_.sn) << fmt(obj_.shuttercnt);
}

PreviewInfo::PreviewInfo(const Exiv2::Image& img_) : seperator(nullptr)
{
    typedef Exiv2::ExifData::const_iterator (*EasyAccessFct)(const Exiv2::ExifData& ed);
    struct _EasyAccess {
	EasyAccessFct find;
	std::string&  target;
    };

    std::array  eatags {
	_EasyAccess{ Exiv2::make,         mftr   },
	_EasyAccess{ Exiv2::model,        camera },
	_EasyAccess{ Exiv2::serialNumber, sn     }
    };


    struct _MiscTags {
        const char*  tag;
        std::string&  s;
    };

    std::array  misctags {
        _MiscTags{ "Exif.Image.DateTime",         datemod },
        _MiscTags{ "Exif.Photo.DateTimeOriginal", dateorig },
        _MiscTags{ "Exif.Nikon3.ShutterCount",    shuttercnt }
    };

    const Exiv2::ExifData&  ed = img_.exifData();
    if (ed.empty()) {
        return;
    }

    Exiv2::ExifData::const_iterator  exif;
    std::for_each(eatags.begin(), eatags.end(), [&ed, &exif](auto& ep) {
	if ( (exif = ep.find(ed)) != ed.end())  {
	    ep.target = exif->print(&ed);
	}
    });

    if (mftr == "NIKON CORPORATION") {
        mftr.clear();  // the model has "NIKON ..."
    }
    else {
        if (mftr.length() > 10) {
            mftr.replace(10, mftr.length(), 3, '.');
        }
    }

    std::for_each(misctags.begin(), misctags.end(), [&ed, &exif](auto& mp) {
        if ( (exif = ed.findKey(Exiv2::ExifKey(mp.tag)) ) != ed.end()) {
            mp.s = exif->print(&ed);
	}
    });
}
