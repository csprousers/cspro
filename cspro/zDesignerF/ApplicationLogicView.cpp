#include "StdAfx.h"
#include "ApplicationLogicView.h"
#include <zEditO/resource_shared.h>


IMPLEMENT_DYNAMIC(ApplicationLogicView, LogicView)


BEGIN_MESSAGE_MAP(ApplicationLogicView, LogicView)
    ON_COMMAND(ID_FORMAT_CSPRO_LOGIC, OnFormatLogic)
END_MESSAGE_MAP()


void ApplicationLogicView::OnFormatLogic()
{
    __super::OnFormatLogic();
    m_logicCtrl->SetModified(true);
}
