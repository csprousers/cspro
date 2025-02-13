#pragma once

#include <zFormO/FormFile.h>


class FieldColorsDlg : public CDialog
{
public:
    FieldColorsDlg(const CDEFormFile& form_file, CWnd* pParent = nullptr);

    enum { IDD = IDD_FIELD_COLORS_DLG };

    const FieldColors GetFieldColors() const { return m_fieldColors; }

protected:
    DECLARE_MESSAGE_MAP()

    void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    void OnColor(UINT nID);
    void OnReset();

private:
    const CDEFormFile& m_formFile;
    FieldColors m_fieldColors;
};
