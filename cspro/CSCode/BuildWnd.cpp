#include "StdAfx.h"
#include "BuildWnd.h"


CSCodeBuildWnd::CSCodeBuildWnd()
    :   m_currentCodeView(nullptr)
{
}


void CSCodeBuildWnd::Initialize(CodeView& code_view, std::wstring action)
{
    m_currentCodeView = &code_view;

    std::visit(
        [&](const auto& source_title)
        {
            BuildWnd::Initialize(m_currentCodeView->GetLogicCtrl(), source_title, std::move(action));

        }, code_view.GetDocumentOrTitleForBuildWnd());
}


CLogicCtrl* CSCodeBuildWnd::ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::wstring*> source_logic_ctrl_or_filename)
{
    ASSERT(std::holds_alternative<const CLogicCtrl*>(source_logic_ctrl_or_filename));

    if( assert_cast<CMainFrame*>(AfxGetMainWnd())->ActivateDocument(m_currentCodeView) )
    {
        ASSERT(std::get<const CLogicCtrl*>(source_logic_ctrl_or_filename) == m_currentCodeView->GetLogicCtrl());
        return m_currentCodeView->GetLogicCtrl();
    }

    return nullptr;
}
