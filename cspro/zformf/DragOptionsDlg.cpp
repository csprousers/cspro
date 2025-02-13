#include "StdAfx.h"
#include "DragOptionsDlg.h"


BEGIN_MESSAGE_MAP(DragOptionsDlg, CDialog)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_ROSTER_HORIZONTAL, IDC_ROSTER_NONE, OnRosterOrientationChange)
END_MESSAGE_MAP()


DragOptionsDlg::DragOptionsDlg(DragOptions& drag_options, CWnd* pParent)
    :   CDialog(DragOptionsDlg::IDD, pParent),
        m_dragOptions(drag_options)
{
    m_textLayout = (int)m_dragOptions.m_textLayout;
    m_textUse = (int)m_dragOptions.m_textUse;
    m_linkFieldTextToDictionary = (BOOL)m_dragOptions.m_linkFieldTextToDictionary;
    m_rosterUse = (int)m_dragOptions.m_rosterUse;
    m_useOccurrenceLabels = (BOOL)m_dragOptions.m_useOccurrenceLabels;
    m_useExtendedControls = m_dragOptions.m_useExtendedControls ? 0 : 1;
    m_useSubitems = (BOOL)m_dragOptions.m_useSubitems;
    m_useEnterKey = (BOOL)m_dragOptions.m_useEnterKey;
}


void DragOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_TEXT_LEFT, m_textLayout);
	DDX_Radio(pDX, IDC_USELABELS, m_textUse);
    DDX_Check(pDX, IDC_LINK_LABELS, m_linkFieldTextToDictionary);
    DDX_Radio(pDX, IDC_ROSTER_HORIZONTAL, m_rosterUse);
    DDX_Check(pDX, IDC_USE_OCCURRENCE_LABELS, m_useOccurrenceLabels);
    DDX_Radio(pDX, IDC_CAPTURE_TYPES_CAPI, m_useExtendedControls);
    DDX_Check(pDX, IDC_USE_SUBITEMS, m_useSubitems);
    DDX_Check(pDX, IDC_REQUIRE_ENTER, m_useEnterKey);

    DDX_Control(pDX, IDC_GRID_HORZ, m_horizontalRosterBitmap);
    DDX_Control(pDX, IDC_GRID_VERT, m_verticalRosterBitmap);
}


BOOL DragOptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SyncRosterOrientationBitmap();
    return TRUE;
}

void DragOptionsDlg::OnRosterOrientationChange(UINT /*nID*/)
{
    UpdateData(TRUE);
    SyncRosterOrientationBitmap();
}

void DragOptionsDlg::SyncRosterOrientationBitmap()
{
    m_horizontalRosterBitmap.ShowWindow(( m_rosterUse == (int)DragOptions::RosterUse::Horizontal));
    m_verticalRosterBitmap.ShowWindow(( m_rosterUse == (int)DragOptions::RosterUse::Vertical));
}


void DragOptionsDlg::OnOK()
{
    UpdateData(TRUE);

    // update the drag options object and then save the settings to the registry
    m_dragOptions.m_textLayout = (CDEFormBase::TextLayout)m_textLayout;
    m_dragOptions.m_textUse = (CDEFormBase::TextUse)m_textUse;
    m_dragOptions.m_linkFieldTextToDictionary = (bool)m_linkFieldTextToDictionary;
    m_dragOptions.m_rosterUse = (DragOptions::RosterUse)m_rosterUse;
    m_dragOptions.m_useOccurrenceLabels = (bool)m_useOccurrenceLabels;
    m_dragOptions.m_useExtendedControls = ( m_useExtendedControls == 0 );
    m_dragOptions.m_useSubitems = (bool)m_useSubitems;
    m_dragOptions.m_useEnterKey = (bool)m_useEnterKey;

    m_dragOptions.SaveToRegistry();

    CDialog::OnOK();
}
