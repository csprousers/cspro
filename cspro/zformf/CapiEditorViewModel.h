#pragma once

#include <zCapiO/CapiText.h>
#include <zCapiO/CapiQuestion.h>

class Application;
class CapiQuestionManager;
class CDEItemBase;
class DesignerCapiLogicCompiler;


// --------------------------------------------------------------------------
// CapiEditorViewModel
//
// Stores the currently selected CAPI question, language, and condition
// to be displayed in CAPI question editor views.
// --------------------------------------------------------------------------

class CapiEditorViewModel
{
public:
    CapiEditorViewModel();
    ~CapiEditorViewModel();

    void SetQuestionManager(Application* application, std::shared_ptr<CapiQuestionManager> question_manager);

    void Clear();

    bool CanHaveText() const { return ( m_item != nullptr ); }

    CapiText GetText(size_t language_index, CapiText::Type type);
    void SetText(size_t language_index, CapiText::Type type, CapiText capi_text);

    void SetCondition(int condition_index, std::string logic);
    void DeleteCondition(int condition_index);

    CapiQuestion GetQuestion();

    int GetSelectedConditionIndex() const         { return m_conditionIndex; }
    void SetSelectedConditionIndex(int condition) { m_conditionIndex = condition; }

    void SetItem(const CDEItemBase* item_base);

    struct SyntaxCheckOk    { };
    struct SyntaxCheckError { std::string error_message; };
    using SyntaxCheckResult = std::variant<SyntaxCheckOk, SyntaxCheckError>;

    SyntaxCheckResult CheckSyntax(std::variant<const CapiCondition*, const CapiText*> condition_or_text);

private:
    std::shared_ptr<CapiQuestionManager> m_questionManager;
    const CDEItemBase* m_item;
    std::string m_itemName;
    size_t m_conditionIndex;
    Application* m_application;
    std::unique_ptr<DesignerCapiLogicCompiler> m_compiler;
};
