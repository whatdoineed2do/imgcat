#include "ImgOut.h"

#include <iostream>
#include <iomanip>

#include "ImgOutJson.h"
#include "ImgHtml.h"


template <typename T>
using Fctry = T*(*)();


ImgOut*  ImgOut::create(const char* type_)
{
    struct OutGenerators {
        const char*  id;
        Fctry<ImgOut>  create;
    };

    OutGenerators  outgens[] = {
        { ImgOutJsonJS::ID,        []() { return (ImgOut*)new ImgOutJsonJS(); } },
        { ImgOutJson::ID,         []() { return (ImgOut*)new ImgOutJson(); } },
        { ImgHtmlJG::ID,           []() { return (ImgOut*)new ImgHtmlJG(); } },
        { ImgHtmlFlexboxSlide::ID, []() { return (ImgOut*)new ImgHtmlFlexboxSlide(); } },
        { ImgHtmlFlexboxHide::ID,  []() { return (ImgOut*)new ImgHtmlFlexboxHide(); } },
        { ImgHtmlClassic::ID,      []() { return (ImgOut*)new ImgHtmlClassic(); } },
        { nullptr, nullptr }
    };

    const bool options = type_ && strcasecmp("help", type_) == 0; 
    if (options) {
        std::cout << "out options[]=";
    }

    ImgOut*  h = NULL;
    OutGenerators*  p = outgens;
    while (h == NULL && p->id) 
    {
        if (options) {
            std::cout << p->id << " ";
        }
        else {
            if (type_ == NULL || strcasecmp(p->id, type_) == 0) {
                h = p->create();
            }
        }
        ++p;
    }
    if (options) std::cout << std::endl;
    return h;
}
