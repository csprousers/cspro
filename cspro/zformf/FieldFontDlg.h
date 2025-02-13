#pragma once

#include <zUtilO/PortableFont.h>


class FieldFontDlg : public CDialog
{
// Construction
public:
    FieldFontDlg(PortableFont field_font, CWnd* pParent = NULL);

    enum { IDD = IDD_FIELD_FONT_DLG };

    const PortableFont& GetFieldFont() const { return m_fieldFont; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    afx_msg void OnFieldFont();
    afx_msg void OnFieldReset();

private:
    PortableFont m_fieldFont;
    CString m_fieldFontDescription;

    PortableFont m_systemFieldFont;
    CString m_systemFieldFontDescription;
};
