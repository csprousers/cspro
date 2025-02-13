#pragma once

#include <zDesignerF/BuildWnd.h>

class CodeView;


class CSCodeBuildWnd : public BuildWnd
{
public:
    CSCodeBuildWnd();

    void Initialize(CodeView& code_view, std::wstring action);

protected:
    CLogicCtrl* ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::wstring*> source_logic_ctrl_or_filename) override;

private:
    CodeView* m_currentCodeView;
};
