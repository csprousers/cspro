#pragma once

#include <zHtml/zHtml.h>

namespace Html { enum class CSS; }


// --------------------------------------------------------------------------
// CssProvider provides the CSS distributed with CSPro in the html directory.
//
// If not embedding CSS, a local file server must be running as the link is
// created using PortableLocalhost::CreateFileUrl.
//
// The CssProvider::GetCssForHead method is virtual so that users do not
// need to depend on zHtml.
// --------------------------------------------------------------------------

class ZHTML_API CssProvider
{
public:
    CssProvider(Html::CSS css, bool embed_css);
    virtual ~CssProvider() { }

    virtual std::string GetCssForHead();

private:
    Html::CSS m_css;
    bool m_embedCss;
};
