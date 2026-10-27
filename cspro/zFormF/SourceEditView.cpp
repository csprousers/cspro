#include "StdAfx.h"
#include "SourceEditView.h"
#include <zUtilO/BCMenu.h>


IMPLEMENT_DYNCREATE(CFSourceEditView, ApplicationLogicView)


BEGIN_MESSAGE_MAP(CFSourceEditView, ApplicationLogicView)
    ON_COMMAND(ID_VIEW_FORM, OnViewForm)
    ON_COMMAND(ID_QSF_EDITOR, OnQsfEditor)
    ON_WM_SETFOCUS()
    ON_COMMAND(ID_LOGIC_FORMAT, OnFormatLogic)
    ON_UPDATE_COMMAND_UI(ID_LOGIC_FORMAT, OnUpdateFormatLogic)
END_MESSAGE_MAP()


std::unique_ptr<CMenu> CFSourceEditView::GetContextMenu()
{
    auto menu = std::make_unique<BCMenu>();

    if( !menu->LoadMenu(IDR_FORM_LOGIC_EDITOR) )
        return nullptr;

    menu->LoadToolbar(IDR_FORM_FRAME);

    return menu;
}


void CFSourceEditView::OnViewForm()
{
    ((CFormChildWnd*)GetParentFrame())->OnViewForm();
}


void CFSourceEditView::OnQsfEditor()
{
    ((CFormChildWnd*)GetParentFrame())->OnQsfEditor();
}


void CFSourceEditView::OnSetFocus(CWnd* pOldWnd)
{
    CView::OnSetFocus(pOldWnd);
    if(GetLogicCtrl() && GetLogicCtrl()->GetSafeHwnd()){
        GetLogicCtrl()->SetFocus();
    }
}


void CFSourceEditView::OnFormatLogic()
{
    GetDocument()->SetModifiedFlag(true);
    GetLogicCtrl()->FormatLogic();
}


void CFSourceEditView::OnUpdateFormatLogic(CCmdUI* const pCmdUI)
{
    LogicCtrl* const logic_ctrl = GetLogicCtrl();

    pCmdUI->Enable(( logic_ctrl == GetFocus() &&
                     Lexers::IsCSProLogic(logic_ctrl->GetLexer()) ));
}
