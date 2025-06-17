#include "StdAfx.h"
#include "CapiEditorViewModel.h"
#include <zAppO/Application.h>
#include <zCapiO/CapiName.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zDesignerF/UWM.h>
#include <zSrcMgrO/DesignerCapiLogicCompiler.h>


CapiEditorViewModel::CapiEditorViewModel()
    :   m_item(nullptr),
        m_conditionIndex(0),
        m_application(nullptr)
{
}


CapiEditorViewModel::~CapiEditorViewModel()
{
}


void CapiEditorViewModel::SetQuestionManager(Application* const application, std::shared_ptr<CapiQuestionManager> question_manager)
{
    ASSERT(application != nullptr && question_manager != nullptr);

    m_application = application;
    m_questionManager = std::move(question_manager);
}


void CapiEditorViewModel::Clear()
{
    m_conditionIndex = 0;
    m_item = nullptr;
}


CapiText CapiEditorViewModel::GetText(const size_t language_index, const CapiText::Type type)
{
    ASSERT(!m_itemName.empty());
    const std::string& language_name = m_questionManager->GetLanguages()[language_index].GetName();

    const CapiQuestion question = GetQuestion();
    const CapiCondition condition = ( m_conditionIndex < question.GetConditions().size() ) ? question.GetConditions()[m_conditionIndex] :
                                                                                             CapiCondition();
    const CapiText* const text = condition.GetText(language_name, type);

    return ( text != nullptr ) ? *text :
                                 CapiText();
}


void CapiEditorViewModel::SetText(const size_t language_index, const CapiText::Type type, CapiText capi_text)
{
    CapiQuestion question = GetQuestion();
    CapiCondition condition = ( m_conditionIndex < question.GetConditions().size() ) ? question.GetConditions()[m_conditionIndex] :
                                                                                       CapiCondition();

    const std::string& language_name = m_questionManager->GetLanguages()[language_index].GetName();

    condition.SetText(std::move(capi_text), language_name, type);
    question.SetCondition(std::move(condition));

    m_questionManager->SetQuestion(std::move(question));
}


void CapiEditorViewModel::SetCondition(const int condition_index, std::string logic)
{
    CapiQuestion question = GetQuestion();
    std::vector<CapiCondition>& conditions = question.GetConditions();

    if( static_cast<size_t>(condition_index) >= conditions.size() )
    {
        conditions.emplace_back(CapiCondition(std::move(logic)));
    }

    else
    {
        conditions[condition_index].SetLogic(logic);
    }

    m_questionManager->SetQuestion(std::move(question));
}


void CapiEditorViewModel::DeleteCondition(const int condition_index)
{
    CapiQuestion question = GetQuestion();
    std::vector<CapiCondition>& conditions = question.GetConditions();
    if (conditions.size() == 1) {
        // don't delete text for last condition, just logic
        conditions.front().SetLogic(std::string());
    }
    else {
        conditions.erase(conditions.begin() + condition_index);
    }
    m_questionManager->SetQuestion(std::move(question));
    m_conditionIndex = 0;
}


CapiQuestion CapiEditorViewModel::GetQuestion()
{
    ASSERT(!m_itemName.empty());
    const CapiQuestion* const question = m_questionManager->GetQuestion(m_itemName);

    return ( question != nullptr ) ? *question :
                                     CapiQuestion(m_itemName);
}


void CapiEditorViewModel::SetItem(const CDEItemBase* const item_base)
{
    ASSERT(item_base != nullptr);

    m_itemName = CapiName::Create(item_base);

    m_item = !m_itemName.empty() ? item_base :
                                   nullptr;

    m_compiler.reset();
}


std::optional<CapiEditorViewModel::SyntaxCheckError> CapiEditorViewModel::CheckSyntax(const SyntaxCheckInput condition_or_text_or_token)
{
    ASSERT(std::visit([](const auto& ptr) { return ( ptr != nullptr ); }, condition_or_text_or_token));

    if( ( m_compiler == nullptr || m_item->GetSymbol() < 1 ) &&
        ( WindowsDesktopMessage::Send(UWM::Designer::CreateCapiLogicCompiler, &m_compiler, m_application) != 1 ) )
    {
        SyntaxCheckError errors;
        errors.emplace_back(Logic::ParserMessage::Type::Error);
        errors.back().message_text = "Could not create DesignerCapiLogicCompiler";
        return ReturnProgrammingError(std::move(errors));
    }

    ASSERT(m_compiler != nullptr && m_item->GetSymbol() >= 1);

    const CapiLogicParameters capi_logic_parameters { m_item->GetSymbol(), condition_or_text_or_token };

    DesignerCapiLogicCompiler::CompileResult result = m_compiler->Compile(capi_logic_parameters);

    if( !result.errors.empty() )
        return result.errors;

    return std::nullopt;
}
