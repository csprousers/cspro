#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zEdit2O/ScintillaDocView.h>


// CSProScintillaFindReplaceDlg is a CScintillaFindReplaceDlg subclass that supports saving/reading
// previously-searched strings to the registry, as well as allowing the user to only replace text in a selection

class CLASS_DECL_ZEDIT2O CSProScintillaFindReplaceDlg : public Scintilla::CScintillaFindReplaceDlg
{
public:
    bool GetReplaceInSelection() const    { return m_replaceInSelection; };
    void SetReplaceInSelection(bool flag) { m_replaceInSelection = flag; };

    void SetAllowReplaceInSelection(bool flag) { m_allowReplaceInSelection = flag; };

    void AddFindStringToCombo(const TCHAR* text)    { AddStringToCombo(IDC_FIND_COMBO, text); }
    void AddReplaceStringToCombo(const TCHAR* text) { AddStringToCombo(IDC_REPLACE_COMBO, text); }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;

    void OnCancel() override;

    void OnClose();

    void OnReplaceInSelection();

    void OnComboSelChange(UINT nID);
    void OnComboEditChange(UINT nID);

private:
    std::tuple<CComboBox*, CWnd*> GetComboBoxAndRelatedEditControl(UINT nID);

    static constexpr int MaxComboStringsToSave = 25;
    void RestoreComboStringsAndHideEditControl(UINT nID);
    void SaveComboStrings(UINT nID);

    void AddStringToCombo(UINT nID, const TCHAR* text);

private:
    bool m_replaceInSelection = false;
    bool m_allowReplaceInSelection = false;

    static std::optional<CPoint> m_lastWindowPosition;
};
