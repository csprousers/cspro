#pragma once

#include <zDesignerF/BuildWnd.h>

class CodeView;


class CSCodeBuildWnd : public BuildWnd
{
public:
    CSCodeBuildWnd();

    void Initialize(CodeView& code_view, std::string action);

protected:
    CLogicCtrl* ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::string*> source_logic_ctrl_or_file_path) override;

private:
    CodeView* m_currentCodeView;
};
