#include "StdAfx.h"
#include "QuestionnaireContentDlg.h"
#include "CaseQuestionnaireContentCreator.h"
#include <zToolsO/DirectoryLister.h>
#include <zAppO/Application.h>
#include <zFormO/FormFile.h>
#include <zCapiO/CapiQuestionManager.h>


BEGIN_MESSAGE_MAP(QuestionnaireContentDlg, ResizableDlg)
    ON_COMMAND(IDC_SELECT_CONTENT, OnSelectContent)
    ON_COMMAND(IDC_AUTO_SEARCH, OnAutoSearch)
    ON_COMMAND(IDC_RESET, OnReset)
END_MESSAGE_MAP()


QuestionnaireContentDlg::QuestionnaireContentDlg(const CaseHoldingDoc& case_holding_doc, CaseQuestionnaireContentCreatorSettings settings,
                                                 CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_QUESTIONNAIRE_CONTENT, pParent),
        m_caseHoldingDoc(case_holding_doc),
        m_settings(std::move(settings)),
        m_formFilePath(m_settings.GetFormFilePath()),
        m_questionTextFilePath(m_settings.GetQuestionTextFilePath()),
        m_formFileMap{ { m_formFilePath, m_settings.GetFormFile() } },
        m_capiQuestionManagerMap{ { m_questionTextFilePath, m_settings.GetCapiQuestionManager() } }
{
    SerializeDialogSize("QuestionnaireContentDlg");
}


void QuestionnaireContentDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_FORM_CONTENT, m_formFilePath, true);
    DDX_Text(pDX, IDC_QUESTION_TEXT_CONTENT, m_questionTextFilePath, true);
}


void QuestionnaireContentDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        m_settings.SetFormFile(m_formFilePath, LoadFormFile(m_formFilePath));
        m_settings.SetCapiQuestionManager(m_questionTextFilePath, LoadCapiQuestionManager(m_questionTextFilePath));

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void QuestionnaireContentDlg::OnSelectContent()
{
    OpenFileDlg open_file_dlg(0, nullptr, nullptr,
                              L"Data Entry Application, Form, and Question Text Files (*.ent;*.fmf;*.qsf)|*.ent;*.fmf;*.qsf|All Files (*.*)|*.*||",
                              this);

    if( open_file_dlg.DoModal() != IDOK )
        return;

    try
    {
        const std::string extension = PortableFunctions::PathGetFileExtension(open_file_dlg.GetFilePath());

        if( SO::EqualsNoCase(extension, FileExtensions::EntryApplication) )
        {
            LoadFromApplication(open_file_dlg.GetFilePath());
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::Form) )
        {
            LoadFormFile(open_file_dlg.GetFilePath());
            m_formFilePath = open_file_dlg.GetFilePath();
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::QuestionText) )
        {
            LoadCapiQuestionManager(open_file_dlg.GetFilePath());
            m_questionTextFilePath = open_file_dlg.GetFilePath();
        }

        else
        {
            throw CSProException("Unable to read a file with with the extension: " + extension);
        }

        UpdateData(FALSE);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


std::shared_ptr<const CDEFormFile> QuestionnaireContentDlg::LoadFormFile(const std::string& file_path)
{
    if( SO::IsWhitespace(file_path) )
        return nullptr;

    auto lookup = m_formFileMap.find(file_path);

    if( lookup == m_formFileMap.cend() )
    {
        lookup = m_formFileMap.try_emplace(file_path,
                                           CaseQuestionnaireContentCreator::LoadFormFile(file_path, m_caseHoldingDoc.GetSharedDictionary())).first;
    }

    return lookup->second;
}


std::shared_ptr<const CapiQuestionManager> QuestionnaireContentDlg::LoadCapiQuestionManager(const std::string& file_path)
{
    if( SO::IsWhitespace(file_path) )
        return nullptr;

    auto lookup = m_capiQuestionManagerMap.find(file_path);

    if( lookup == m_capiQuestionManagerMap.cend() )
    {
        lookup = m_capiQuestionManagerMap.try_emplace(file_path,
                                                      CaseQuestionnaireContentCreator::LoadCapiQuestionManager(file_path)).first;
    }

    return lookup->second;
}


void QuestionnaireContentDlg::LoadFromApplication(const std::string& file_path)
{
    Application application;
    application.Open(file_path, true, false);

    // the appropriate form file needs to be found
    std::string matched_form_file_path;

    for( const std::string& form_file_path : application.GetFormFilePaths() )
    {
        try
        {
            LoadFormFile(form_file_path);
            matched_form_file_path = form_file_path;
            break;
        }

        catch(...)
        {
            // when the application has only one form file, throw the exception directly
            if( application.GetFormFilePaths().size() == 1 )
                throw;
        }
    }

    if( matched_form_file_path.empty() )
    {
        throw CSProException("No form file exists in '%s' that is linked to dictionary '%s'.",
                             file_path.c_str(),
                             m_caseHoldingDoc.GetDictionary().GetName().c_str());
    }

    // all question text can be associated with the dictionary
    std::string question_text_file_path = application.GetQuestionTextFilePath();

    if( !question_text_file_path.empty() )
        LoadCapiQuestionManager(question_text_file_path);

    m_formFilePath = std::move(matched_form_file_path);
    m_questionTextFilePath = std::move(question_text_file_path);
}


void QuestionnaireContentDlg::OnAutoSearch()
{
    DirectoryLister application_directory_lister;
    application_directory_lister.SetNameFilter(FileExtensions::CreateWildcard(FileExtensions::EntryApplication));

    auto process = [&](const std::string& directory)
    {
        for( const std::string& application_file_path : application_directory_lister.GetPaths(directory) )
        {
            try
            {
                LoadFromApplication(application_file_path);
                UpdateData(FALSE);
                return true;
            }
            catch(...) { }
        }

        return false;
    };

    // first process the applications in the same directory as the dictionary
    const std::string dictionary_directory = m_caseHoldingDoc.GetDictionaryDirectory();

    if( dictionary_directory.empty() )
    {
        ErrorMessage::Display(L"This feature is not available because the location of the dictionary is not known.");
        return;
    }

    if( process(dictionary_directory) )
        return;

    // if not found, search for applications in subdirectories, starting from the parent directory
    const std::string parent_directory = PortableFunctions::PathGetDirectory(PortableFunctions::PathRemoveTrailingSlash(dictionary_directory));

    if( !parent_directory.empty() )
    {
        DirectoryLister directory_lister(true, false, true);

        for( const std::string& directory : directory_lister.GetPaths(parent_directory) )
        {
            if( directory != dictionary_directory )
            {
                if( process(directory) )
                    return;
            }
        }
    }

    ErrorMessage::Display("No content could be found in the files located here: " +
                          PortableFunctions::PathRemoveTrailingSlash(!parent_directory.empty() ? dictionary_directory : parent_directory));
}


void QuestionnaireContentDlg::OnReset()
{
    m_formFilePath.clear();
    m_questionTextFilePath.clear();

    UpdateData(FALSE);
}
