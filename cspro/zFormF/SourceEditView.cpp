#include "StdAfx.h"
#include "SourceEditView.h"
#include <zUtilO/BCMenu.h>


IMPLEMENT_DYNCREATE(CFSourceEditView, ApplicationLogicView)


std::unique_ptr<CMenu> CFSourceEditView::GetContextMenu()
{
    auto menu = std::make_unique<BCMenu>();

    if( !menu->LoadMenu(IDR_FORM_LOGIC_EDITOR) )
        return nullptr;

    menu->LoadToolbar(IDR_FORM_FRAME);

    return menu;
}
