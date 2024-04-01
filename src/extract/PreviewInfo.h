#ifndef PREVIEW_INFO_H
#define PREVIEW_INFO_H

#include <string>
#include <iosfwd>

#include <exiv2/exiv2.hpp>

struct PreviewInfo {
    std::string  dateorig;
    std::string  datemod;
    std::string  mftr;
    std::string  camera;
    std::string  sn;
    std::string  shuttercnt;

    const char* seperator;

    PreviewInfo(const Exiv2::Image&);
    ~PreviewInfo() = default;

    PreviewInfo(PreviewInfo&& rhs_) {
        seperator = rhs_.seperator;
        dateorig = std::move(rhs_.dateorig);
        datemod  = std::move(rhs_.datemod);
        mftr = std::move(rhs_.mftr);
        camera = std::move(rhs_.camera);
        sn = std::move(rhs_.sn);
        shuttercnt = std::move(rhs_.shuttercnt);
    }

    PreviewInfo(const PreviewInfo&) = delete;
    PreviewInfo& operator=(const PreviewInfo&) = delete;
    PreviewInfo& operator=(PreviewInfo&&) = delete;
};

std::ostream& operator<<(std::ostream& os_, const PreviewInfo& obj_);

#endif
