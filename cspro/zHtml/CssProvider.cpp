#include "stdafx.h"
#include "CssProvider.h"
#include "PortableLocalhost.h"


CssProvider::CssProvider(const Html::CSS css, const bool link_to_css)
    :   m_css(css),
        m_linkToCss(link_to_css)
{
}


std::string CssProvider::GetCssForHead()
{
    const std::string css_file_path = Html::GetCSSFilePath(m_css);

    if( m_linkToCss )
    {
        return SO::Concatenate("<link rel=\"stylesheet\" href=\"",
                               PortableLocalhost::CreateFileUrl(css_file_path),
                               "\">\n");
    }

    else
    {
        return SO::Concatenate("<style>\n",
                               FileIO::ReadText(css_file_path),
                               "</style>\n");
    }
}
