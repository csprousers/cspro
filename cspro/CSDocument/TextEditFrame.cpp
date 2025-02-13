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


void TextEditFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    // code based on CodeFrame::OnMDIActivate
    __super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    if( bActivate )
        PostMessage(UWM::CSDocument::TextEditFrameActivate, ++m_codeFrameActivatePostMessageCounter);

    // update the toolbar (which, when closing the final document, needs to be restored to the main frame toolbar)
    AfxGetMainWnd()->PostMessage(UWM::CSDocument::SyncToolbarAndWindows);
}


LRESULT TextEditFrame::OnTextEditFrameActivate(WPARAM wParam, LPARAM /*lParam*/)
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
    TextEditDoc* text_edit_doc = assert_nullable_cast<TextEditDoc*>(GetActiveDocument());

    if( text_edit_doc == nullptr )
        return;

    const TextSource* text_source = text_edit_doc->GetTextSource();

    if( text_source == nullptr )
        return;

    const int64_t file_on_disk_modified_time = PortableFunctions::FileModifiedTime(text_source->GetFilename());
    const bool file_on_disk_is_newer = ( text_source->GetModifiedIteration() < PortableFunctions::FileModifiedTime(text_source->GetFilename()) );

    if( file_on_disk_is_newer && file_on_disk_modified_time > m_lastCheckIfFileIsUpdatedTime )
    {
        const int response = AfxMessageBox(FormatText(_T("The file has been modified by another program.\nDo you want to reload '%s'?"),
                                                      PortableFunctions::PathGetFilename(text_source->GetFilename())), MB_YESNO);

        if( response == IDYES )
            text_edit_doc->ReloadFromDisk();
    }

    m_lastCheckIfFileIsUpdatedTime = GetTimestamp<int64_t>();
}


void TextEditFrame::OnUpdateDocumentMustBeSavedToDisk(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!GetActiveDocument()->GetPathName().IsEmpty());
}
