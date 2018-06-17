#include "ImgKey.h"

#include <sstream>


ImgKey::Dt::Dt(const char* hms_, const char* ms_)  throw (std::invalid_argument)
    : hms(hms_), ms(ms_)
{
    struct tm  tm;
    memset(&tm, 0, sizeof(tm));
    if (strptime(hms_, "%Y:%m:%d %T", &tm) == NULL) {
	std::ostringstream  err;
	err << "invalid time '" << hms_ << "'";
	throw std::invalid_argument(err.str());
    }
    _hms = mktime(&tm);
}

ImgKey::Dt::Dt(const time_t& t_)  throw (std::invalid_argument)
    : _hms(t_)
{
    struct tm  tm;
    localtime_r(&_hms, &tm);
    char  tmp[21];
    memset(tmp, 0, sizeof(tmp));
    strftime(tmp, 20, "%Y:%m:%d %T", &tm);
    hms = tmp;
}

ImgKey::Dt::Dt(const Dt&& rhs_)
{
    hms = std::move(rhs_.hms);
    ms = std::move(rhs_.ms);
    _hms = rhs_._hms;
}

const ImgKey::Dt&  ImgKey::Dt::operator=(const Dt&& rhs_)
{
    if (this != &rhs_) {
	hms = std::move(rhs_.hms);
	ms = std::move(rhs_.ms);
	_hms = rhs_._hms;
    }

    return *this;
}

const ImgKey::Dt&  ImgKey::Dt::operator=(const Dt& rhs_)
{
    if (this != &rhs_) {
	hms = rhs_.hms;
	ms = rhs_.ms;
	_hms = rhs_._hms;
    }
    return *this;
}

const std::string  ImgKey::_ino2str(const ino_t& ino_)
{
    std::stringstream  conv;
    conv << ino_;
    return conv.str();
}

