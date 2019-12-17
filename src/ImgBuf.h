#ifndef IMG_BUF_H
#define IMG_BUF_H

#include <sys/types.h>
#include <string.h>

#ifdef NEED_UCHAR_UINT_T
typedef unsigned char  uchar_t;
typedef unsigned int   uint_t;
#endif


namespace ImgCat {
struct _Buf
{
  public:
    _Buf() : buf(NULL), sz(0), bufsz(0) { }
    _Buf(size_t sz_) : buf(NULL), sz(0), bufsz(0) { alloc(sz_); }

    ~_Buf() { free(); }

    uchar_t*  buf;
    size_t    bufsz;
    size_t    sz;

    void  alloc(size_t sz_)
    {
	if (sz_ > sz) {
	    delete [] buf;
	    sz = sz_;
	    bufsz = sz;
	    buf = new uchar_t[sz];
	}
	memset(buf, 0, sz);
    }

    void  free()
    {
	delete []  buf;
	buf = NULL;
	sz = 0;
	bufsz = 0;
    }

    const uchar_t*  copy(uchar_t* buf_, size_t sz_)
    {
	alloc(sz_);
	memcpy(buf, buf_, sz_);
	bufsz = sz_;
	return buf;
    }

    void  clear()
    {
	memset(buf, 0, sz);
    }

    _Buf(const _Buf&) = delete;
    _Buf(_Buf&& rhs_)
    {
        buf = rhs_.buf;
        rhs_.buf = NULL;
        bufsz = rhs_.bufsz;
        sz = rhs_.sz;
    }

    _Buf& operator=(const _Buf&) = delete;
    _Buf& operator=(_Buf&& rhs_)
    {
        if (this != &rhs_) {
          buf = rhs_.buf;
          rhs_.buf = NULL;
          bufsz = rhs_.bufsz;
          sz = rhs_.sz;
        }
        return *this;
    }
};
}

#endif
