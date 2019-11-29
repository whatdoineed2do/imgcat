#ifndef IMG_OUT_H
#define IMG_OUT_H

#include <memory>
#include <list>

#include "ImgIdx.h"
#include "ImgThumbGen.h"


class ImgOut
{
   public:
     virtual ~ImgOut() = default;

     ImgOut(const ImgOut&) = delete;
     ImgOut& operator=(const ImgOut&) = delete;
     ImgOut(const ImgOut&&) = delete;
     ImgOut& operator=(const ImgOut&&) = delete;

     struct Payload {
         Payload(ImgIdx& idx_) : idx(idx_) { }
         Payload(Payload&& rhs_) : idx(rhs_.idx), thumbs(std::move(rhs_.thumbs)) { }

         ImgIdx& idx;
         ImgThumbGens thumbs;  // dont own 
     };
     typedef std::list<ImgOut::Payload>  Payloads;


     virtual std::string  filename() = 0;
     virtual std::string  generate(Payloads&) = 0;

     static ImgOut*  create(const char* type_)  throw (std::range_error);

   protected:
     ImgOut() = default;
};

#endif
