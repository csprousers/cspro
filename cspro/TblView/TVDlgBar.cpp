#include "StdAfx.h"
#include "TVDlgBar.h"
#include <zUtilO/DynamicLayoutControlResizer.h>


CTVDlgBar::CTVDlgBar()
    :   COXSizeDialogBar(SZBARF_STDMOUSECLICKS)
{
}


CTVDlgBar::~CTVDlgBar() = default;


BOOL CTVDlgBar::Create(CWnd* const pParentWnd, const UINT nStyle)
{
    ASSERT(( m_Style & SZBARF_DLGAUTOSIZE ) == 0 );

    if( !__super::Create(pParentWnd, IDD_DIALOGBAR, nStyle, 0) )
        return FALSE;

    // by default, DynamicLayoutControlResizer uses the client window for its
    // sizing calculation, but doing this here in Create (as opposed to OnSizedOrDocked),
    // using the window rectangle seems to work better for these calculations
    CRect rect;
    GetWindowRect(rect);
    const CSize size = rect.Size();
    m_dynamicLayoutControlResizer = std::make_unique<DynamicLayoutControlResizer>(*this, &size);

    m_dynamicLayoutControlResizer->Add(IDC_TABLETREE, SizingDirection::XY);

    return TRUE;
}


void CTVDlgBar::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_TABLETREE, m_TblTree);
}


BOOL CTVDlgBar::OnInitDialog()
{
    UpdateData(FALSE);

    m_TblTree.InitImageList();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CTVDlgBar::OnSizedOrDocked(int cx, int cy, BOOL bFloating, int flags)
{
    ASSERT(( m_Style & SZBARF_DLGAUTOSIZE ) == 0 );

    __super::OnSizedOrDocked(cx, cy, bFloating, flags);

    if( m_dynamicLayoutControlResizer != nullptr )
        m_dynamicLayoutControlResizer->OnSize(cx, cy);
}
