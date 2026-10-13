#pragma once


// hint IDs used by CDocument::UpdateAllViews and CView::OnUpdate are here to ensure uniqueness

namespace Hint
{
    constexpr LPARAM DesignerDocLoaded              =  1;
    constexpr LPARAM DictionaryTreeSelectionChanged =  2;
    constexpr LPARAM LogicSettingsChanged           =  3;
    constexpr LPARAM LanguageChanged                =  4;
    constexpr LPARAM UseQuestionTextChanged         =  5;

    constexpr LPARAM CapiEditorUpdateQuestion       =  6;
    constexpr LPARAM CapiEditorUpdateCondition      =  7;
    constexpr LPARAM CapiEditorUpdateLanguages      =  8;
    constexpr LPARAM CapiEditorUpdateStyles         =  9;
    constexpr LPARAM CapiEditorUpdateQuestionStyles = 10;
}
