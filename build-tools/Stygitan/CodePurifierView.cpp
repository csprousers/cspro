#include "StdAfx.h"
#include "CodePurifierView.h"


IMPLEMENT_DYNCREATE(CodePurifierView, CFormView)


BEGIN_MESSAGE_MAP(CodePurifierView, CFormView)
END_MESSAGE_MAP()


CodePurifierView::CodePurifierView()
    :   CFormView(IDD_CODE_PURIFIER)
{
}


void CodePurifierView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);
}
