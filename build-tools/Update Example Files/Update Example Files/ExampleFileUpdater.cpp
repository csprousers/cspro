#include "stdafx.h"
#include "ExampleFileUpdater.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/Interapp.h>
#include <zMessageO/SystemMessageIssuer.h>
#include <zJson/Json.h>
#include <ZCAPIO/CapiQuestionManager.h>
#include <zTableO/Table.h>
#include <ZBRIDGEO/npff.h>
#include <zEngineO/SaveArrayFile.h>


namespace
{
    class FileUpdateHelper
    {
    public:
        FileUpdateHelper(ExampleFileUpdater* example_file_updater, std::wstring filename)
            :   m_exampleFileUpdater(example_file_updater),
                m_filename(std::move(filename))
        {
        }

        ~FileUpdateHelper()
        {
            m_exampleFileUpdater->MarkAsUpdated(m_filename);
        }

        [[noreturn]] void LoadError()
        {
            throw CSProException(_T("Error loading %s"), m_filename.c_str());
        }

        [[noreturn]] void SaveError()
        {
            throw CSProException(_T("Error saving %s"), m_filename.c_str());
        }

    private:
        ExampleFileUpdater* m_exampleFileUpdater;
        const std::wstring m_filename;
    };

    
    class DummySystemMessageIssuer : public SystemMessageIssuer
    {
    public:
        void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::wstring& message_text)
        {
            throw CSProException(message_text);
        };
    };

}


void ExampleFileUpdater::Update(const std::wstring& examples_directory)
{
    DirectoryLister directory_lister(true);

    // dictionaries
    directory_lister.SetNameFilter(_T("*.dcf"));

    for( const std::wstring& dictionary_filename : directory_lister.GetPaths(examples_directory) )
    {
        FileUpdateHelper fuh(this, dictionary_filename);

        CDataDict dictionary;
        dictionary.Open(dictionary_filename);
        dictionary.Save(dictionary_filename);
    }

    
    // PFF files
    directory_lister.SetNameFilter(_T("*.pff"));

    for( const std::wstring& pff_filename : directory_lister.GetPaths(examples_directory) )
    {
        FileUpdateHelper fuh(this, pff_filename);
    
        CNPifFile pff;
        pff.SetPifFileName(WS2CS(pff_filename));

        if( !pff.LoadPifFile(true) )
            fuh.LoadError();

        if( !pff.Save(true) )
            fuh.SaveError();
    }


    // save array files
    directory_lister.SetNameFilter(_T("*.sva"));

    for( const std::wstring& save_array_filename : directory_lister.GetPaths(examples_directory) )
    {
        FileUpdateHelper fuh(this, save_array_filename);

        std::vector<LogicArray*> logic_arrays;
        auto system_message_issuer = std::make_shared<DummySystemMessageIssuer>();

        SaveArrayFile().ReadArrays(save_array_filename, logic_arrays, system_message_issuer, true);

        SaveArrayFile().WriteArrays(save_array_filename, logic_arrays, system_message_issuer, 0, false);
    }


    // deployment specifications
    directory_lister.SetNameFilter(_T("*.csds"));

    for( const std::wstring& deployment_filename : directory_lister.GetPaths(examples_directory) )
    {
        FileUpdateHelper fuh(this, deployment_filename);

        const JsonNode<char> json_node = Json::ParseFile<char>(deployment_filename);
        ASSERT(json_node.IsObject());

        std::unique_ptr<JsonFileWriter> json_writer = Json::CreateFileWriter(deployment_filename);
        json_writer->BeginObject();

        constexpr std::string_view VersionKey_sv = "version";
        ASSERT(VersionKey_sv == UTF8Convert::WideToUTF8(JK::version));

        for( const std::string& key : json_node.GetKeys() )
        {
            if( key == VersionKey_sv )
            {
                json_writer->Write(key, CSPRO_VERSION_NUMBER);
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
    directory_lister.SetNameFilter(_T("*.ent;*.bch;*.xtb"));

    for( const std::wstring& filename : directory_lister.GetPaths(examples_directory) )
        UpdateApplication(filename);


    // area names files
    // logic files
    directory_lister.SetNameFilter(_T("*.anm;*.apc"));

    for( const std::wstring& filename : directory_lister.GetPaths(examples_directory) )
        UpdateObjectlessFile(filename);


    // show a summary of the files updated
    std::wcout << _T("\n\nSuccess\n-------\n");

    for( const auto& [extension, count] : m_updateCounts )
        std::wcout << FormatTextCS2WS(_T("%6s : %2d\n"), extension.c_str(), static_cast<int>(count)).c_str();
}


void ExampleFileUpdater::MarkAsUpdated(const std::wstring& filename)
{
    std::wcout << "Updated: " << filename.c_str() << _T("\n");

    std::wstring extension = SO::ToLower(PortableFunctions::PathGetFileExtension(filename, true));
    
    auto& extension_lookup = m_updateCounts.find(extension);

    if( extension_lookup == m_updateCounts.cend() )
    {
        m_updateCounts.try_emplace(std::move(extension), 1);
    }

    else
    {
        extension_lookup->second = extension_lookup->second + 1;
    }
}


void ExampleFileUpdater::UpdateApplication(const std::wstring& application_filename)
{
    FileUpdateHelper fuh(this, application_filename);
    
    Application application;
    application.Open(WS2CS(application_filename), false, false);

    if( application.GetEngineAppType() == EngineAppType::Entry || application.GetEngineAppType() == EngineAppType::Batch )
    {
        std::vector<std::shared_ptr<CDEFormFile>> form_files;

        for( const CString& form_filename : application.GetFormFilenames() )
            form_files.emplace_back(UpdateFormFile(CS2WS(form_filename)));
        
        if( !application.GetQuestionTextFilename().IsEmpty() )
            UpdateQuestionText(CS2WS(application.GetQuestionTextFilename()), form_files);
    }

    else
    {
        ASSERT(application.GetEngineAppType() == EngineAppType::Tabulation);

        for( const CString& tab_spec_filename : application.GetTabSpecFilenames() )
            UpdateTabSpec(CS2WS(tab_spec_filename));
    }

    application.Save(WS2CS(application_filename));
}


std::shared_ptr<CDEFormFile> ExampleFileUpdater::UpdateFormFile(const std::wstring& form_filename)
{
    std::wstring uppercase_form_filename = SO::ToUpper(form_filename);

    auto& form_file_lookup = m_formFilesProcessed.find(uppercase_form_filename);

    if( form_file_lookup != m_formFilesProcessed.cend() )
        return form_file_lookup->second;

    FileUpdateHelper fuh(this, form_filename);

    auto form_file = std::make_shared<CDEFormFile>();
            
    if( !form_file->Open(WS2CS(form_filename), true) )
        fuh.LoadError();

    auto dictionary = std::make_unique<CDataDict>();
    dictionary->Open(form_file->GetDictionaryFilename());

    form_file->SetDictionary(std::move(dictionary));
    form_file->UpdatePointers();

    form_file->RefreshAssociatedFieldText();

    if( !form_file->Save(WS2CS(form_filename)) )
        fuh.SaveError();

    m_formFilesProcessed.try_emplace(std::move(uppercase_form_filename), form_file);

    return form_file;
}


void ExampleFileUpdater::UpdateQuestionText(const std::wstring& qsf_filename, const std::vector<std::shared_ptr<CDEFormFile>>& form_files)
{
    FileUpdateHelper fuh(this, qsf_filename);
    
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
    capi_question_manager.Load(qsf_filename);
    capi_question_manager.Save(qsf_filename);
}


void ExampleFileUpdater::UpdateTabSpec(const std::wstring& tab_spec_filename)
{
    FileUpdateHelper fuh(this, tab_spec_filename);

    CSpecFile spec_file;
    
    if( !spec_file.Open(WS2CS(tab_spec_filename), CFile::modeRead) )
        fuh.LoadError();

    const std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(spec_file, CSPRO_DICTS);
    ASSERT(dictionary_filenames.size() == 1);

    spec_file.Close();

    CTabSet table_set;
            
    if( !table_set.Open(WS2CS(tab_spec_filename), true) )
        fuh.LoadError();

    CTblPrintFmt* pTblPrintFmt = dynamic_cast<CTblPrintFmt*>(table_set.GetFmtRegPtr()->GetFmt(FMT_ID_TBLPRINT));

    if( pTblPrintFmt != nullptr )
    {
        pTblPrintFmt->SetPrinterDevice(CString());
        pTblPrintFmt->SetPrinterDriver(CString());
        pTblPrintFmt->SetPrinterOutput(CString());
    }

    if( !table_set.Save(WS2CS(tab_spec_filename), WS2CS(dictionary_filenames.front())) )
        fuh.SaveError();
}


void ExampleFileUpdater::UpdateObjectlessFile(const std::wstring& filename)
{
    constexpr wstring_view VersionText_sv = _T("Version=CSPro ");

    FileUpdateHelper fuh(this, filename);

    std::wstring contents = FileIO::ReadText(filename);

    for( size_t version_pos = std::wstring::npos; ( version_pos = contents.find(VersionText_sv, version_pos + 1) ) != std::wstring::npos; )
    {
        const size_t version_insertion_pos = version_pos + VersionText_sv.length();
        size_t version_end_pos = version_insertion_pos;

        for( ; version_end_pos < contents.length(); ++version_end_pos )
        {
            const TCHAR ch = contents[version_end_pos];

            if( !_istdigit(ch) && ch != '.' )
                break;
        }

        ASSERT(version_end_pos > version_insertion_pos);
        contents.erase(contents.begin() + version_insertion_pos, contents.begin() + version_end_pos);
        contents.insert(version_insertion_pos, CSPRO_VERSION_NUMBER_TEXT);

        version_pos = version_insertion_pos;
    }

    FileIO::WriteText(filename, contents, true);
}
