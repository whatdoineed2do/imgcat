/*  $Id: ImgKey.h,v 1.1 2011/10/30 17:33:53 ray Exp $
 */

#ifndef IMG_KEY_H
#define IMG_KEY_H

#include <sys/types.h>

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
using namespace  std;


class ImgKey
{
  public:
    struct Dt
    { 
	Dt(const char* hms_, const char* ms_)  throw (std::invalid_argument)
	    : hms(hms_), ms(ms_)
	{
	    struct tm  tm;
	    memset(&tm, 0, sizeof(tm));
	    if (strptime(hms_, "%Y:%m:%d %T", &tm) == NULL) {
		ostringstream  err;
		err << "invalid time '" << hms_ << "'";
		throw invalid_argument(err.str());
	    }
	    _hms = mktime(&tm);
	}

	Dt(const time_t& t_)  throw (std::invalid_argument)
	    : _hms(t_)
	{ }

	Dt(const Dt& rhs_) : hms(rhs_.hms), ms(rhs_.ms), _hms(rhs_._hms)  { }

	const Dt&  operator=(const Dt& rhs_)
	{
	    if (this != &rhs_) {
		hms = rhs_.hms;
		ms = rhs_.ms;
		_hms = rhs_._hms;
	    }
	    return *this;
	}

	string  hms;
	string  ms;
	time_t  _hms;

	bool  operator==(const Dt& rhs_) const
	{
	    return _hms == rhs_._hms && ms == rhs_.ms;
	}

	bool  operator<(const Dt& rhs_) const
	{
	    return _hms < rhs_._hms;
	}
    };
    

    ImgKey(const char* mfctr_, const char* mdl_, const char* sn_,
	   const char* dtorg_, const char* dtorgSub_)  throw (std::invalid_argument)
	: mfctr(mfctr_), mdl(mdl_), sn(sn_),
          dt(dtorg_, dtorgSub_)
    { }

    // ctr for images that have no meta info
    ImgKey(const ino_t&  ino_, const time_t&  mtime_)  throw (std::invalid_argument)
	: sn(ImgKey::_ino2str(ino_)), dt(mtime_)
    { }

    ImgKey(const ImgKey& rhs_)
    	: mfctr(rhs_.mfctr), mdl(rhs_.mdl), sn(rhs_.sn),
          dt(rhs_.dt)
    { }

    const string  mfctr;
    const string  mdl;
    const string  sn;

    const ImgKey::Dt   dt;


    bool  operator==(const ImgKey& rhs_) const
    {
	return dt == rhs_.dt && sn == rhs_.sn && mfctr == rhs_.mfctr && mdl == rhs_.mdl;
    }

    bool  operator<(const ImgKey& rhs_) const
    {
	return dt < rhs_.dt;
	//return *this == rhs_ && dt < rhs_.dt;
    }

  private:
    void operator=(const ImgKey&);

    static const string  _ino2str(const ino_t& ino_)
    {
	stringstream  conv;
	conv << ino_;
	return conv.str();
    }

};

inline
ostream& operator<<(ostream& os_, const ImgKey& obj_)
{
    return os_ << obj_.mfctr << ", " << obj_.mdl << " #" << obj_.sn << " [" << obj_.dt._hms << "]";
}

#endif
