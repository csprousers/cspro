#pragma once

#include <zEngineO/AllSymbolDeclarations.h>

class CCapiControl;
class ResponseProcessor;


#define EXTENDED_CONTROL_BORDER_SIZE    10
#define EXTENDED_CONTROL_COL_SPACING    2 // in characters in current font
#define EXTENDED_CONTROL_ROW_SPACING    7 // in px
#define EXTENDED_CONTROL_RESOURCE_ID    60001


// CExtendedControl dialog

class AFX_EXT_CLASS CExtendedControl : public CDialog
{
    DECLARE_DYNAMIC(CExtendedControl)

    friend class CCapiControl;

private:
    const VART* m_pVarT;
    CaptureInfo m_captureInfo;
    CaptureType m_captureType;
    const CDictItem* m_pDictItem;
    const ResponseProcessor* m_responseProcessor;
    CWnd* m_pParent;
    CWnd* m_pEdit;
    CCapiControl* m_pCapiControl;
    CRect m_OrigControlsRect;
    CSize m_minSearchBarSize;
    CFont font;
    bool m_bCommaDecimal;
    CToolTipCtrl m_toolTips;

public:
    CExtendedControl(CWnd* pParent = NULL);   // standard constructor
    virtual ~CExtendedControl();

    int DoModeless(VART* pVarT, const CaptureInfo& capture_info, const ResponseProcessor* response_processor,
        CWnd* pEdit, bool bCommaDecimal);

    void UpdateSelection(const CString& keyedText);

    CSize GetSearchBarSize();
    void PlaceSearchBarControls();
    void DoSizing(CRect & rect);
    void RefreshPosition();

    CFont* GetControlFont(bool font_for_number_pad = false);

    void OnSysCommand(UINT nID,LPARAM lParam);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreTranslateMessage(MSG* pMsg) override;

    afx_msg void OnClickedButtonNextField();
    afx_msg void OnClickedButtonPreviousField();
    afx_msg void OnClickedButtonSearch();
};
