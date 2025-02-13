#include "StdAfx.h"
#include "TableToolbarButton.h"
#include "TableToolbarDropDown.h"

IMPLEMENT_SERIAL(TableToolbarButton, CMFCToolBarButton, 1)

TableToolbarButton::TableToolbarButton()
    : m_dropdown(nullptr)
{
}

TableToolbarButton::TableToolbarButton(UINT uiID, int iImage, LPCTSTR lpszText)
    : CMFCToolBarButton(uiID, iImage, lpszText),
      m_dropdown(nullptr)
{
}

void TableToolbarButton::OnDraw(CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight, BOOL bDrawBorder, BOOL bGrayDisabledButtons)
{
    const int nSeparatorSize = 2;

    //----------------------
    // Fill button interior:
    //----------------------
    FillInterior(pDC, rect, bHighlight || IsDroppedDown());

    CSize sizeImage = CMenuImages::Size();
    if (CMFCToolBar::IsLargeIcons())
    {
        sizeImage.cx *= 2;
        sizeImage.cy *= 2;
    }

    CRect rectInternal = rect;
    CSize sizeExtra = m_bExtraSize ? CMFCVisualManager::GetInstance()->GetButtonExtraBorder() : CSize(0, 0);

    if (sizeExtra != CSize(0, 0))
    {
        rectInternal.DeflateRect(sizeExtra.cx / 2 + 1, sizeExtra.cy / 2 + 1);
    }

    CRect rectParent = rect;
    CRect rectArrow = rectInternal;

    const int nMargin = CMFCVisualManager::GetInstance()->GetMenuImageMargin();
    const int nXMargin = bHorz ? nMargin : 0;
    const int nYMargin = bHorz ? 0 : nMargin;

    rectParent.DeflateRect(nXMargin, nYMargin);

    if (bHorz)
    {
        rectParent.right -= sizeImage.cx + nSeparatorSize - 2 + sizeExtra.cx;
        rectArrow.left = rectParent.right + 1;

        if (sizeExtra != CSize(0, 0))
        {
            rectArrow.OffsetRect(-sizeExtra.cx / 2 + 1, -sizeExtra.cy / 2 + 1);
        }
    }
    else
    {
        rectParent.bottom -= sizeImage.cy + nSeparatorSize - 1;
        rectArrow.top = rectParent.bottom;
    }

    BOOL bDisableFill = m_bDisableFill;
    m_bDisableFill = TRUE;

    UINT old_style = m_nStyle;

    if (m_dropdown)
        m_nStyle |= TBBS_PRESSED;

    CMFCToolBarButton::OnDraw(pDC, rectParent, pImages, bHorz, bCustomizeMode, bHighlight, bDrawBorder, bGrayDisabledButtons);

    m_bDisableFill = bDisableFill;

    //----------------
    // Draw separator:
    //----------------
    CRect rectSeparator = rectArrow;

    if (bHorz)
    {
        rectSeparator.right = rectSeparator.left + nSeparatorSize;
    }
    else
    {
        rectSeparator.bottom = rectSeparator.top + nSeparatorSize;
    }

    BOOL bDisabled = (bCustomizeMode && !IsEditable()) || (!bCustomizeMode && (m_nStyle & TBBS_DISABLED));

    int iImage;
    if (bHorz)
    {
        iImage = CMenuImages::IdArrowDown;
    }
    else
    {
        iImage = CMenuImages::IdArrowRight;
    }

    CMenuImages::Draw(pDC, (CMenuImages::IMAGES_IDS) iImage, rectArrow, bDisabled ? CMenuImages::ImageGray : CMenuImages::ImageBlack, sizeImage);

    if ((m_nStyle & (TBBS_PRESSED | TBBS_CHECKED)))
    {
        //-----------------------
        // Pressed in or checked:
        //-----------------------
        rectParent.right++;

        CMFCVisualManager::GetInstance()->OnDrawButtonBorder(pDC, this, rectParent, CMFCVisualManager::ButtonsIsPressed);
        CMFCVisualManager::GetInstance()->OnDrawButtonBorder(pDC, this, rectArrow, CMFCVisualManager::ButtonsIsPressed);
    }
    else if (bHighlight && !(m_nStyle & TBBS_DISABLED) && !(m_nStyle & (TBBS_CHECKED | TBBS_INDETERMINATE)))
    {
        CMFCVisualManager::GetInstance()->OnDrawButtonBorder(pDC, this, rect, CMFCVisualManager::ButtonsIsHighlighted);
    }

    m_nStyle = old_style;
}

SIZE TableToolbarButton::OnCalculateSize(CDC* pDC, const CSize& sizeDefault, BOOL bHorz)
{
    m_bHorz = bHorz;

    if (!IsVisible())
    {
        return CSize(0, 0);
    }

    int nArrowSize = 0;
    const int nSeparatorSize = 2;

    nArrowSize = (bHorz) ? CMenuImages::Size().cx : CMenuImages::Size().cy;

    if (CMFCToolBar::IsLargeIcons())
    {
        nArrowSize *= 2;
    }

    nArrowSize += nSeparatorSize - AFX_TEXT_MARGIN - 1;

    CSize size = CMFCToolBarButton::OnCalculateSize(pDC, sizeDefault, bHorz);

    if (bHorz)
    {
        size.cx += nArrowSize;
    }
    else
    {
        size.cy += nArrowSize;
    }

    const int nMargin = CMFCVisualManager::GetInstance()->GetMenuImageMargin();

    if (bHorz)
    {
        size.cx += nMargin * 2;
    }
    else
    {
        size.cy += nMargin * 2;
    }

    return size;
}

BOOL TableToolbarButton::OnClick(CWnd* /*pWnd*/, BOOL /*bDelay*/)
{
    if (m_dropdown) {
        CloseDropdown();
    }
    else {
        OpenDropdown();
    }
    return FALSE;
}

void TableToolbarButton::OpenDropdown()
{
    m_dropdown = new TableToolbarDropdown();
    m_dropdown->Create(this);
}

void TableToolbarButton::CloseDropdown()
{
    if (m_dropdown) {
        m_dropdown->DestroyWindow();
        delete m_dropdown;
        m_dropdown = nullptr;
        GetParentWnd()->InvalidateRect(&Rect());
    }
}

void TableToolbarButton::OnTableDimensionsSelected(CSize size)
{
    m_dimensions = std::move(size);
    GetParentWnd()->SendMessage(WM_COMMAND, m_nID);
}
