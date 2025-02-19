#include "StdAfx.h"
#include "BuildWnd.h"


CSCodeBuildWnd::CSCodeBuildWnd()
    :   m_currentCodeView(nullptr)
{
}


void CSCodeBuildWnd::Initialize(CodeView& code_view, std::string action)
{
    m_currentCodeView = &code_view;

    std::visit(
        [&](const auto& source_title)
        {
            BuildWnd::Initialize(m_currentCodeView->GetLogicCtrl(), source_title, std::move(action));

        }, code_view.GetDocumentOrTitleForBuildWnd());
}


CLogicCtrl* CSCodeBuildWnd::ActivateDocumentAndGetLogicCtrl(const std::variant<const CLogicCtrl*, const std::string*> source_logic_ctrl_or_file_path)
{
    ASSERT(std::holds_alternative<const CLogicCtrl*>(source_logic_ctrl_or_file_path));

    if( assert_cast<CMainFrame*>(AfxGetMainWnd())->ActivateDocument(m_currentCodeView) )
    {
        ASSERT(std::get<const CLogicCtrl*>(source_logic_ctrl_or_file_path) == m_currentCodeView->GetLogicCtrl());
        return m_currentCodeView->GetLogicCtrl();
    }

    return nullptr;
}
