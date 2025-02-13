#include "StdAfx.h"
#include "CapiEditorViewModel.h"
#include <zAppO/Application.h>
#include <zCapiO/CapiQuestionManager.h>
#include <Zsrcmgro/DesignerCapiLogicCompiler.h>


CapiEditorViewModel::CapiEditorViewModel()
    :   m_item(nullptr),
        m_condition_index(0),
        m_application(nullptr)
{
}

CapiEditorViewModel::~CapiEditorViewModel()
{
}


void CapiEditorViewModel::Clear()
{
    m_condition_index = 0;
    m_item = nullptr;
}


CapiText CapiEditorViewModel::GetText(size_t language_index, CapiTextType type)
{
    ASSERT(!m_item_name.IsEmpty());
    const auto& language_name = m_question_manager->GetLanguages()[language_index].GetName();
    auto question = GetQuestion();
    CapiCondition condition = (m_condition_index < question.GetConditions().size())
        ? question.GetConditions()[m_condition_index]
        : CapiCondition();
    return condition.GetText(language_name, type);
}


void CapiEditorViewModel::SetText(size_t language_index, CapiTextType type, CString new_text)
{
    auto question = GetQuestion();
    CapiCondition condition = (m_condition_index < question.GetConditions().size())
        ? question.GetConditions()[m_condition_index]
        : CapiCondition();
    const auto& language_name = m_question_manager->GetLanguages()[language_index].GetName();
    if (new_text == "<p></p>")
        new_text.Empty();
    condition.SetText(new_text, language_name, type);
    question.SetCondition(condition);
    m_question_manager->SetQuestion(std::move(question));
}


void CapiEditorViewModel::SetCondition(int condition_index, CString logic)
{
    auto question = GetQuestion();
    std::vector<CapiCondition>& conditions = question.GetConditions();
    if ((size_t)condition_index >= conditions.size()) {
        conditions.emplace_back(CapiCondition(logic));
    }
    else {
        conditions[condition_index].SetLogic(logic);
    }

    m_question_manager->SetQuestion(std::move(question));
}


void CapiEditorViewModel::DeleteCondition(int condition_index)
{
    auto question = GetQuestion();
    std::vector<CapiCondition>& conditions = question.GetConditions();
    if (conditions.size() == 1) {
        // don't delete text for last condition, just logic
        conditions.front().SetLogic(CString());
    }
    else {
        conditions.erase(conditions.begin() + condition_index);
    }
    m_question_manager->SetQuestion(std::move(question));
    m_condition_index = 0;      
}


CapiQuestion CapiEditorViewModel::GetQuestion()
{
    ASSERT(!m_item_name.IsEmpty());
    auto question = m_question_manager->GetQuestion(m_item_name);
    if (!question)
        question.emplace(m_item_name);
    return *question;
}


void CapiEditorViewModel::SetItem(CDEItemBase* item)
{
    if (item->IsKindOf(RUNTIME_CLASS(CDEField))) {
        CDEField* pField = DYNAMIC_DOWNCAST(CDEField, item);
        m_item = item;
        m_item_name = pField->GetDictItem()->GetQualifiedName();
    }
    else if (item->IsKindOf(RUNTIME_CLASS(CDEBlock))) {
        CDEBlock* pBlock = DYNAMIC_DOWNCAST(CDEBlock, item);
        m_item = item;
        m_item_name = pBlock->GetName();
    }
    else {
        m_item = nullptr;
        m_item_name.Empty();
    }
    m_compiler.reset();
}


CapiEditorViewModel::SyntaxCheckResult CapiEditorViewModel::CheckSyntax(CapiLogicParameters::Type type, std::wstring logic)
{
    if( m_compiler == nullptr || m_item->GetSymbol() < 1 )
        m_compiler = std::make_unique<DesignerCapiLogicCompiler>(*m_application);

    CapiLogicParameters capi_logic_parameters { type, m_item->GetSymbol(), std::move(logic) };

    // Creating compiler above should have rebuilt the symbol table
    // but if for some reason the symbol isn't set, use the name
    if( std::get<int>(capi_logic_parameters.symbol_index_or_name) < 1 )
    {
        ASSERT(false);
        capi_logic_parameters.symbol_index_or_name = m_item->GetName();
    }

    DesignerCapiLogicCompiler::CompileResult result = m_compiler->Compile(capi_logic_parameters);

    if( result.expression == -1 )
    {
        return SyntaxCheckError { std::move(result.error_message) };
    }

    else
    {
        return SyntaxCheckOk();
    }
}
