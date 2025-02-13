#pragma once

#include <zCapiO/ExtendedControl.h>


// CCapiControl form view

class CCapiControl : public CFormView
{
public:
    CCapiControl();           // protected constructor used by dynamic creation

    enum { IDD = IDD_CAPICONTROL };

    void LaunchWindow(CExtendedControl* pParent);

    // Layout controls (images, radio buttons, texts...)
    // to best fit the client rect. May end up using more
    // space than is available in clientWidth if controls don't fit.
    // Will attempt to use up the entire width if possible rather
    // than drawing controls on the left side of the window.
    void LayoutControls();

    // Get smallest size required to draw all controls.
    CSize GetControlsMinSize();

    CSize Filter(wstring_view filter_sv);

    void UpdateSelection(const CString& keyedText);

protected:
    LRESULT WindowProc(UINT message,WPARAM wParam,LPARAM lParam);
    BOOL PreTranslateMessage(MSG* pMsg);

private:
    COleDateTime TranslateStringToDate(CString fieldDateString);
    CString TranslateDateToString();

    void TranslateStringToCheckbox(CString checkboxString);
    CString TranslateCheckboxToString();

    int SearchVS(CString searchText);
    bool GetVSSelection(int vsID, CString* newFieldText);

    void ScrollToSelectedPosition(int valueSelected);

    void CreateControls();

    // Layout a 2D array of controls in a grid.
    // Sets width for entire column based on the widest control in that column. Row heights
    // are variable based on contents of rows. Specify horizontal alignment of controls within columns
    // by passing  in columnAlignments.
    CSize LayoutInGrid(const CArray< CArray<CWnd*>* > &columns, // controls to layout
                       const CArray<UINT> &columnAlignments, // horizontal alignment of items within column (LVCFMT_LEFT, LVCFMT_CENTER or LVCFMT_RIGHT)
                       int border, // Border in px between edges of grid and edges of window
                       int colSpacing, // Spacing in space characters in current font between adjacent columns
                       int rowSpacing, // Spacing in px characters in current font between adjacent rows
                       const CArray<CWnd*> &rowBackgrounds, // optional row backgrounds (empty CStatics) used for highlighting
                       bool sizeOnly); // only compute size, don't move controls

    void CreateStaticControlWithImage(CStatic& pControl, UINT controlId, const CString& imagePath);

    int CharWidthInPixels();

private:
    CExtendedControl* m_pParent;

    CMonthCalCtrl m_Calendar;

    CButton m_JunkButton;
    CArray<CButton> m_buttons;
    CArray<CStatic> m_values;
    CArray<CStatic> m_labels;
    CArray<CStatic> m_images;
    CArray<CStatic> m_rowBackgrounds;
    std::vector<PortableColor> m_textColors;

    CImageList m_imageList;

    CBrush m_backgroundBrush;
    CBrush m_selectedBackgroundBrush;
    CBrush m_textBrush;
    CBrush m_selectedTextBrush;

    int m_selectedComboBoxDropdownIndex;

    CString m_filterString;
};
