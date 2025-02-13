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


void CodeDoc::UpdateTitle(const TCHAR* filename_or_title/* = nullptr*/)
{
    if( filename_or_title != nullptr || !m_baseModifiedTitle.has_value() )
    {
        if( filename_or_title == nullptr )
            filename_or_title = GetPathName();

        m_baseModifiedTitle = SO::Concatenate(_T("*"), PortableFunctions::PathGetFilename(filename_or_title));
    }

    ASSERT(m_baseModifiedTitle.has_value() && !m_baseModifiedTitle->empty() && m_baseModifiedTitle->front() == '*');

    SetTitle(m_baseModifiedTitle->c_str() + ( IsModified() ? 0 : 1 ));
}


void CodeDoc::SetModifiedFlag(BOOL modified/* = TRUE*/)
{
    if( IsModified() == modified )
        return;

    CDocument::SetModifiedFlag(modified);

    UpdateTitle();
}


void CodeDoc::SetModifiedFlag(BOOL modified, CWnd* scintilla_editor_parent)
{
    // only text modifications to the primary code view will mark the document as modified
    if( IsModified() != modified && scintilla_editor_parent == &GetPrimaryCodeView() )
        SetModifiedFlag(modified);
}


const std::wstring& CodeDoc::GetInitialText() const
{
    return ( m_textSource != nullptr ) ? m_textSource->GetText() :
                                         SO::EmptyString;
}


BOOL CodeDoc::OnNewDocument()
{
    static int new_counter = 0;
    CString title = FormatText(_T("new %d"), ++new_counter);

    UpdateTitle(title);

    return TRUE;
}


BOOL CodeDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    try
    {
        m_textSource = std::make_unique<TextSourceEditable>(lpszPathName);
        m_languageSettings = LanguageSettings(lpszPathName);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    UpdateTitle(lpszPathName);

    return TRUE;
}


BOOL CodeDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    try
    {
        // get the text
        POSITION pos = GetFirstViewPosition();
        CodeView* code_view = assert_cast<CodeView*>(GetNextView(pos));
        CLogicCtrl* logic_ctrl = code_view->GetLogicCtrl();
        std::wstring text = logic_ctrl->GetText();

        // if saving to the same file, use the existing text source
        if( GetPathName() == lpszPathName )
        {
            ASSERT(SO::EqualsNoCase(m_textSource->GetFilename(), lpszPathName));

            m_textSource->SetText(std::move(text));
            m_textSource->Save();
        }

        // otherwise create a new one
        else
        {
            m_textSource = std::make_unique<TextSourceEditable>(lpszPathName, std::move(text), true);
            UpdateTitle(lpszPathName);
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
        AfxMessageBox(FormatText(_T("There is a running operation associated with '%s' and the ")
                                 _T("document cannot be closed until it is canceled or completes."), GetTitle().GetString()));
        return;
    }

    __super::OnCloseDocument();
}


BOOL CodeDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace/* = TRUE*/)
{
    // the default implementation of DoSave uses the title as a suggestion for the filename,
    // but because our title can start with the modified marker *, that got treated as a wildcard,
    // so we will query for the filename ourselves (using code from CDocument::DoSave)
    if( bReplace && lpszPathName == nullptr )
    {
		CDocTemplate* pTemplate = GetDocTemplate();
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


std::wstring CodeDoc::GetPathNameOrFakeTempName(const TCHAR* extension) const
{
    std::wstring path = CS2WS(GetPathName());

    if( path.empty() )
    {
        path = CS2WS(GetTitle());
        path = SO::Trim(SO::Remove(path, '*'));
        path = GetUniqueTempFilename(PortableFunctions::PathAppendFileExtension(path, extension));
    }

    return path;
}


std::tuple<bool, int64_t> CodeDoc::GetFileModificationTimeParameters() const
{
    if( m_textSource != nullptr )
    {
        int64_t file_on_disk_modified_time = PortableFunctions::FileModifiedTime(m_textSource->GetFilename());
        bool file_on_disk_is_newer = ( m_textSource->GetModifiedIteration() < PortableFunctions::FileModifiedTime(m_textSource->GetFilename()) );

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
        GetPrimaryCodeView().SetTextAndSetSavePoint(m_textSource->GetText());

        SetModifiedFlag(FALSE);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CodeDoc::OnFileReloadFromDisk()
{
    int response = AfxMessageBox(FormatText(_T("Are you sure that you want to reload '%s' and lose any changes made in CSCode?"),
                                            PortableFunctions::PathGetFilename(GetPathName())), MB_YESNOCANCEL);

    if( response == IDYES )
        ReloadFromDisk();
}


void CodeDoc::OnUpdateFileReloadFromDisk(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_textSource != nullptr && IsModified());
}


void CodeDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
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
    // the JavaScript processors are set up based on the filename, so use a different one for every filename this document has
    auto lookup = m_processorJavaScript.find(GetPathName());

    if( lookup != m_processorJavaScript.cend() )
    {
        return *lookup->second;
    }

    else
    {
        // keep an old processor around if it might be part of a running operation
        if( !IsRunOperationInProgress() )
            m_processorJavaScript.clear();

        return *m_processorJavaScript.try_emplace(GetPathName(), std::make_unique<ProcessorJavaScript>(*this)).first->second;
    }
}


void CodeDoc::RegisterRunOperation(std::shared_ptr<RunOperation> run_operation)
{
    ASSERT(run_operation != nullptr);
    ASSERT(m_runOperation == nullptr || !m_runOperation->IsRunning());

    m_runOperation = run_operation;

    try
    {
        assert_cast<CMainFrame*>(AfxGetMainWnd())->RegisterRunOperationAndRun(std::move(run_operation));
    }

    catch( const CSProException& exception )
    {
        m_runOperation.reset();
        ErrorMessage::Display(exception);
    }
}
