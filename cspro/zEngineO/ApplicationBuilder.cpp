#include "stdafx.h"
#include "ApplicationBuilder.h"
#include "ApplicationLoader.h"
#include <zUtilF/ProgressDlg.h>
#include <ZBRIDGEO/npff.h>


// --------------------------------------------------
// ApplicationBuilder
// --------------------------------------------------

class ApplicationBuilder
{
public:
    ApplicationBuilder(std::shared_ptr<ApplicationLoader> application_loader);

    void Build(const std::optional<EngineAppType>& required_application_type);

private:
    void CheckUniqueName(const CString& filename, const CString& name);

    std::shared_ptr<CDataDict> LoadDictionary(std::wstring filename, std::wstring parent_filename, DictionaryType dictionary_type);

    void LoadExternalDictionaries();

    void LoadFormFiles();

private:
    std::shared_ptr<ApplicationLoader> m_applicationLoader;
    std::map<CString, CString> m_usedNames;

    // APP_LOAD_TODO eventually all of these objects should be shared pointers
    Application* m_application; // (owned by the ApplicationLoader)
};


ApplicationBuilder::ApplicationBuilder(std::shared_ptr<ApplicationLoader> application_loader)
    :   m_applicationLoader(std::move(application_loader)),
        m_application(nullptr)
{
    ASSERT(m_applicationLoader != nullptr);
}


void ApplicationBuilder::Build(const std::optional<EngineAppType>& required_application_type)
{
    // load the application
    m_application = m_applicationLoader->GetApplication();
    m_application->SetApplicationLoader(m_applicationLoader);

    // check that the application type is valid
    if( m_application->GetEngineAppType() == EngineAppType::Invalid )
        throw ApplicationLoadException("The application was of an invalid type.");

    if( required_application_type.has_value() && required_application_type != m_application->GetEngineAppType() )
        throw ApplicationLoadException(_T("This program can only load %s applications."), ToString(*required_application_type));

    // for now this is only used by entry and batch applications
    if( m_application->GetEngineAppType() != EngineAppType::Batch && m_application->GetEngineAppType() != EngineAppType::Entry )
        throw ProgrammingErrorException();

    LoadExternalDictionaries();

    LoadFormFiles();

    CNPifFile::SetFormFileNumber(*m_application);
}


void ApplicationBuilder::CheckUniqueName(const CString& filename, const CString& name)
{
    ASSERT(SO::IsUpper(name));
    const auto& used_name_lookup = m_usedNames.find(name);

    if( used_name_lookup != m_usedNames.cend() )
    {
        throw ApplicationLoadException(_T("An application cannot contain multiple objects with the same name. %s is used in both %s and %s."),
                                       name.GetString(), PortableFunctions::PathGetFilename(filename), PortableFunctions::PathGetFilename(used_name_lookup->second));
    }

    m_usedNames.try_emplace(name, filename);
}


std::shared_ptr<CDataDict> ApplicationBuilder::LoadDictionary(std::wstring filename, std::wstring parent_filename, DictionaryType dictionary_type)
{
    std::shared_ptr<CDataDict> dictionary = m_applicationLoader->GetDictionary(filename);

    CheckUniqueName(WS2CS(filename), dictionary->GetName());

    DictionaryDescription* dictionary_description = m_application->GetDictionaryDescription(filename);

    // if there is no dictionary description, create one
    if( dictionary_description == nullptr )
        dictionary_description = m_application->AddDictionaryDescription(DictionaryDescription(std::move(filename), std::move(parent_filename), dictionary_type));

    dictionary_description->SetDictionary(dictionary.get());

    return dictionary;
}


void ApplicationBuilder::LoadExternalDictionaries()
{
    for( const CString& dictionary_filename : m_application->GetExternalDictionaryFilenames() )
    {
        std::shared_ptr<CDataDict> dictionary = LoadDictionary(CS2WS(dictionary_filename), std::wstring(), DictionaryType::External);
        m_application->AddRuntimeExternalDictionary(std::move(dictionary));
    }
}


void ApplicationBuilder::LoadFormFiles()
{
    if( m_application->GetFormFilenames().empty() )
        throw ApplicationLoadException("An application must have at least one form.");

    DictionaryType dictionary_type = DictionaryType::Input;

    for( const CString& form_filename : m_application->GetFormFilenames() )
    {
        std::shared_ptr<CDEFormFile> form_file = m_applicationLoader->GetFormFile(form_filename);

        CheckUniqueName(form_filename, form_file->GetName());

        // load the form file's dictionary
        std::shared_ptr<CDataDict> dictionary = LoadDictionary(CS2WS(form_file->GetDictionaryFilename()), CS2WS(form_filename), dictionary_type);

        form_file->SetDictionary(std::move(dictionary));
        form_file->UpdatePointers();

        m_application->AddRuntimeFormFile(std::move(form_file));

        // dictionaries on non-primary form files will be loaded as external dictionaries
        dictionary_type = DictionaryType::External;
    }
}



// --------------------------------------------------
// BuildApplication
// --------------------------------------------------

void BuildApplication(std::shared_ptr<ApplicationLoader> application_loader,
                      std::optional<EngineAppType> required_application_type/* = std::nullopt*/)
{
    ProgressDlgSharing share_progress_dialog;

    ApplicationBuilder(std::move(application_loader)).Build(required_application_type);
}
