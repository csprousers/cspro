#include "StdAfx.h"
#include "CodeDoc.h"
#include <zUtilO/TextSourceEditable.h>


IMPLEMENT_DYNCREATE(CodeDoc, CDocument)

BEGIN_MESSAGE_MAP(CodeDoc, CDocument)

    // File menu
    ON_COMMAND(ID_FILE_RELOAD_FROM_DISK, OnFileReloadFromDisk)
    ON_UPDATE_COMMAND_UI(ID_FILE_RELOAD_FROM_DISK, OnUpdateFileReloadFromDisk)

    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)

END_MESSAGE_MAP()


CodeDoc::CodeDoc()
{
}


CodeDoc::~CodeDoc()
{
}


CodeView& CodeDoc::GetPrimaryCodeView()
{
    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);

    return assert_cast<CodeView&>(*GetNextView(pos));
}


CodeView* CodeDoc::GetSecondaryCodeView()
{
    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);

    // skip past the first view
    GetNextView(pos);

    return assert_nullable_cast<CodeView*>(GetNextView(pos));
}


CodeFrame& CodeDoc::GetCodeFrame()
{
    return assert_cast<CodeFrame&>(*GetPrimaryCodeView().GetParentFrame());
}


void CodeDoc::UpdateTitle(cs::cref_optional<std::string> file_path_or_title/* = std::nullopt*/)
{
    if( file_path_or_title.has_value() || !m_baseModifiedTitle.has_value() )
    {
        if( !file_path_or_title.has_value() )
            file_path_or_title = GetFilePath();

        m_baseModifiedTitle = SO::Concatenate("*", PortableFunctions::PathGetFilename(*file_path_or_title));
    }

    ASSERT(m_baseModifiedTitle.has_value() && !m_baseModifiedTitle->empty() && m_baseModifiedTitle->front() == '*');

    std::string_view title_sv = *m_baseModifiedTitle;

    if( !IsModified() )
        title_sv.remove_prefix(1);

    SetTitle(TC::ToWide(title_sv).c_str());
}


void CodeDoc::SetModifiedFlag(const BOOL modified/* = TRUE*/)
{
    if( IsModified() == modified )
        return;

    CDocument::SetModifiedFlag(modified);

    UpdateTitle();
}


void CodeDoc::SetModifiedFlag(const BOOL modified, CWnd* const scintilla_editor_parent)
{
    // only text modifications to the primary code view will mark the document as modified
    if( IsModified() != modified && scintilla_editor_parent == &GetPrimaryCodeView() )
        SetModifiedFlag(modified);
}


const std::string& CodeDoc::GetInitialText() const
{
    return ( m_textSource != nullptr ) ? m_textSource->GetText() :
                                         SO::Empty_string;
}


BOOL CodeDoc::OnNewDocument()
{
    static int new_counter = 0;
    const std::string title = FormatText("new %d", ++new_counter);

    UpdateTitle(title);

    return TRUE;
}


BOOL CodeDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    try
    {
        m_textSource = std::make_unique<TextSourceEditable>(TC::ToUtf8(lpszPathName));
        m_languageSettings = LanguageSettings(m_textSource->GetFilePath());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    UpdateTitle(m_textSource->GetFilePath());

    return TRUE;
}


BOOL CodeDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    try
    {
        // get the text
        POSITION pos = GetFirstViewPosition();
        CodeView* const code_view = assert_cast<CodeView*>(GetNextView(pos));
        CLogicCtrl* const logic_ctrl = code_view->GetLogicCtrl();
        std::string text = logic_ctrl->GetText();

        // if saving to the same file, use the existing text source
        if( GetPathName() == lpszPathName )
        {
            ASSERT(SO::EqualsNoCase(m_textSource->GetFilePath(), lpszPathName));

            m_textSource->SetText(std::move(text));
            m_textSource->Save();
        }

        // otherwise create a new one
        else
        {
            m_textSource = std::make_unique<TextSourceEditable>(TC::ToUtf8(lpszPathName), std::move(text), true);
            UpdateTitle(m_textSource->GetFilePath());
        }

        SetModifiedFlag(FALSE);

        code_view->GetLogicCtrl()->SetSavePoint();

        return TRUE;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }
}


void CodeDoc::OnCloseDocument()
{
    if( IsRunOperationInProgress() )
    {
        AfxMessageBox(FormatText(L"There is a running operation associated with '%s' and the "
                                 L"document cannot be closed until it is canceled or completes.", GetTitle().GetString()));
        return;
    }

    __super::OnCloseDocument();
}


BOOL CodeDoc::DoSave(LPCTSTR lpszPathName, const BOOL bReplace/* = TRUE*/)
{
    // the default implementation of DoSave uses the title as a suggestion for the filename,
    // but because our title can start with the modified marker *, that got treated as a wildcard,
    // so we will query for the filename ourselves (using code from CDocument::DoSave)
    if( bReplace && lpszPathName == nullptr )
    {
        CDocTemplate* const pTemplate = GetDocTemplate();
        ASSERT(pTemplate != NULL);

        CString newName = GetPathName();

        if( !AfxGetApp()->DoPromptFileName(newName, bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY, OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, pTemplate) )
            return FALSE;

        return __super::DoSave(newName, bReplace);
    }

    else
    {
        return __super::DoSave(lpszPathName, bReplace);
    }
}


const std::string& CodeDoc::GetFilePath() const
{
    if( m_textSource != nullptr )
    {
        ASSERT81(m_textSource->GetFilePath() == TC::ToUtf8(GetPathName()));
        return m_textSource->GetFilePath();
    }

    ASSERT(GetPathName().IsEmpty());
    return SO::Empty_string;
}


std::string CodeDoc::GetActualOrTempFilePath(const char* const extension) const
{
    std::string path = GetFilePath();

    if( !path.empty() )
        return path;

    path = TC::ToUtf8(GetTitle());
    path = SO::MakeTrim(SO::Remove(path, '*'));

    return GetUniqueTempFilePath(PortableFunctions::PathAppendFileExtension(std::move(path), extension));
}


std::string CodeDoc::GetActualOrTempDirectory() const
{
    const std::string path = GetFilePath();

    if( !path.empty() )
        return PortableFunctions::PathGetDirectory(path);

    return GetTempDirectory();
}


std::tuple<bool, int64_t> CodeDoc::GetFileModificationTimeParameters() const
{
    if( m_textSource != nullptr )
    {
        const int64_t file_on_disk_modified_time = PortableFunctions::FileModifiedTime(m_textSource->GetFilePath());
        const bool file_on_disk_is_newer = ( m_textSource->GetModifiedIteration() < PortableFunctions::FileModifiedTime(m_textSource->GetFilePath()) );

        return { file_on_disk_is_newer, file_on_disk_modified_time };
    }

    else
    {
        return { false, 0 };
    }
}


void CodeDoc::ReloadFromDisk()
{
    try
    {
        m_textSource->ReloadFromDisk();
        GetPrimaryCodeView().GetLogicCtrl()->SetTextAndSetSavePoint(m_textSource->GetText());

        SetModifiedFlag(FALSE);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CodeDoc::OnFileReloadFromDisk()
{
    const int response = AfxMessageBox(FormatText("Are you sure that you want to reload '%s' and lose any changes made in CSCode?",
                                                  PortableFunctions::PathGetFilename(GetFilePath()).c_str()), MB_YESNOCANCEL);

    if( response == IDYES )
        ReloadFromDisk();
}


void CodeDoc::OnUpdateFileReloadFromDisk(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(m_textSource != nullptr && IsModified());
}


void CodeDoc::OnUpdateFileSave(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsModified());
}


ProcessorHtml& CodeDoc::GetHtmlProcessor()
{
    if( m_processorHtml == nullptr )
        m_processorHtml = std::make_unique<ProcessorHtml>();

    return *m_processorHtml;
}


ProcessorJavaScript& CodeDoc::GetJavaScriptProcessor()
{
    // the JavaScript processors are set up based on the file path, so use a different one for every file path this document has
    std::string file_path = GetFilePath();
    auto lookup = m_processorJavaScript.find(file_path);

    if( lookup != m_processorJavaScript.cend() )
    {
        return *lookup->second;
    }

    else
    {
        // keep an old processor around if it might be part of a running operation
        if( !IsRunOperationInProgress() )
            m_processorJavaScript.clear();

        return *m_processorJavaScript.try_emplace(std::move(file_path), std::make_unique<ProcessorJavaScript>(*this)).first->second;
    }
}


void CodeDoc::RegisterRunOperation(std::shared_ptr<RunOperation> run_operation)
{
    ASSERT(run_operation != nullptr);
    ASSERT(m_runOperation == nullptr || !m_runOperation->IsRunning());

    m_runOperation = std::move(run_operation);

    try
    {
        assert_cast<CMainFrame*>(AfxGetMainWnd())->RegisterRunOperationAndRun(m_runOperation);
    }

    catch( const CSProException& exception )
    {
        m_runOperation.reset();
        ErrorMessage::Display(exception);
    }
}
