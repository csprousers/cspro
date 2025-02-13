#pragma once


// hint IDs used by CDocument::UpdateAllViews and CView::OnUpdate are here to ensure uniqueness

namespace Hint
{
    constexpr LPARAM DictionaryTreeSelectionChanged = 1;
    constexpr LPARAM LanguageChanged                = 2;
    constexpr LPARAM UseQuestionTextChanged         = 3;

    constexpr LPARAM CapiEditorUpdateQuestion       = 4;
    constexpr LPARAM CapiEditorUpdateCondition      = 5;
    constexpr LPARAM CapiEditorUpdateLanguages      = 6;
    constexpr LPARAM CapiEditorUpdateStyles         = 7;
    constexpr LPARAM CapiEditorUpdateQuestionStyles = 8;
}
