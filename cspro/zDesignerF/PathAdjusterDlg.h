#pragma once

#include <zUtilF/EditControls.h>
#include <zEdit2O/LogicCtrl.h>


class PathAdjusterDlg : public CDialog
{
public:
    enum { IDD = IDD_PATH_ADJUSTER };

    PathAdjusterDlg(int lexer_language, std::wstring initial_path, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnPathChange();
    void OnRelativeToChange();
    void OnUseForwardSlahes();
    void OnPathSelect(UINT nID);
    void OnCopy(UINT nID);

private:
    std::wstring AdjustPathSlashes(std::wstring path) const;
    void UpdatePaths();

private:
    Logic::StringEscaper m_logicStringEscaper;
    int m_csproLexerLanguage;

    std::wstring m_path;
    std::wstring m_relativeToFilename;
    bool m_useForwardSlashes;

    CEditWithSelectAll m_pathEdit;
    CLogicCtrl m_pathLogicCtrl;
    CEditWithSelectAll m_relativeToEdit;
    HWND m_relativePathHWnd;
    CLogicCtrl m_relativePathLogicCtrl;
    HWND m_useForwardSlashesHWnd;
    bool m_initialized;
};
