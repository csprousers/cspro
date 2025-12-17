#pragma once

#include <Stygitan/CodePurifierDoc.h>


class CodePurifierView : public CFormView
{
    DECLARE_DYNCREATE(CodePurifierView)

protected:
    CodePurifierView();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
};
