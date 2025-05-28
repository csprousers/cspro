#pragma once

#include <zMapping/zMapping.h>
#include <zMapping/IMapUI.h>

class MappingProperties;


// --------------------------------------------------------------------------
// HTML-based implementation of mapping.
// --------------------------------------------------------------------------

class ZMAPPING_API HtmlMapUI : public IMapUI
{
public:
    HtmlMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties);

protected:
    std::string GetUrlOfMapHtml() const;
    std::string GetUrlForUrlOrFile(const std::string& url_or_file_path);

protected:
    cs::non_null_shared_or_raw_ptr<const MappingProperties> m_mappingProperties;
};
