#include "StdAfx.h"
#include "TSourceEditView.h"


IMPLEMENT_DYNCREATE(CTSourceEditView, ApplicationLogicView)


std::unique_ptr<CMenu> CTSourceEditView::GetContextMenu()
{
    auto menu = std::make_unique<CMenu>();

    if( !menu->LoadMenu(IDR_TABLE_LOGIC_EDITOR) )
        menu.reset();

    return menu;
}
