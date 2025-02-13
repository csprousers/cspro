#pragma once

#include <zFormO/DragOptions.h>


class DragOptionsDlg : public CDialog
{
public:
    DragOptionsDlg(DragOptions& drag_options, CWnd* pParent);

    enum { IDD = IDD_DRAG_OPTS };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;

    afx_msg void OnRosterOrientationChange(UINT nID);

private:
    void SyncRosterOrientationBitmap();

private:
    DragOptions& m_dragOptions;

	int m_textLayout;
	int m_textUse;
    BOOL m_linkFieldTextToDictionary;
    int m_rosterUse;
    BOOL m_useOccurrenceLabels;
    int m_useExtendedControls;
    BOOL m_useSubitems;
    BOOL m_useEnterKey;

    CWnd m_horizontalRosterBitmap;
    CWnd m_verticalRosterBitmap;
};
