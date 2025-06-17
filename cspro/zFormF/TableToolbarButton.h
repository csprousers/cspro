#pragma once
class TableToolbarDropdown;

class TableToolbarButton : public CMFCToolBarButton
{
    DECLARE_SERIAL(TableToolbarButton)
public:

    TableToolbarButton();
    TableToolbarButton(UINT uiID, int iImage, LPCTSTR lpszText = NULL);

    void OnDraw(CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
        BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE) override;
    SIZE OnCalculateSize(CDC* pDC, const CSize& sizeDefault, BOOL bHorz) override;
    BOOL OnClick(CWnd* pWnd, BOOL bDelay = TRUE) override;
    BOOL OnClickUp() override  { return TRUE; }

    void OpenDropdown();
    void CloseDropdown();
    void OnTableDimensionsSelected(CSize dimensions);

    CSize GetDimensions() const {
        return m_dimensions;
    }

protected:
    TableToolbarDropdown* m_dropdown;
    CSize m_dimensions;
};

