#pragma once

#include <zUtilO/PortableFont.h>


class FieldFontDlg : public CDialog
{
public:
    FieldFontDlg(PortableFont field_font, CWnd* pParent = nullptr);

    const PortableFont& GetFieldFont() const { return m_fieldFont; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    afx_msg void OnFieldFont();
    afx_msg void OnFieldReset();

private:
    PortableFont m_fieldFont;
    std::string m_fieldFontDescription;

    PortableFont m_systemFieldFont;
    std::string m_systemFieldFontDescription;
};
