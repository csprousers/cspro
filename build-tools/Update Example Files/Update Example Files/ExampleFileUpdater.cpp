#include "stdafx.h"
#include "ExampleFileUpdater.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zUtilO/Interapp.h>
#include <zMessageO/SystemMessageIssuer.h>
#include <zJson/Json.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zTableO/Table.h>
#include <zBridgeO/NPff.h>
#include <zEngineO/SaveArrayFile.h>


namespace
{
    class FileUpdateHelper
    {
    public:
        FileUpdateHelper(ExampleFileUpdater* const example_file_updater, std::string file_path)
            :   m_exampleFileUpdater(example_file_updater),
                m_filePath(std::move(file_path))
        {
        }

        ~FileUpdateHelper()
        {
            m_exampleFileUpdater->MarkAsUpdated(m_filePath);
        }

        [[noreturn]] void LoadError()
        {
            throw CSProException("Error loading %s", m_filePath.c_str());
        }

        [[noreturn]] void SaveError()
        {
            throw CSProException("Error saving %s", m_filePath.c_str());
        }

    private:
        ExampleFileUpdater* m_exampleFileUpdater;
        std::string m_filePath;
    };


    class DummySystemMessageIssuer : public SystemMessageIssuer
    {
    public:
        void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::string& message_text)
        {
            throw CSProException(message_text);
        };

        void OnIssue(const Logic::ParserMessage& /*parser_message*/) override
        {
            throw ProgrammingErrorException();
        }

        void OnAbort(const std::string& /*message_text*/) override
        {
            throw ProgrammingErrorException();
        }
    };

}


void ExampleFileUpdater::Update(const std::string& examples_directory)
{
    DirectoryLister directory_lister(true);

    // dictionaries
    directory_lister.SetNameFilter("*.dcf");

    for( const std::string& dictionary_file_path : directory_lister.GetPaths(examples_directory) )
    {
        FileUpdateHelper fuh(this, dictionary_file_path);

        CDataDict dictionary;
        dictionary.Open(dictionary_file_path);
        dictionary.Save(dictionary_file_path);
    }


    // PFF files
    directory_lister.SetNameFilter("*.pff");

    for( const std::string& pff_file_path : directory_lister.GetPaths(examples_directory) )
    {
        FileUpdateHelper fuh(this, pff_file_path);

        CNPifFile pff;
        pff.SetPifFileName(UTF8_TODO::GetCString(pff_file_path));

        if( !pff.LoadPifFile(true) )
            fuh.LoadError();

        if( !pff.Save(true) )
            fuh.SaveError();
    }


    // save array files
    directory_lister.SetNameFilter("*.sva");

    for( const std::string& save_array_file_path : directory_lister.GetPaths(examples_directory) )
    {
        FileUpdateHelper fuh(this, save_array_file_path);

        std::vector<LogicArray*> logic_arrays;
        auto system_message_issuer = std::make_shared<DummySystemMessageIssuer>();

        SaveArrayFile().ReadArrays(UTF8_TODO::GetWide(save_array_file_path), logic_arrays, system_message_issuer, true);

        SaveArrayFile().WriteArrays(UTF8_TODO::GetWide(save_array_file_path), logic_arrays, system_message_issuer, 0, false);
    }


    // deployment specifications
    directory_lister.SetNameFilter("*.csds");

    for( const std::string& deployment_file_path : directory_lister.GetPaths(examples_directory) )
    {
        FileUpdateHelper fuh(this, deployment_file_path);

        const JsonNode json_node = Json::ParseFile(deployment_file_path);
        ASSERT(json_node.IsObject());

        const std::unique_ptr<JsonFileWriter> json_writer = Json::CreateFileWriter(deployment_file_path);
        json_writer->BeginObject();

        constexpr std::string_view VersionKey_sv = "version";
        ASSERT(VersionKey_sv == JK::version);

        for( const std::string& key : json_node.GetKeys() )
        {
            if( key == VersionKey_sv )
            {
                json_writer->Write(key, Versioning::Number);
            }

            else
            {
                json_writer->Write(key, json_node.Get(key));
            }
        }

        json_writer->EndObject();
    }


    // data entry applications
    // batch applications
    // tabulation applications
    directory_lister.SetNameFilter("*.ent;*.bch;*.xtb");

    for( const std::string& file_path : directory_lister.GetPaths(examples_directory) )
        UpdateApplication(file_path);


    // area names files
    // logic files
    directory_lister.SetNameFilter("*.anm;*.apc");

    for( const std::string& file_path : directory_lister.GetPaths(examples_directory) )
        UpdateObjectlessFile(file_path);


    // show a summary of the files updated
    std::wcout << L"\n\nSuccess\n-------\n";

    for( const auto& [extension, count] : m_updateCounts )
        std::wcout << FormatTextCS2WS(L"%6s : %2d\n", TC::ToWide(extension).c_str(), static_cast<int>(count)).c_str();
}


void ExampleFileUpdater::MarkAsUpdated(const std::string& file_path)
{
    std::wcout << L"Updated: " << TC::ToWide(file_path).c_str() << L"\n";

    std::string extension = SO::ToLower(PortableFunctions::PathGetFileExtension(file_path, true));

    auto extension_lookup = m_updateCounts.find(extension);

    if( extension_lookup == m_updateCounts.cend() )
    {
        m_updateCounts.try_emplace(std::move(extension), 1);
    }

    else
    {
        extension_lookup->second = extension_lookup->second + 1;
    }
}


void ExampleFileUpdater::UpdateApplication(const std::string& application_file_path)
{
    FileUpdateHelper fuh(this, application_file_path);

    Application application;
    application.Open(application_file_path, false, false);

    if( application.GetEngineAppType() == EngineAppType::Entry || application.GetEngineAppType() == EngineAppType::Batch )
    {
        std::vector<std::shared_ptr<CDEFormFile>> form_files;

        for( const CString& form_file_path : application.GetFormFilenames() )
            form_files.emplace_back(UpdateFormFile(UTF8_TODO::GetUtf8(form_file_path)));

        if( !application.GetQuestionTextFilename().IsEmpty() )
            UpdateQuestionText(UTF8_TODO::GetUtf8(application.GetQuestionTextFilename()), form_files);
    }

    else
    {
        ASSERT(application.GetEngineAppType() == EngineAppType::Tabulation);

        for( const CString& tab_spec_file_path : application.GetTabSpecFilenames() )
            UpdateTabSpec(UTF8_TODO::GetUtf8(tab_spec_file_path));
    }

    application.Save(application_file_path);
}


std::shared_ptr<CDEFormFile> ExampleFileUpdater::UpdateFormFile(const std::string& form_file_path)
{
    std::string uppercase_form_file_path = SO::ToUpper(form_file_path);

    const auto& form_file_lookup = m_formFilesProcessed.find(uppercase_form_file_path);

    if( form_file_lookup != m_formFilesProcessed.cend() )
        return form_file_lookup->second;

    FileUpdateHelper fuh(this, form_file_path);

    auto form_file = std::make_unique<CDEFormFile>();

    if( !form_file->Open(UTF8_TODO::GetCString(form_file_path), true) )
        fuh.LoadError();

    auto dictionary = std::make_unique<CDataDict>();
    dictionary->Open(form_file->GetDictionaryFilename());

    form_file->SetDictionary(std::move(dictionary));
    form_file->UpdatePointers();

    form_file->RefreshAssociatedFieldText();

    if( !form_file->Save(UTF8_TODO::GetCString(form_file_path)) )
        fuh.SaveError();

    return m_formFilesProcessed.try_emplace(std::move(uppercase_form_file_path), std::move(form_file)).first->second;
}


void ExampleFileUpdater::UpdateQuestionText(const std::string& qsf_file_path, const std::vector<std::shared_ptr<CDEFormFile>>& form_files)
{
    FileUpdateHelper fuh(this, qsf_file_path);

    class CapiQuestionManagerWithFormFiles : public CapiQuestionManager
    {
    public:
        CapiQuestionManagerWithFormFiles(const std::vector<std::shared_ptr<CDEFormFile>>& form_files)
            :   m_formFiles(form_files)
        {
        }

        std::vector<std::shared_ptr<CDEFormFile>> GetRuntimeFormFiles() const override
        {
            return m_formFiles;
        }

    private:
        const std::vector<std::shared_ptr<CDEFormFile>>& m_formFiles;
    };

    CapiQuestionManagerWithFormFiles capi_question_manager(form_files);
    capi_question_manager.Load(qsf_file_path);
    capi_question_manager.Save(qsf_file_path);
}


void ExampleFileUpdater::UpdateTabSpec(const std::string& tab_spec_file_path)
{
    FileUpdateHelper fuh(this, tab_spec_file_path);

    CSpecFile spec_file;

    if( !spec_file.Open(UTF8_TODO::GetCString(tab_spec_file_path), CFile::modeRead) )
        fuh.LoadError();

    const std::vector<std::wstring> dictionary_file_paths = GetFileNameArrayFromSpecFile(spec_file, CSPRO_DICTS);
    ASSERT(dictionary_file_paths.size() == 1);

    spec_file.Close();

    CTabSet table_set;

    if( !table_set.Open(UTF8_TODO::GetCString(tab_spec_file_path), true) )
        fuh.LoadError();

    CTblPrintFmt* const pTblPrintFmt = dynamic_cast<CTblPrintFmt*>(table_set.GetFmtRegPtr()->GetFmt(FMT_ID_TBLPRINT));

    if( pTblPrintFmt != nullptr )
    {
        pTblPrintFmt->SetPrinterDevice(CString());
        pTblPrintFmt->SetPrinterDriver(CString());
        pTblPrintFmt->SetPrinterOutput(CString());
    }

    if( !table_set.Save(UTF8_TODO::GetCString(tab_spec_file_path), WS2CS(dictionary_file_paths.front())) )
        fuh.SaveError();
}


void ExampleFileUpdater::UpdateObjectlessFile(const std::string& file_path)
{
    constexpr std::string_view VersionText_sv = "Version=CSPro ";

    FileUpdateHelper fuh(this, file_path);

    std::string contents = FileIO::ReadText(file_path);

    for( size_t version_pos = std::string::npos; ( version_pos = contents.find(VersionText_sv, version_pos + 1) ) != std::string::npos; )
    {
        const size_t version_insertion_pos = version_pos + VersionText_sv.length();
        size_t version_end_pos = version_insertion_pos;

        for( ; version_end_pos < contents.length(); ++version_end_pos )
        {
            const char ch = contents[version_end_pos];

            if( !isdigit(ch) && ch != '.' )
                break;
        }

        ASSERT(version_end_pos > version_insertion_pos);
        contents.erase(contents.begin() + version_insertion_pos, contents.begin() + version_end_pos);
        contents.insert(version_insertion_pos, Versioning::NumberText);

        version_pos = version_insertion_pos;
    }

    FileIO::WriteText(file_path, contents, true);
}
