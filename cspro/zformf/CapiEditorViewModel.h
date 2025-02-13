#pragma once

#include <zCapiO/CapiText.h>
#include <zCapiO/CapiQuestion.h>
#include <zCapiO/CapiLogicParameters.h>

class Application;
class CapiQuestionManager;
class CDEItemBase;
class DesignerCapiLogicCompiler;


/// <summary>
/// Stores currently selected CAPI question, language and
/// condition to be displayed in CAPI question editor views
/// </summary>
class CapiEditorViewModel
{
public:
    CapiEditorViewModel();
    ~CapiEditorViewModel();

    void SetQuestionManager(Application* application, std::shared_ptr<CapiQuestionManager> question_manager)
    {
        m_application = application;
        m_question_manager = std::move(question_manager);
    }

    void Clear();

    bool CanHaveText() const { return ( m_item != nullptr ); }

    CapiText GetText(size_t language_index, CapiTextType type);
    void SetText(size_t language_index, CapiTextType type, CString new_text);

    void SetCondition(int condition_index, CString logic);
    void DeleteCondition(int condition_index);

    CapiQuestion GetQuestion();

    int GetSelectedConditionIndex() const         { return m_condition_index; }
    void SetSelectedConditionIndex(int condition) { m_condition_index = condition; }

    void SetItem(CDEItemBase* item);

    struct SyntaxCheckOk    { };
    struct SyntaxCheckError { std::wstring error_message; };
    using SyntaxCheckResult = std::variant<SyntaxCheckOk, SyntaxCheckError>;

    SyntaxCheckResult CheckSyntax(CapiLogicParameters::Type type, std::wstring logic);

private:
    std::shared_ptr<CapiQuestionManager> m_question_manager;
    CDEItemBase* m_item;
    CString m_item_name;
    size_t m_condition_index;
    Application* m_application;
    std::unique_ptr<DesignerCapiLogicCompiler> m_compiler;
};
