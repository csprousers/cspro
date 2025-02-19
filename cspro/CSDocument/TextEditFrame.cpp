#include "StdAfx.h"
#include "TextEditFrame.h"


BEGIN_MESSAGE_MAP(TextEditFrame, CMDIChildWndEx)
    ON_WM_MDIACTIVATE()
    ON_MESSAGE(UWM::CSDocument::TextEditFrameActivate, OnTextEditFrameActivate)
END_MESSAGE_MAP()


TextEditFrame::TextEditFrame()
    :   m_codeFrameActivatePostMessageCounter(0),
        m_lastCheckIfFileIsUpdatedTime(0)
{
}


void TextEditFrame::OnMDIActivate(BOOL const bActivate, CWnd* const pActivateWnd, CWnd* const pDeactivateWnd)
{
    // code based on CodeFrame::OnMDIActivate
    __super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    if( bActivate )
        PostMessage(UWM::CSDocument::TextEditFrameActivate, ++m_codeFrameActivatePostMessageCounter);

    // update the toolbar (which, when closing the final document, needs to be restored to the main frame toolbar)
    AfxGetMainWnd()->PostMessage(UWM::CSDocument::SyncToolbarAndWindows);
}


LRESULT TextEditFrame::OnTextEditFrameActivate(const WPARAM wParam, LPARAM /*lParam*/)
{
    // code based on CodeFrame::OnCodeFrameActivate
    const WPARAM& post_message_counter = wParam;

    if( post_message_counter == m_codeFrameActivatePostMessageCounter || post_message_counter == static_cast<WPARAM>(-1) )
    {
        m_codeFrameActivatePostMessageCounter = 0;

        // check if the file has been modified
        CheckIfFileIsUpdated();
    }

    return 1;
}


void TextEditFrame::CheckIfFileIsUpdated()
{
    TextEditDoc* const text_edit_doc = assert_nullable_cast<TextEditDoc*>(GetActiveDocument());

    if( text_edit_doc == nullptr )
        return;

    const TextSource* const text_source = text_edit_doc->GetTextSource();

    if( text_source == nullptr )
        return;

    const int64_t file_on_disk_modified_time = PortableFunctions::FileModifiedTime(text_source->GetFilePath());
    const bool file_on_disk_is_newer = ( text_source->GetModifiedIteration() < file_on_disk_modified_time );

    if( file_on_disk_is_newer && file_on_disk_modified_time > m_lastCheckIfFileIsUpdatedTime )
    {
        const std::string message = FormatText("The file has been modified by another program.\nDo you want to reload '%s'?",
                                               PortableFunctions::PathGetFilename(text_source->GetFilePath()).c_str());

        if( AfxMessageBox(message, MB_YESNO) == IDYES )
            text_edit_doc->ReloadFromDisk();
    }

    m_lastCheckIfFileIsUpdatedTime = GetTimestamp<int64_t>();
}


void TextEditFrame::OnUpdateDocumentMustBeSavedToDisk(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!GetActiveDocument()->GetPathName().IsEmpty());
}
