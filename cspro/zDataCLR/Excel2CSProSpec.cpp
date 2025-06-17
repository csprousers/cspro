#include "Stdafx.h"
#include "Excel2CSProSpec.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/SpecFile.h>
#include <zJson/JsonSpecFile.h>
#include <zAppO/PFF.h>


CREATE_JSON_VALUE(createNewFile)
CREATE_JSON_VALUE(excelConverter)
CREATE_JSON_VALUE(modifyAddCases)
CREATE_JSON_VALUE(modifyAddDeleteCases)

enum class CaseManagementNative
{
    CreateNewFile = 0,
    ModifyAddCases = 1,
    ModifyAddDeleteCases = 2
};

CREATE_ENUM_JSON_SERIALIZER(CaseManagementNative,
    { CaseManagementNative::CreateNewFile,        JV::createNewFile },
    { CaseManagementNative::ModifyAddCases,       JV::modifyAddCases },
    { CaseManagementNative::ModifyAddDeleteCases, JV::modifyAddDeleteCases })


namespace Pre80Spec
{
    constexpr std::string_view Excel          = "Excel";
    constexpr std::string_view InputDict      = "InputDict";
    constexpr std::string_view OutputData     = "OutputData";
    constexpr std::string_view StartingRow    = "StartingRow";
    constexpr std::string_view CaseManagement = "CaseManagement";
    constexpr std::string_view RunOnlyIfNewer = "RunOnlyIfNewer";
    constexpr std::string_view Mapping        = "Mapping";

    std::string ConvertFile(InterfaceString file_path);

    System::Collections::Generic::List<CSPro::Data::Excel2CSPro::RecordMapping^>^ ConvertMappings(const std::string& dictionary_file_path,
                                                                                                  const std::vector<std::vector<CString>>& mapping_lines);
}

namespace
{
    std::unique_ptr<JsonSpecFile::Reader> CreateJsonReader(InterfaceString file_path)
    {
        const std::string file_contents = FileIO::ReadText(file_path);

        // see if the file contains one of the pre-8.0 spec commands
        for( const std::string_view& command_sv : { Pre80Spec::Excel,
                                                    Pre80Spec::InputDict,
                                                    Pre80Spec::OutputData,
                                                    Pre80Spec::StartingRow,
                                                    Pre80Spec::CaseManagement,
                                                    Pre80Spec::RunOnlyIfNewer,
                                                    Pre80Spec::Mapping } )
        {
            if( SO::StartsWithNoCase(file_contents, command_sv) )
                return JsonSpecFile::CreateReader(file_path, Pre80Spec::ConvertFile(file_path));
        }

        return JsonSpecFile::CreateReader(std::move(file_path), file_contents);
    }
}


CSPro::Data::Excel2CSPro::Spec::Spec()
{
    StartingRow = 2;
    CaseManagement = CSPro::Data::Excel2CSPro::CaseManagement::CreateNewFile;
    RunOnlyIfNewer = false;

    Mappings = gcnew System::Collections::Generic::List<RecordMapping^>();
}


void CSPro::Data::Excel2CSPro::Spec::Load(System::String^ file_path)
{
    try
    {
        auto json_reader = CreateJsonReader(clr_helpers::to_string(file_path));

        try
        {
            json_reader->CheckVersion();
            json_reader->CheckFileType(JV::excelConverter);

            if( json_reader->Contains(JK::excel) )
                ExcelFilename = gcnew System::String(json_reader->GetAbsolutePath(JK::excel).c_str());

            if( json_reader->Contains(JK::dictionary) )
                DictionaryFilename = gcnew System::String(json_reader->GetAbsolutePath(JK::dictionary).c_str());

            if( json_reader->Contains(JK::output) )
            {
                OutputConnectionString = gcnew CSPro::Util::ConnectionString(gcnew System::String(json_reader->Get<CString>(JK::output)));
                OutputConnectionString->AdjustRelativePath(System::IO::Path::GetDirectoryName(file_path));
            }

            StartingRow = json_reader->GetOrDefault<int>(JK::startingRow, StartingRow);
            CaseManagement = (CSPro::Data::Excel2CSPro::CaseManagement)json_reader->GetOrDefault<CaseManagementNative>(JK::caseManagement, (CaseManagementNative)CaseManagement);
            RunOnlyIfNewer = json_reader->GetOrDefault<bool>(JK::runOnlyIfNewer, RunOnlyIfNewer);

            bool errors_processing_mappings = false;

            for( const auto& record_node : json_reader->GetArrayOrEmpty(JK::records) )
            {
                try
                {
                    auto record_mapping = gcnew RecordMapping;

                    record_mapping->RecordName = gcnew System::String(record_node.Get<CString>(JK::name));

                    // worskheet
                    {
                        auto worksheet_node = record_node.Get(JK::worksheet);

                        if( worksheet_node.Contains(JK::name) )
                            record_mapping->WorksheetName = gcnew System::String(worksheet_node.Get<CString>(JK::name));

                        record_mapping->WorksheetIndex = worksheet_node.Get<int>(JK::index);
                    }

                    for( const auto& item_node : record_node.GetArrayOrEmpty(JK::items) )
                    {
                        auto item_mapping = gcnew ItemMapping;

                        item_mapping->ItemName = gcnew System::String(item_node.Get<CString>(JK::name));

                        if( item_node.Contains(JK::occurrence) )
                            item_mapping->Occurrence = item_node.Get<int>(JK::occurrence);

                        item_mapping->ColumnIndex = item_node.Get<int>(JK::column);

                        record_mapping->ItemMappings->Add(item_mapping);
                    }

                    Mappings->Add(record_mapping);
                }

                catch( const JsonParseException& )
                {
                    errors_processing_mappings = true;
                }
            }

            if( errors_processing_mappings )
                json_reader->LogWarning("Some mappings were not included due to errors in the file");
        }

        catch( const CSProException& exception )
        {
            json_reader->GetMessageLogger().RethrowException(clr_helpers::to_string(file_path), exception);
        }

        json_reader->GetMessageLogger().DisplayWarnings();
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(clr_helpers::to_SystemString(exception.what()));
    }
}


namespace
{
    void WriteMappings(JsonWriter& json_writer,
                       System::Collections::Generic::List<CSPro::Data::Excel2CSPro::RecordMapping^>^ mappings)
    {
        json_writer.BeginArray(JK::records);

        for( int i = 0; i < mappings->Count; ++i )
        {
            auto record_mapping = mappings[i];

            json_writer.BeginObject();

            json_writer.Write(JK::name, clr_helpers::to_wstring(record_mapping->RecordName));

            // worksheet
            {
                json_writer.BeginObject(JK::worksheet);

                if( record_mapping->WorksheetName != nullptr )
                    json_writer.Write(JK::name, clr_helpers::to_wstring(record_mapping->WorksheetName));

                json_writer.Write(JK::index, record_mapping->WorksheetIndex);

                json_writer.EndObject();
            }

            // items array
            {
                json_writer.BeginArray(JK::items);

                for( int j = 0; j < record_mapping->ItemMappings->Count; ++j )
                {
                    auto item_mapping = record_mapping->ItemMappings[j];

                    json_writer.BeginObject();

                    json_writer.Write(JK::name, clr_helpers::to_wstring(item_mapping->ItemName));

                    if( item_mapping->Occurrence.HasValue )
                        json_writer.Write(JK::occurrence, item_mapping->Occurrence.Value);

                    json_writer.Write(JK::column, item_mapping->ColumnIndex);

                    json_writer.EndObject();
                }

                json_writer.EndArray();
            }

            json_writer.EndObject();
        }

        json_writer.EndArray();
    }
}


void CSPro::Data::Excel2CSPro::Spec::Save(System::String^ file_path)
{
    try
    {
        const std::string utf8_file_path = clr_helpers::to_string(file_path);
        const std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(utf8_file_path, JV::excelConverter);

        if( ExcelFilename != nullptr )
            json_writer->WriteRelativePath(JK::excel, clr_helpers::to_string(ExcelFilename));

        if( DictionaryFilename != nullptr )
            json_writer->WriteRelativePath(JK::dictionary, clr_helpers::to_string(DictionaryFilename));

        if( OutputConnectionString != nullptr )
        {
            json_writer->Write(JK::output, OutputConnectionString->GetNativeConnectionString().ToRelativeString(PortableFunctions::PathGetDirectory(utf8_file_path), true));
        }

        json_writer->Write(JK::startingRow, StartingRow)
                    .Write(JK::caseManagement, (CaseManagementNative)CaseManagement)
                    .Write(JK::runOnlyIfNewer, RunOnlyIfNewer);

        WriteMappings(*json_writer, Mappings);

        json_writer->EndObject();

        json_writer->Close();

        // save a PFF file for this spec file if one does not already exist
        const std::string pff_file_path = PortableFunctions::PathReplaceFileExtension(utf8_file_path, FileExtensions::Pff);

        if( !PortableFunctions::FileIsRegular(pff_file_path) )
        {
            PFF pff;
            pff.SetPifFileName(UTF8_TODO::GetCString(pff_file_path));
            pff.SetAppType(APPTYPE::EXCEL2CSPRO_TYPE);
            pff.SetAppFName(UTF8_TODO::GetCString(utf8_file_path));
            pff.Save();
        }
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(clr_helpers::to_SystemString(exception.what()));
    }
}


namespace Pre80Spec
{
    std::string ConvertFile(const InterfaceString file_path)
    {
        CSpecFile specfile;

        if( !specfile.Open(file_path.GetString<std::wstring>().c_str(), CFile::modeRead) )
            throw CSProException("Failed to open the Excel to CSPro specification file: %s", file_path.c_str_utf8());

        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

        json_writer->BeginObject();

        json_writer->Write(JK::version, 7.7);
        json_writer->Write(JK::fileType, JV::excelConverter);

        try
        {
            std::string dictionary_file_path;
            std::vector<std::vector<CString>> mapping_lines;

            CString command;
            CString argument;

            while( specfile.GetLine(command, argument) == SF_OK )
            {
                if( SO::EqualsNoCase(command, UTF8_TODO::GetWide(Pre80Spec::Excel)) )
                {
                    json_writer->Write(JK::excel, specfile.EvaluateRelativeFilename(argument));
                }

                else if( SO::EqualsNoCase(command, UTF8_TODO::GetWide(Pre80Spec::InputDict)) )
                {
                    dictionary_file_path = UTF8_TODO::GetUtf8(specfile.EvaluateRelativeFilename(argument));
                    json_writer->Write(JK::dictionary, dictionary_file_path);
                }

                else if( SO::EqualsNoCase(command, UTF8_TODO::GetWide(Pre80Spec::OutputData)) )
                {
                    ConnectionString output_data(UTF8_TODO::GetUtf8(argument));
                    output_data.AdjustRelativePath(PortableFunctions::PathGetDirectory(file_path.GetString<std::string>()));
                    json_writer->Write(JK::output, output_data.ToRelativeString(PortableFunctions::PathGetDirectory(file_path.GetString<std::string>()), true));
                }

                else if( SO::EqualsNoCase(command, UTF8_TODO::GetWide(Pre80Spec::StartingRow)) )
                {
                    json_writer->Write(JK::startingRow, _ttoi(argument));
                }

                else if( SO::EqualsNoCase(command, UTF8_TODO::GetWide(Pre80Spec::CaseManagement)) )
                {
                    for( const auto& potential_argument : { JV::createNewFile, JV::modifyAddCases, JV::modifyAddDeleteCases } )
                    {
                        if( SO::EqualsNoCase(argument, UTF8_TODO::GetWide(potential_argument)) )
                        {
                            json_writer->Write(JK::caseManagement, potential_argument);
                            break;
                        }
                    }
                }

                else if( SO::EqualsNoCase(command, UTF8_TODO::GetWide(Pre80Spec::RunOnlyIfNewer)) )
                {
                    json_writer->Write(JK::runOnlyIfNewer, SO::EqualsNoCase(argument, _T("Yes")));
                }

                else if( SO::EqualsNoCase(command, _T("Mapping")) )
                {
                    std::vector<CString> components = WS2CS_Vector(SO::SplitString(argument, ';'));

                    if( components.size() < 2 || components.size() > 3 )
                        throw CSProException("Unrecognized mapping at line %d", specfile.GetLineNumber());

                    mapping_lines.emplace_back(std::move(components));
                }

                else
                {
                    throw CSProException("Unrecognized command at line %d", specfile.GetLineNumber());
                }
            }

            WriteMappings(*json_writer, ConvertMappings(dictionary_file_path, mapping_lines));

            specfile.Close();
        }

        catch( const CSProException& exception )
        {
            specfile.Close();

            throw CSProException("There was an error reading the Excel to CSPro specification file %s:\n\n%s",
                                 PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str(), exception.what());
        }

        json_writer->EndObject();

        return json_writer->ReleaseString();
    }


    System::Collections::Generic::List<CSPro::Data::Excel2CSPro::RecordMapping^>^ ConvertMappings(const std::string& dictionary_file_path,
                                                                                                  const std::vector<std::vector<CString>>& mapping_lines)
    {
        // ignore any errors while converting the mappings
        auto mappings = gcnew System::Collections::Generic::List<CSPro::Data::Excel2CSPro::RecordMapping^>();

        if( mapping_lines.empty() || dictionary_file_path.empty() )
            return mappings;

        CDataDict dictionary;

        try
        {
            dictionary.Open(dictionary_file_path, true);
        }

        catch( const CSProException& )
        {
            return mappings;
        }

        // first create the record mappings
        std::map<std::wstring, int> record_map; // name, index into mappings list
        std::map<std::wstring, int> item_lines; // name, column index from the file

        for( const std::vector<CString>& components : mapping_lines )
        {
            ASSERT(components.size() >= 2);

            std::wstring name = SO::ToUpper(wstring_view(components.front()));
            int index = _ttoi(CString(components[1]).TrimLeft('@'));

            const CDictRecord* record = dictionary.FindRecord(UTF8_TODO::GetUtf8(name));

            if( record != nullptr )
            {
                if( record_map.find(name) == record_map.cend() )
                {
                    auto record_mapping = gcnew CSPro::Data::Excel2CSPro::RecordMapping();

                    record_mapping->RecordName = gcnew System::String(name.c_str());
                    record_mapping->WorksheetIndex = index;

                    if( components.size() == 3 )
                        record_mapping->WorksheetName = gcnew System::String(components[2]);

                    record_map.try_emplace(std::move(name), mappings->Count);
                    mappings->Add(record_mapping);
                }
            }

            else
            {
                item_lines.try_emplace(std::move(name), index);
            }
        }

        // now add the item mappings
        for( const auto& [name, index] : item_lines )
        {
            wstring_view item_name = name;
            std::optional<std::wstring> record_name;
            std::optional<int> occurrence;

            size_t dot_pos = name.find('.');

            if( dot_pos != std::wstring::npos )
            {
                record_name = item_name.substr(0, dot_pos);
                item_name = item_name.substr(dot_pos + 1);
            }

            size_t left_paren_pos = item_name.find('(');

            if( left_paren_pos != std::wstring::npos )
            {
                size_t right_paren_pos = item_name.find(')', left_paren_pos + 1);

                if( right_paren_pos != std::wstring::npos )
                {
                    occurrence = _ttoi(std::wstring(item_name.substr(left_paren_pos + 1, right_paren_pos - left_paren_pos - 1)).c_str());
                    item_name = item_name.substr(0, left_paren_pos);
                }
            }

            const CDictRecord* dict_record;
            const CDictItem* dict_item;

            if( dictionary.LookupName<CDictItem>(UTF8_TODO::GetUtf8(item_name), nullptr, &dict_record, &dict_item) )
            {
                // if the record name was not provided, use the record name
                // (the record name is not overridden because it had to be explicitly specified for ID items)
                if( !record_name.has_value() )
                    record_name = UTF8_TODO::GetWide(dict_record->GetName());

                const auto& record_lookup = record_map.find(*record_name);

                if( record_lookup != record_map.cend() )
                {
                    auto item_mapping = gcnew CSPro::Data::Excel2CSPro::ItemMapping();

                    item_mapping->ItemName = clr_helpers::to_SystemString(dict_item->GetName());

                    if( occurrence.has_value() )
                        item_mapping->Occurrence = *occurrence;

                    item_mapping->ColumnIndex = index;

                    mappings[record_lookup->second]->ItemMappings->Add(item_mapping);
                }
            }
        }

        return mappings;
    }
}
