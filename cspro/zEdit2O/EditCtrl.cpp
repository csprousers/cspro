#include "StdAfx.h"
#include "EditCtrl.h"


BEGIN_MESSAGE_MAP(EditCtrl, CLogicCtrl)

    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateNeedSel)

    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateNeedSel)

    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)

    ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, OnUpdateNeedSel)

    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)

    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)

    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)

    ON_COMMAND(ID_EDIT_DELETE_LINE, OnEditDeleteLine)

    ON_COMMAND(ID_EDIT_DUPLICATE_LINE, OnEditDuplicateLine)

    ON_COMMAND(ID_EDIT_COMMENT_LINE, OnEditCommentLine)

END_MESSAGE_MAP()


EditCtrl::EditCtrl()
    :   m_hAccel(nullptr)
{
}


bool EditCtrl::Create(DWORD dwStyle, CWnd* pParentWnd)
{
    if( !CScintillaCtrl::Create(dwStyle, CRect(), pParentWnd, (UINT)-1) )
        return false;

    InitializeControl();

    return true;
}


void EditCtrl::SetAccelerators(UINT nId)
{
    HINSTANCE hInstance = AfxFindResourceHandle(MAKEINTRESOURCE(nId), RT_ACCELERATOR);
    m_hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(nId));
}


BOOL EditCtrl::PreTranslateMessage(MSG* pMsg)
{
    if( m_hAccel != nullptr && TranslateAccelerator(m_hWnd, m_hAccel, pMsg) )
        return TRUE;

    return CLogicCtrl::PreTranslateMessage(pMsg);
}


void EditCtrl::OnEditCopy()
{
    CLogicCtrl::CopySelection();
}


void EditCtrl::OnEditCut()
{
    CLogicCtrl::Cut();
}


void EditCtrl::OnEditPaste()
{
    Paste();
    SetModified();
    OnUpdateStatusPaneCaretPos();
}

void EditCtrl::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!GetReadOnly() && IsClipboardFormatAvailable(CF_TEXT));
}


void EditCtrl::OnEditClear()
{
    Clear();
    SetModified();
    OnUpdateStatusPaneCaretPos();
}


void EditCtrl::OnEditSelectAll()
{
    SelectAll();
}


void EditCtrl::OnEditUndo()
{
    Undo();
    SetModified();
    OnUpdateStatusPaneCaretPos();
}

void EditCtrl::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanUndo());
}


void EditCtrl::OnEditRedo()
{
    Redo();
    SetModified();
    OnUpdateStatusPaneCaretPos();
}

void EditCtrl::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanRedo());
}


void EditCtrl::OnEditDeleteLine()
{
    LineDelete();
    SetModified();
    OnUpdateStatusPaneCaretPos();
}


void EditCtrl::OnEditDuplicateLine()
{
    if( GetSelectionEmpty() )
        LineDuplicate();

    else
        SelectionDuplicate();

    SetModified();
    OnUpdateStatusPaneCaretPos();
}


void EditCtrl::OnEditCommentLine()
{
    if( !CanCommentLine() )
        return;

    CommentCode();
    // the above method calls SetModified
}
