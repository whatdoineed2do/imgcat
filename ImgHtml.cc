#include "ImgHtml.h"

#include <iostream>
#include <iomanip>


template <typename T>
using Fctry = T*(*)();


ImgHtml*  ImgHtml::create(const char* type_)
    throw (std::range_error)
{
    struct _HtmlGenerators {
        const char*  id;
        Fctry<ImgHtml>  create;
    };

    _HtmlGenerators  htmlgens[] = {
        { ImgHtmlClassic::ID, []() { return (ImgHtml*)new ImgHtmlClassic(); } },
        { ImgHtmlFlexbox::ID, []() { return (ImgHtml*)new ImgHtmlFlexbox(); } },
        { nullptr, nullptr }
    };

    ImgHtml*  h = NULL;
    _HtmlGenerators*  p = htmlgens;
    while (h == NULL && p->id) {
        if (type_ == NULL || strcasecmp(p->id, type_) == 0) {
            h = p->create();
        }
        ++p;
    }
    return h;
}


////////////////////////////////////////////////////////////////////////////////

std::string  ImgHtmlClassic::generate(ImgHtml::Payloads& payloads_)
{
    std::ostringstream  html;
    html << "<html lang=\"en\"><body>\n";

    for (auto& p : payloads_)
    {
        ImgIdx&        idx    = p.idx;
        ImgThumbGens&  thumbs = p.thumbs;
        

        cout << "  working on [" << setw(3) << idx.size() << "]  " << idx.id << "  " << flush;
        html << "<h2>/<a href=\"" << idx.id << "\">" << idx.id << "</a></h2>";

        if (idx.empty()) {
            cout << '\n';
            continue;
        }

        idx.sort();

        int  tj = 0;
        int  tk = 0;

        ImgIdx::const_iterator  dts = idx.begin();

        html << "<p><sup>" << dts->key.dt.hms;

        advance(dts, idx.size()-1);
        if (dts != idx.end()) {
            html << " .. " << dts->key.dt.hms;
        }
        html << "</sup></p>"
             << "<table cellpadding=0 cellspacing=2 frame=border>";

        
        for (auto& t : thumbs)
        {
            const ImgData&  img = t->img();

            if (tk == 0) {
                 html << "<tr height=" << t->thumbsize << " align=center valign=middle>";
            }

            // insert the caption for the cell
            html << "<td width=" << t->thumbsize 
                 << " title=\"";
            if (!img.rating.empty()) {
                html << "[" << img.rating << "] ";
            }
            html << img.title << " (" << t->idx().key.dt.hms << ") Exif: " << img << "\">"
                 << "<a href=\"" << img.filename << "\"><img src=\"" << t->prevpath() << "\"></a>";

            ImgIdx::Imgs::const_iterator  alts = t->idx().imgs.begin();
            while (alts != t->idx().imgs.end()) {
                const ImgData&  altimg = *alts++;
                html << "<a href=\"" << altimg.filename << "\">. </a>";
            }
            html << "</td>";
                
            if (++tk == 8) {
                html << "</tr>\n";
                tk = 0;
            }
            cout << '.' << flush;
        }
        if (tk) {
            html << "</tr>";
        }
        html << "</table>\n\n";
        cout << endl;
    }
    html << "</body>\n</html>";
    //return std::move(html.str());
    return html.str();
}


std::string  ImgHtmlFlexbox::generate(ImgHtml::Payloads& payloads_)
{
    const char* const  hdr = "<html lang=en>\
<head>\
<style>\
body { margin: 0; background: #333; }\
header { \
  padding: .vw;\
  font-size: 0;\
  display: -ms-flexbox;\
  -ms-flex-wrap: wrap;\
  -ms-flex-direction: column;\
  -webkit-flex-flow: row wrap; \
  flex-flow: row wrap; \
  display: -webkit-box;\
  display: flex;\
}\
header div { \
  -webkit-box-flex: auto;\
  -ms-flex: auto;\
  flex: auto; \
  width: 200px; \
  margin: 0.2vw; \
}\
header div img { \
  width: 100%; \
  height: auto; \
}\
@media screen and (max-width: 400px) {\
  header div { margin: 0; }\
  header { padding: 0; }\
  \
}\
</style> </head> <body> <header>";

    std::ostringstream  html;

    html << hdr;
    for (auto& p : payloads_)
    {
        ImgIdx&        idx    = p.idx;
        ImgThumbGens&  thumbs = p.thumbs;
        

        cout << "  working on [" << setw(3) << idx.size() << "]  " << idx.id << "  " << flush;

        if (idx.empty()) {
            cout << '\n';
            continue;
        }

        idx.sort();


        ImgIdx::const_iterator  dts = idx.begin();

        html << "<p><sup>" << dts->key.dt.hms;

        advance(dts, idx.size()-1);
        if (dts != idx.end()) {
            html << " .. " << dts->key.dt.hms;
        }
        html << "</sup></p>"
             << "<table cellpadding=0 cellspacing=2 frame=border>";

        
        for (auto& t : thumbs)
        {
            const ImgData&  img = t->img();

            if (tk == 0) {
                 html << "<tr height=" << t->thumbsize << " align=center valign=middle>";
            }

            // insert the caption for the cell
            html << "<td width=" << t->thumbsize 
                 << " title=\"";
            if (!img.rating.empty()) {
                html << "[" << img.rating << "] ";
            }
            html << img.title << " (" << t->idx().key.dt.hms << ") Exif: " << img << "\">"
                 << "<a href=\"" << img.filename << "\"><img src=\"" << t->prevpath() << "\"></a>";

            ImgIdx::Imgs::const_iterator  alts = t->idx().imgs.begin();
            while (alts != t->idx().imgs.end()) {
                const ImgData&  altimg = *alts++;
                html << "<a href=\"" << altimg.filename << "\">. </a>";
            }
            html << "</td>";
                
            if (++tk == 8) {
                html << "</tr>\n";
                tk = 0;
            }
            cout << '.' << flush;
        }
        if (tk) {
            html << "</tr>";
        }
        html << "</table>\n\n";
        cout << endl;
    }
    html << "</body>\n</html>";
    //return std::move(html.str());
    return html.str();
}


std::string  ImgHtmlFlexbox::generate(ImgHtml::Payloads& payloads_)
{
    const char* const  hdr = "<html lang=en>\
<head>\
<style>\
body { margin: 0; background: #333; }\
header { \
  padding: .vw;\
  font-size: 0;\
  display: -ms-flexbox;\
  -ms-flex-wrap: wrap;\
  -ms-flex-direction: column;\
  -webkit-flex-flow: row wrap; \
  flex-flow: row wrap; \
  display: -webkit-box;\
  display: flex;\
}\
header div { \
  -webkit-box-flex: auto;\
  -ms-flex: auto;\
  flex: auto; \
  width: 200px; \
  margin: 0.2vw; \
}\
header div img { \
  width: 100%; \
  height: auto; \
}\
@media screen and (max-width: 400px) {\
  header div { margin: 0; }\
  header { padding: 0; }\
  \
}\
</style> </head> <body> <header>";

    std::ostringstream  html;

    html << hdr;

    html << "</header></body>\n</html>";

    //return std::move(html.str());
    return html.str();
}

    html << "</header></body>\n</html>";

    //return std::move(html.str());
    return html.str();
}
