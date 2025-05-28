#include "stdafx.h"
#include "HtmlMapUI.h"
#include <zToolsO/Encoders.h>
#include <zHtml/PortableLocalhost.h>


HtmlMapUI::HtmlMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties)
    :   m_mappingProperties(std::move(mapping_properties))
{
}


std::string HtmlMapUI::GetUrlOfMapHtml() const
{
    const std::string& file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Mapping), "logic-map.html");
    return PortableLocalhost::CreateFileUrl(file_path);
}


std::string HtmlMapUI::GetUrlForUrlOrFile(const std::string& url_or_file_path)
{
    if( Encoders::IsDataOrHttpUrl(url_or_file_path) )
        return url_or_file_path;

    return PortableLocalhost::CreateFileUrl(url_or_file_path);
}
