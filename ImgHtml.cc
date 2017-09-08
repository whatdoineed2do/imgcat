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

std::string  ImgHtmlClassic::generate(ImgIdxs&  idxs_, const ImgThumbGens& tgs_)
{
    std::ostringstream  html;

    for (ImgIdxs::const_iterator i=idxs_.begin(); i!=idxs_.end(); ++i) 
    {
        ImgIdx&  idx = **i;
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

        
        for (auto t : tgs_)
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

            delete t;
        }
        if (tk) {
            html << "</tr>";
        }
        html << "</table>\n\n";
        cout << endl;
    }
    //return std::move(html.str());
    return html.str();
}


std::string  ImgHtmlFlexbox::generate(ImgIdxs&  idxs_, const ImgThumbGens& tgs_)
{
    std::ostringstream  html;
    //return std::move(html.str());
    return html.str();
}
