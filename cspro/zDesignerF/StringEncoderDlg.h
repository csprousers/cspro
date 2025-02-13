#pragma once

#include <zEdit2O/LogicCtrl.h>


class StringEncoderDlg : public CDialog
{
public:
    class EncoderWorker;

    enum { IDD = IDD_STRING_ENCODER };

    StringEncoderDlg(const LogicSettings& logic_settings, std::wstring initial_text, CWnd* pParent = nullptr);
    ~StringEncoderDlg();

    bool SplitNewlines() const             { return m_splitNewlines.value_or(false); }
    bool UseVerbatimStringLiterals() const { return m_useVerbatimStringLiterals.value_or(false); }
    bool EscapeJsonForwardSlashes() const  { return m_escapeJsonForwardSlashes; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

    void OnTextChange(UINT nID);
    void OnCopy(UINT nID);
    void OnSplitNewlines();
    void OnUseVerbatimStringLiterals();
    void OnEscapeForwardSlashes();

private:
    void UpdateText();

private:
    int m_lexerLanguage;
    std::wstring m_initialText;

    std::optional<bool> m_splitNewlines;
    std::optional<bool> m_useVerbatimStringLiterals;
    bool m_escapeJsonForwardSlashes;

    CLogicCtrl m_textLogicCtrl;
    CLogicCtrl m_logicLogicCtrl;
    CLogicCtrl m_jsonLogicCtrl;
    CLogicCtrl m_percentEncodingLogicCtrl;
    HWND m_splitNewlinesHWnd;
    HWND m_useVerbatimStringLiteralsHWnd;
    HWND m_escapeJsonForwardSlashesHWnd;

    std::vector<std::unique_ptr<EncoderWorker>> m_encoderWorkers;
    EncoderWorker* m_lastUpdatedEncoderWorker;

    HWND m_currentErrorHWnd;

    bool m_updatingText;
};
