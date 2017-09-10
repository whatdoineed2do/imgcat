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
        { ImgHtmlFlexbox::ID, []() { return (ImgHtml*)new ImgHtmlFlexbox(); } },
        { ImgHtmlClassic::ID, []() { return (ImgHtml*)new ImgHtmlClassic(); } },
        { nullptr, nullptr }
    };

    const bool options = type_ && strcasecmp("help", type_) == 0; 
    if (options) {
        cout << "html options[]=";
    }

    ImgHtml*  h = NULL;
    _HtmlGenerators*  p = htmlgens;
    while (h == NULL && p->id) 
    {
        if (options) {
            cout << p->id << " ";
        }
        else {
            if (type_ == NULL || strcasecmp(p->id, type_) == 0) {
                h = p->create();
            }
        }
        ++p;
    }
    if (options) cout << endl;
    return h;
}


////////////////////////////////////////////////////////////////////////////////

std::string  ImgHtmlClassic::generate(ImgHtml::Payloads& payloads_)
{
    std::ostringstream  html;
    html << "<html><body>\n";

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
            html << img.title << " (" << t->idx().key.dt.hms << ")\nExif: " << img << "\">"
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
    html << "</body></html>";

    //return std::move(html.str());
    return html.str();
}


std::string  ImgHtmlFlexbox::generate(ImgHtml::Payloads& payloads_)
{
    static const char* const  hdr = "\
<html lang=en><meta charset=utf-8><head>\
<style>\
body { margin: 0; background: #333; }\
.flex-container {\
  padding: .vw;\
  font-size: 0;\
  display: -ms-flexbox;\
  -ms-flex-wrap: wrap;\
  -ms-flex-direction: column;\
  -webkit-flex-flow: row wrap;\
  flex-flow: row wrap;\
  display: -webkit-box;\
  display: flex;\
  margin: 0.5vw;\
}\
.flex-item {\
  -webkit-box-flex: auto;\
  -ms-flex: auto;\
  flex: 200px;\
  margin: 0.2vw;\
}\
.bottom-left { \
  position: relative;\
  bottom: 15px;\
  left: 10px;\
  font-size: 10px;\
}\
h2 {\
  padding: .vw;\
  padding-right: 5px;\
  background: #333;\
  text-align: right;\
  color: white;\
  margin: 0;\
}\
p {\
  background: #333;\
  padding-right: 10px;\
  text-align: right;\
  color: white;\
  margin: 0;\
  font-size: 90%;\
}\
p1 {\
  background: #333;\
  padding-left: 5px;\
  text-align: left;\
  color: white;\
  margin: 0;\
  font-size: 80%;\
}\
img {\
  width: 100%;\
  height: auto;\
  border-radius: 3px;\
}\
a {\
  color: white;\
}\
a:hover {\
  color: #333;\
  background-color: white;\
}\
@media screen and (max-width: 400px) {\
  header div { margin: 0; }\
  header { padding: 0; }\
\
}\
</style>\
</head>\
<body>\n";

    std::ostringstream  html;
    html << hdr;

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


        ImgIdx::const_iterator  dts = idx.begin();

        html << "<p>" << dts->key.dt.hms;
        advance(dts, idx.size()-1);
        if (dts != idx.end()) {
            html << " .. " << dts->key.dt.hms;
        }
        html << "</p>\n";

        const ImgIdx::Stats  stats = idx.stats();
        {
            struct P {
                const char*  category;
                const ImgIdx::Stats::_Stat&  stat;
            }
            all[] = {
                { "cameras",    stats.camera },
                { "lenses",     stats.lens },
                { "focal lens", stats.focallen },
                { NULL,         stats.camera },
            };
            P*  p = all;
            while (p->category) {
                html << "<p1>";
                for (const auto&  si : p->stat) {
                    html << "<b>" << si.first << "</b> #" << si.second << " ";
                }
                html << "</p1><br>\n";
                ++p;
            }
        }
        html << "<div class=flex-container>\n";
        
        for (auto& t : thumbs)
        {
            const ImgData&  img = t->img();

            // insert the caption for the cell
            html << " <div class=flex-item  title=\"";
            if (!img.rating.empty()) {
                html << "[" << img.rating << "] ";
            }
            html << img.title << "  (" << t->idx().key.dt.hms << ")\n"
                 << "Exif: " << img << "\">\n"
                 << "  <a href=\"" << img.filename << "\"><img src=\"" << t->prevpath() << "\"></a>";

            if (t->idx().imgs.size() > 1)
            {
                html << "   <div class=bottom-left>";
                ImgIdx::Imgs::const_iterator  alts = t->idx().imgs.begin();
                while (alts != t->idx().imgs.end()) {
                    const ImgData&  altimg = *alts++;
                    html << "<a href=\"" << altimg.filename << "\"># </a>";
                }
                html << "\n  </div>";
            }
            html << "</div>";
                
            cout << '.' << flush;
        }
        html << "</div>\n\n";
        cout << endl;
    }

    html << "</body></html>";
    return std::move(html.str());
}
