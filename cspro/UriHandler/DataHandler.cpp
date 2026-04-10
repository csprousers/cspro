#include "stdafx.h"
#include "UriHandler.h"
#include <zUtilO/ConnectionString.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/WindowHelpers.h>
#include <zDataO/ConnectionStringProperties.h>


namespace Pre81
{
    constexpr std::string_view DataViewerPropertyData_sv        = "file";
    constexpr std::string_view DataViewerPropertyDictionary_sv  = "dcf";
}


void UriHandlerApp::HandleDataUri(std::string uri)
{
    ASSERT(CustomUri::UsesCSProScheme(uri, CustomUri::UriType::Data));

    // if Data Manager is already open, open the data source in that process
    CWnd* const data_manager_instance = CWnd::FindWindow(IMSA_WNDCLASS_DATAMANAGER, nullptr);

    if( data_manager_instance != nullptr )
    {
        IMSASendMessage(IMSA_WNDCLASS_DATAMANAGER, WM_IMSA_FILEOPEN, UTF8_TODO::GetWide(uri));
        data_manager_instance->SetForegroundWindow();
    }

    // otherwise launch a new instance of Data Manager
    else
    {
        CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::DataManager, std::move(uri), true);
    }
}


void UriHandlerApp::ProcessPre81DataViewerProperties(const std::map<std::string, std::string>& properties)
{
    const std::string* file_path = nullptr;
    const std::string* dictionary_file_path = nullptr;
    std::vector<std::tuple<std::string, std::string>> reformatted_properties;

    for( const auto& [attribute, value] : properties )
    {
        std::string_view attribute_to_use_sv = attribute;

        if( attribute == Pre81::DataViewerPropertyData_sv )
        {
            file_path = &value;
        }

        else if( attribute == Pre81::DataViewerPropertyDictionary_sv )
        {
            dictionary_file_path = &value;
        }

        else
        {
            reformatted_properties.emplace_back(attribute, value);
        }
    }

    if( file_path == nullptr )
        throw CSProException("The CSPro URI that launches Data Manager must contain the property: " + std::string(Pre81::DataViewerPropertyData_sv));

    // this object exists only to call PropertyString's protected method
    struct ConnectionStringCreator : public PropertyString
    {
        static std::string CreateString(const std::string& file_path, const std::vector<std::tuple<std::string, std::string>>& properties)
        {
            return ToString(file_path, properties);
        }
    };

    const ConnectionString connection_string(ConnectionStringCreator::CreateString(*file_path, reformatted_properties));

    HandleDataUri(CustomUri::CreateDataUri(connection_string, dictionary_file_path));
}
