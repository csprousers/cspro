#pragma once

#include <zSrcMgrO/DesignerApplicationLoader.h>


// --------------------------------------------------------------------------
// APP_LOAD_TODO this functionality should eventually be incorporated
// directly into DesignerApplicationLoader
// --------------------------------------------------------------------------

class DesignerApplicationLoaderWithFileSupport : public DesignerApplicationLoader
{
public:
    DesignerApplicationLoaderWithFileSupport(CAplDoc& application_doc);

    std::shared_ptr<CDataDict> GetDictionary(const std::string& dictionary_file_path) override;

    std::shared_ptr<CDEFormFile> GetFormFile(const std::string& form_file_path) override;

    std::shared_ptr<CTabSet> GetTableSpec(const std::string& table_spec_file_path) override;

private:
    CAplDoc& m_applicationDoc;
    std::vector<std::tuple<std::string, std::shared_ptr<CDataDict>>> m_dictionaries;
    std::vector<std::tuple<std::string, std::shared_ptr<CDEFormFile>>> m_formFiles;
    std::vector<std::tuple<std::string, std::shared_ptr<CTabSet>>> m_tableSpecs;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline DesignerApplicationLoaderWithFileSupport::DesignerApplicationLoaderWithFileSupport(CAplDoc& application_doc)
    :   DesignerApplicationLoader(&application_doc.GetAppObject(), nullptr),
        m_applicationDoc(application_doc),
        m_dictionaries(m_applicationDoc.GetAllDictionaries())
{
}


inline std::shared_ptr<CDataDict> DesignerApplicationLoaderWithFileSupport::GetDictionary(const std::string& dictionary_file_path)
{
    const auto& lookup = std::find_if(m_dictionaries.cbegin(), m_dictionaries.cend(),
                                      [&](const auto& path_and_dictionary) { return SO::EqualsNoCase(dictionary_file_path, std::get<0>(path_and_dictionary)); });

    return ( lookup != m_dictionaries.cend() ) ? std::get<1>(*lookup) :
                                                 ReturnProgrammingError(DesignerApplicationLoader::GetDictionary(dictionary_file_path));
}


inline std::shared_ptr<CDEFormFile> DesignerApplicationLoaderWithFileSupport::GetFormFile(const std::string& form_file_path)
{
    if( m_formFiles.empty() )
        m_formFiles = m_applicationDoc.GetAllFormFiles();

    const auto& lookup = std::find_if(m_formFiles.cbegin(), m_formFiles.cend(),
                                      [&](const auto& path_and_form_file) { return SO::EqualsNoCase(form_file_path, std::get<0>(path_and_form_file)); });

    return ( lookup != m_formFiles.cend() ) ? std::get<1>(*lookup) :
                                              ReturnProgrammingError(DesignerApplicationLoader::GetFormFile(form_file_path));
}


inline std::shared_ptr<CTabSet> DesignerApplicationLoaderWithFileSupport::GetTableSpec(const std::string& table_spec_file_path)
{
    if( m_tableSpecs.empty() )
        m_tableSpecs = m_applicationDoc.GetAllTableSpecs();

    const auto& lookup = std::find_if(m_tableSpecs.cbegin(), m_tableSpecs.cend(),
                                      [&](const auto& path_and_table_spec) { return SO::EqualsNoCase(table_spec_file_path, std::get<0>(path_and_table_spec)); });

    return ( lookup != m_tableSpecs.cend() ) ? std::get<1>(*lookup) :
                                               ReturnProgrammingError(DesignerApplicationLoader::GetTableSpec(table_spec_file_path));
}
