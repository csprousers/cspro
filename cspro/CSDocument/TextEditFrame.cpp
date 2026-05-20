#include "StdAfx.h"
#include "TextEditFrame.h"


BEGIN_MESSAGE_MAP(TextEditFrame, CMDIChildWndEx)
    ON_WM_MDIACTIVATE()
    ON_MESSAGE(UWM::CSDocument::TextEditFrameActivate, OnTextEditFrameActivate)
END_MESSAGE_MAP()


TextEditFrame::TextEditFrame()
    :   m_codeFrameActivatePostMessageCounter(0)
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

    if( text_edit_doc != nullptr &&
        m_fileModificationChecker.ShouldReloadFile(text_edit_doc->GetTextSource()) )
    {
        text_edit_doc->ReloadFromDisk();
    }
}


void TextEditFrame::OnUpdateDocumentMustBeSavedToDisk(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!GetActiveDocument()->GetPathName().IsEmpty());
}
