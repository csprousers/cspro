#include "StdAfx.h"
#include "CapiControl.h"
#include <zUtilO/Interapp.h>
#include <zUtilF/ImageManager.h>
#include <zEngineO/ResponseProcessor.h>
#include <CSEntry/UWM.h>


CCapiControl::CCapiControl()
    :   CFormView(CCapiControl::IDD),
        m_pParent(nullptr),
        m_selectedComboBoxDropdownIndex(-1)
{
}

void CCapiControl::LaunchWindow(CExtendedControl * pParent)
{
    m_pParent = pParent;
    m_filterString = CString();

    switch( m_pParent->m_captureType )
    {
        case CaptureType::DropDown:
        case CaptureType::ComboBox:
        case CaptureType::CheckBox:
        case CaptureType::ToggleButton:
            // White background like list control
            m_backgroundBrush.CreateSysColorBrush(COLOR_WINDOW);
            break;
        default:
            // Grey dialog background color
            m_backgroundBrush.CreateSysColorBrush(COLOR_3DFACE);
    }

    m_selectedBackgroundBrush.CreateSysColorBrush(COLOR_HIGHLIGHT);
    m_selectedComboBoxDropdownIndex = -1;

    CRect rect;
    m_pParent->GetClientRect(&rect);
    Create(NULL,NULL,WS_CHILD | WS_VISIBLE,rect,m_pParent,CCapiControl::IDD,NULL);

    CreateControls();
}

void CCapiControl::CreateControls()
{
    CRect controlRect;

    auto process_label = [&](CClientDC& pDC, INT_PTR i, CString label_text)
    {
        if( label_text.IsEmpty() ) // empty labels were messing up the spacing of the labels
            label_text = _T(" ");

        label_text.Replace(_T("&"), _T("&&")); // prevent Windows from converting &s to accelerator keys

        CRect textRect;
        pDC.DrawTextEx(label_text, &textRect, DT_CALCRECT, nullptr);

        controlRect.top = controlRect.left = 0;
        controlRect.right = textRect.Width();
        controlRect.bottom = textRect.Height();

        m_labels[i].Create(label_text, WS_CHILD | WS_VISIBLE | SS_NOTIFY, controlRect, this, EXTENDED_CONTROL_RESOURCE_ID + 1 + i);
        m_labels[i].SetFont(m_pParent->GetControlFont());
    };


    if( m_pParent->m_captureType == CaptureType::Date )
    {
        CPoint pt(EXTENDED_CONTROL_BORDER_SIZE,EXTENDED_CONTROL_BORDER_SIZE);
        m_Calendar.Create(WS_CHILD | WS_VISIBLE | WS_BORDER,pt,this,EXTENDED_CONTROL_RESOURCE_ID);
    }

    else if( m_pParent->m_captureType == CaptureType::DropDown || m_pParent->m_captureType == CaptureType::ComboBox )
    {
        const auto& responses = m_pParent->m_responseProcessor->GetResponses();
        INT_PTR number_responses = (INT_PTR)responses.size();

        m_values.SetSize(number_responses);
        m_labels.SetSize(number_responses);
        m_images.SetSize(number_responses);
        m_rowBackgrounds.SetSize(number_responses);

        CClientDC pDC(this);
        pDC.SelectObject(m_pParent->GetControlFont());

        // this hidden button makes it so that when the dialog regains focus the first legit radio button isn't
        // selected automatically (instead this junk radio button is)

        CRect hiddenButtonRect(0,0,0,0);
        m_JunkButton.Create(_T(""),WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,hiddenButtonRect,this,EXTENDED_CONTROL_RESOURCE_ID);


        // first display just the values, then we'll display the labels
        for( INT_PTR i = 0; i < number_responses; i++ )
        {
            // Add a row background - a big empty static whose background color we can change when
            // the row is selected. This needs to be added first so that it is not drawn on top.
            // It will get sized correctly in the LayoutInGrid method.
            m_rowBackgrounds[i].Create(_T(""),WS_CHILD | WS_VISIBLE,controlRect,this,EXTENDED_CONTROL_RESOURCE_ID + 1 + i);

            CString code_text = responses[i]->GetCode();

            if( code_text.IsEmpty() )
                code_text = _T(" ");

            CSize textSize = pDC.GetTextExtent(code_text);

            controlRect.top = controlRect.left = 0;
            controlRect.right = textSize.cx;
            controlRect.bottom = textSize.cy;

            m_values[i].Create(code_text,WS_CHILD | WS_VISIBLE | SS_NOTIFY,controlRect,this,EXTENDED_CONTROL_RESOURCE_ID + 1 + i);
            m_values[i].SetFont(m_pParent->GetControlFont());
        }

        // now draw the labels in a second column
        for( INT_PTR i = 0; i < number_responses; i++ )
            process_label(pDC, i, responses[i]->GetLabel());

        // Now the images and colors
        for( INT_PTR i = 0; i < number_responses; i++ )
        {
            CreateStaticControlWithImage(m_images[i], EXTENDED_CONTROL_RESOURCE_ID + 1 + i, responses[i]->GetImageFilename());
            m_textColors.emplace_back(responses[i]->GetTextColor());
        }
    }


    else if( m_pParent->m_captureType == CaptureType::CheckBox || m_pParent->m_captureType == CaptureType::RadioButton ||
             m_pParent->m_captureType == CaptureType::ToggleButton )
    {
        const auto& responses = m_pParent->m_responseProcessor->GetResponses();
        INT_PTR number_responses = (INT_PTR)responses.size();

        m_buttons.SetSize(number_responses);
        m_labels.SetSize(number_responses);
        m_images.SetSize(number_responses);

        CClientDC pDC(this);
        pDC.SelectObject(m_pParent->GetControlFont());

        // this hidden button makes it so that when the dialog regains focus the first legit radio button isn't
        // selected automatically (instead this junk radio button is)

        CRect hiddenButtonRect(0,0,0,0);
        m_JunkButton.Create(_T(""),WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,hiddenButtonRect,this,EXTENDED_CONTROL_RESOURCE_ID);

        // first display just the values, then we'll display the labels

        int characterWidth = CharWidthInPixels();

        for( INT_PTR i = 0; i < number_responses; i++ )
        {
            CString code_text = responses[i]->GetCode();

            if( code_text.IsEmpty() )
                code_text = _T(" ");

            CSize textSize = pDC.GetTextExtent(code_text);

            controlRect.top = controlRect.left = 0;
            controlRect.right = textSize.cx + 3 * characterWidth; // Add 3 characters spacing to leave space for button
            controlRect.bottom = textSize.cy;

            DWORD dwStyle = WS_CHILD | WS_VISIBLE;

            if( m_pParent->m_captureType == CaptureType::CheckBox || m_pParent->m_captureType == CaptureType::ToggleButton )
                dwStyle |= BS_AUTOCHECKBOX;

            // Radio button
            else
                dwStyle |= BS_AUTORADIOBUTTON;

            m_buttons[i].Create(code_text,dwStyle,controlRect,this,EXTENDED_CONTROL_RESOURCE_ID + 1 + i);
            m_buttons[i].SetFont(m_pParent->GetControlFont());
        }

        // now draw the labels in a second column
        for( INT_PTR i = 0; i < number_responses; i++ )
            process_label(pDC, i, responses[i]->GetLabel());

        // Now the images and colors
        for( INT_PTR i = 0; i < number_responses; i++ )
        {
            CreateStaticControlWithImage(m_images[i], EXTENDED_CONTROL_RESOURCE_ID + 1 + i, responses[i]->GetImageFilename());
            m_textColors.emplace_back(responses[i]->GetTextColor());
        }
    }

    else if( m_pParent->m_captureType == CaptureType::NumberPad )
    {
        CFont* pFont = m_pParent->GetControlFont(true);
        LOGFONT lf;
        pFont->GetLogFont(&lf);

        int buttonWidth = std::max(abs(lf.lfWidth),abs(lf.lfHeight)) + EXTENDED_CONTROL_BORDER_SIZE;

        /*const TCHAR digits[4][4] =    {   _T('7'), _T('8'), _T('9'), _T('←'),
                                            _T('4'), _T('5'), _T('6'), _T('-'),
                                            _T('1'), _T('2'), _T('3'), _T('→'),
                                            _T('0'), _T('0'), _T('.'), _T('→'),
                                        }; */

        m_buttons.SetSize(16);
        int buttonNumber = 0;

        CRect rect;

        for( int x = 0; x < 3; x++ )
        {
            for( int y = 0; y < 3; y++ )
            {
                CString text = IntToString(( 2 - y ) * 3 + x + 1);

                rect.left = EXTENDED_CONTROL_BORDER_SIZE * ( x + 1 ) + buttonWidth * x;
                rect.right = rect.left + buttonWidth;

                rect.top = EXTENDED_CONTROL_BORDER_SIZE * ( y + 1 ) + buttonWidth * y;
                rect.bottom = rect.top + buttonWidth;

                m_buttons[buttonNumber].Create(text,WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_CENTER | BS_VCENTER,rect,this,EXTENDED_CONTROL_RESOURCE_ID + buttonNumber);
                buttonNumber++;
            }
        }

        rect.left = EXTENDED_CONTROL_BORDER_SIZE * ( 3 + 1 ) + buttonWidth * 3;
        rect.right = rect.left + buttonWidth;

        // backspace
        rect.top = EXTENDED_CONTROL_BORDER_SIZE * ( 0 + 1 ) + buttonWidth * 0;
        rect.bottom = rect.top + buttonWidth;
        m_buttons[buttonNumber].Create(_T("←"),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_CENTER | BS_VCENTER,rect,this,EXTENDED_CONTROL_RESOURCE_ID + buttonNumber);
        buttonNumber++;

        // minus
        rect.top = EXTENDED_CONTROL_BORDER_SIZE * ( 1 + 1 ) + buttonWidth * 1;
        rect.bottom = rect.top + buttonWidth;
        m_buttons[buttonNumber].Create(_T("–"),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_CENTER | BS_VCENTER,rect,this,EXTENDED_CONTROL_RESOURCE_ID + buttonNumber);
        buttonNumber++;

        // delete
        rect.top = EXTENDED_CONTROL_BORDER_SIZE * ( 2 + 1 ) + buttonWidth * 2;
        rect.bottom = rect.top + buttonWidth;
        m_buttons[buttonNumber].Create(_T("C"),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_CENTER | BS_VCENTER,rect,this,EXTENDED_CONTROL_RESOURCE_ID + buttonNumber);
        buttonNumber++;

        rect.top = EXTENDED_CONTROL_BORDER_SIZE * ( 3 + 1 ) + buttonWidth * 3;
        rect.bottom = rect.top + buttonWidth;

        // 0
        rect.left = EXTENDED_CONTROL_BORDER_SIZE * ( 0 + 1 ) + buttonWidth * 0;
        rect.right = rect.left + buttonWidth;
        m_buttons[buttonNumber].Create(_T("0"),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_CENTER | BS_VCENTER,rect,this,EXTENDED_CONTROL_RESOURCE_ID + buttonNumber);
        buttonNumber++;

        // decimal point
        TCHAR * decimalChar = m_pParent->m_bCommaDecimal ? _T(",") : _T("."); // GHM 20130704

        rect.left = EXTENDED_CONTROL_BORDER_SIZE * 2 + buttonWidth;
        rect.right = rect.left + buttonWidth;
        m_buttons[buttonNumber].Create(decimalChar,WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_CENTER | BS_VCENTER,rect,this,EXTENDED_CONTROL_RESOURCE_ID + buttonNumber);
        buttonNumber++;

        // prev
        rect.left = EXTENDED_CONTROL_BORDER_SIZE * ( 2 + 1 ) + buttonWidth * 2;
        rect.right = rect.left + buttonWidth;
        m_buttons[buttonNumber].Create(_T("\u25C0"),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_CENTER | BS_VCENTER,rect,this,EXTENDED_CONTROL_RESOURCE_ID + buttonNumber);
        buttonNumber++;

        // next
        rect.left = EXTENDED_CONTROL_BORDER_SIZE * ( 3 + 1 ) + buttonWidth * 3;
        rect.right = rect.left + buttonWidth;
        m_buttons[buttonNumber].Create(_T("\u25B6"),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT | BS_CENTER | BS_VCENTER,rect,this,EXTENDED_CONTROL_RESOURCE_ID + buttonNumber);
        buttonNumber++;

        for( int i = 0; i < buttonNumber; i++ )
            m_buttons[i].SetFont(pFont);

        SetWindowLong(m_hWnd,GWL_EXSTYLE,GetWindowLong(m_hWnd,GWL_EXSTYLE) | WS_EX_NOACTIVATE);
    }
}

CSize CCapiControl::GetControlsMinSize()
{
    CSize result;

    if( m_pParent->m_captureType == CaptureType::Date )
    {
        CRect calendarRect;
        m_Calendar.GetClientRect(&calendarRect);
        result = calendarRect.Size();
    }

    else if( m_pParent->m_captureType == CaptureType::DropDown || m_pParent->m_captureType == CaptureType::ComboBox )
    {
        CArray< CArray<CWnd*>* > controls;
        CArray<CWnd*> values, labels, images, rowBackgrounds;
        for (int i = 0; i < m_values.GetCount(); i++) {
            values.Add(&m_values.GetAt(i));
            labels.Add(&m_labels.GetAt(i));
            images.Add(&m_images.GetAt(i));
        }
        controls.Add(&values);
        controls.Add(&labels);
        controls.Add(&images);
        CArray<UINT> alignments;
        alignments.Add(LVCFMT_LEFT);
        alignments.Add(LVCFMT_LEFT);
        alignments.Add(LVCFMT_CENTER);
        result = LayoutInGrid(controls, alignments, 0, EXTENDED_CONTROL_COL_SPACING, EXTENDED_CONTROL_ROW_SPACING, rowBackgrounds, true);
    }

    else if( m_pParent->m_captureType == CaptureType::CheckBox || m_pParent->m_captureType == CaptureType::RadioButton ||
             m_pParent->m_captureType == CaptureType::ToggleButton )
    {
        CArray< CArray<CWnd*>* > controls;
        CArray<CWnd*> buttons, labels, images, rowBackgrounds;
        for (int i = 0; i < m_buttons.GetCount(); i++) {
            buttons.Add(&m_buttons.GetAt(i));
            labels.Add(&m_labels.GetAt(i));
            images.Add(&m_images.GetAt(i));
        }
        controls.Add(&buttons);
        controls.Add(&labels);
        controls.Add(&images);
        CArray<UINT> alignments;
        alignments.Add(LVCFMT_LEFT);
        alignments.Add(LVCFMT_LEFT);
        alignments.Add(LVCFMT_CENTER);
        result = LayoutInGrid(controls, alignments, 0, EXTENDED_CONTROL_COL_SPACING, EXTENDED_CONTROL_ROW_SPACING, rowBackgrounds, true);
    }

    else if( m_pParent->m_captureType == CaptureType::NumberPad )
    {
        CRect buttonsRect;
        for (int i = 0; i < m_buttons.GetSize(); ++i) {
            CRect r;
            m_buttons[i].GetWindowRect(r);
            buttonsRect.UnionRect(buttonsRect, r);
        }

        result = buttonsRect.Size();
    }

    result.cx += 2 * EXTENDED_CONTROL_BORDER_SIZE;
    result.cy += 2 * EXTENDED_CONTROL_BORDER_SIZE;

    return result;
}

void CCapiControl::LayoutControls()
{
    if( m_pParent->m_captureType == CaptureType::Date )
    {
        // This is already done when computing size and window title is always
        // set to "Date" so it never needs re-layout.
    }
    else if( m_pParent->m_captureType == CaptureType::DropDown || m_pParent->m_captureType == CaptureType::ComboBox )
    {
        CArray< CArray<CWnd*>* > controls;
        CArray<CWnd*> values, labels, images, rowBackgrounds;
        for (int i = 0; i < m_values.GetCount(); i++) {
            values.Add(&m_values.GetAt(i));
            labels.Add(&m_labels.GetAt(i));
            images.Add(&m_images.GetAt(i));
            rowBackgrounds.Add(&m_rowBackgrounds.GetAt(i));
        }
        controls.Add(&values);
        controls.Add(&labels);
        controls.Add(&images);
        CArray<UINT> alignments;
        alignments.Add(LVCFMT_LEFT);
        alignments.Add(LVCFMT_LEFT);
        alignments.Add(LVCFMT_CENTER);
        LayoutInGrid(controls, alignments, EXTENDED_CONTROL_BORDER_SIZE, EXTENDED_CONTROL_COL_SPACING, EXTENDED_CONTROL_ROW_SPACING, rowBackgrounds, false);
    }
    else if( m_pParent->m_captureType == CaptureType::CheckBox || m_pParent->m_captureType == CaptureType::RadioButton ||
             m_pParent->m_captureType == CaptureType::ToggleButton )
    {
        CArray< CArray<CWnd*>* > controls;
        CArray<CWnd*> buttons, labels, images, rowBackgrounds;
        for (int i = 0; i < m_buttons.GetCount(); i++) {
            buttons.Add(&m_buttons.GetAt(i));
            labels.Add(&m_labels.GetAt(i));
            images.Add(&m_images.GetAt(i));
        }
        controls.Add(&buttons);
        controls.Add(&labels);
        controls.Add(&images);
        CArray<UINT> alignments;
        alignments.Add(LVCFMT_LEFT);
        alignments.Add(LVCFMT_LEFT);
        alignments.Add(LVCFMT_CENTER);
        LayoutInGrid(controls, alignments, EXTENDED_CONTROL_BORDER_SIZE, EXTENDED_CONTROL_COL_SPACING, EXTENDED_CONTROL_ROW_SPACING, rowBackgrounds, false);
    }

    else if( m_pParent->m_captureType == CaptureType::NumberPad )
    {
        // This is already done when computing size and window title is always
        // set to "Numberpad" so it never needs re-layout.
    }

}


void CCapiControl::UpdateSelection(const CString& keyedText)
{
    if( m_pParent->m_captureType == CaptureType::Date )
    {
        COleDateTime fieldDate = TranslateStringToDate(keyedText);
        m_Calendar.SetCurSel(fieldDate);
    }

    else if( m_pParent->m_captureType == CaptureType::DropDown || m_pParent->m_captureType == CaptureType::ComboBox )
    {
        int valueSelected = SearchVS(keyedText);

        if (m_selectedComboBoxDropdownIndex != -1) {
            CRect oldSelectedRect;
            m_labels.GetAt(m_selectedComboBoxDropdownIndex).GetWindowRect(&oldSelectedRect);
            ScreenToClient(oldSelectedRect);
            InvalidateRect(oldSelectedRect);
            m_values.GetAt(m_selectedComboBoxDropdownIndex).GetWindowRect(&oldSelectedRect);
            ScreenToClient(oldSelectedRect);
            InvalidateRect(oldSelectedRect);
            m_rowBackgrounds.GetAt(m_selectedComboBoxDropdownIndex).GetWindowRect(&oldSelectedRect);
            ScreenToClient(oldSelectedRect);
            InvalidateRect(oldSelectedRect);
        }

        m_selectedComboBoxDropdownIndex = valueSelected;

        if( valueSelected != -1 )
        {
            CRect oldSelectedRect;
            m_labels.GetAt(m_selectedComboBoxDropdownIndex).GetWindowRect(&oldSelectedRect);
            ScreenToClient(oldSelectedRect);
            InvalidateRect(oldSelectedRect);
            m_values.GetAt(m_selectedComboBoxDropdownIndex).GetWindowRect(&oldSelectedRect);
            ScreenToClient(oldSelectedRect);
            InvalidateRect(oldSelectedRect);
            m_rowBackgrounds.GetAt(m_selectedComboBoxDropdownIndex).GetWindowRect(&oldSelectedRect);
            ScreenToClient(oldSelectedRect);
            InvalidateRect(oldSelectedRect);
            ScrollToSelectedPosition(valueSelected);
        }
    }

    else if( m_pParent->m_captureType == CaptureType::CheckBox )
        TranslateStringToCheckbox(keyedText);

    else if( m_pParent->m_captureType == CaptureType::RadioButton )
    {
        int valueSelected = SearchVS(keyedText);

        for( int i = 0; i < m_buttons.GetSize(); i++ ) // make sure no values are checked
            m_buttons[i].SetCheck(BST_UNCHECKED);

        if( valueSelected != -1 )
        {
            m_buttons[valueSelected].SetCheck(BST_CHECKED);
            ScrollToSelectedPosition(valueSelected);
        }

        else
            m_JunkButton.SetCheck(BST_CHECKED);
    }

    else if( m_pParent->m_captureType == CaptureType::ToggleButton )
    {
        ASSERT(m_buttons.GetSize() == 1);
        int valueSelected = SearchVS(keyedText);
        m_buttons.GetAt(0).SetCheck(( valueSelected != -1 ) ? BST_CHECKED : BST_UNCHECKED);
    }
}


void CCapiControl::ScrollToSelectedPosition(int valueSelected)
{
    BOOL horzBar,vertBar;

    CheckScrollBars(horzBar,vertBar);

    if( !vertBar )
        return;

    SCROLLINFO si;
    ZeroMemory(&si,sizeof(SCROLLINFO));
    si.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
    GetScrollInfo(SB_VERT,&si);

    int farDownInList = (int) (double(valueSelected) / m_pParent->m_responseProcessor->GetResponses().size() * si.nMax);
    int bufferZone =std::min(si.nMax,farDownInList + 50); // make sure that we're not selecting the very most item in the list

    // aim for the selected value to be in the middle of the screen
    CPoint newScrollPos;
    newScrollPos.x = 0;
    newScrollPos.y =std::max((UINT) 0,farDownInList - si.nPage / 2);

    if( bufferZone < si.nPos || ((UINT) bufferZone) > ( si.nPos + si.nPage ) )
        ScrollToPosition(newScrollPos);
}


int CCapiControl::SearchVS(CString searchText)
{
    // format the input of numerics with decimals so that it matches how it will appear when fully entered
    if( m_pParent->m_pDictItem->GetContentType() == ContentType::Numeric )
    {
        if( m_pParent->m_pDictItem->GetDecimal() > 0 )
        {
            searchText.Replace(_T(','), _T('.')); // in case the user is using commas for decimal marks

            if( !m_pParent->m_pDictItem->GetDecChar() )
                searchText.Remove('.');

            bool string_is_empty = true;

            for( int i = 0; string_is_empty && i < searchText.GetLength(); i++ )
                string_is_empty = ( searchText.GetAt(i) == _T(' ') );

            if( !string_is_empty )
            {
                // zero fill any spaces to decimals
                for( UINT i = 1; i <= m_pParent->m_pDictItem->GetDecimal(); i++ )
                {
                    if( searchText.GetAt(searchText.GetLength() - i) == _T(' ') )
                        searchText.SetAt(searchText.GetLength() - i, _T('0'));

                    else
                        break;
                }
            }
        }
    }

    size_t index = m_pParent->m_responseProcessor->GetResponseIndex(searchText);

    return ( index == SIZE_MAX ) ? -1 : (int)index;
}


bool CCapiControl::GetVSSelection(int vsID, CString* newFieldText)
{
    try
    {
        *newFieldText = m_pParent->m_responseProcessor->GetInputFromResponseIndex(vsID);
        return true;
    }

    catch( const ResponseProcessor::SelectionError& )
    {
        // this will occur if the user clicks on a range
    }

    return false;
}


LRESULT CCapiControl::WindowProc(UINT message,WPARAM wParam,LPARAM lParam)
{
    if (message == WM_CTLCOLORDLG) {
        // Set background color to white to look like list control
        return (INT_PTR) m_backgroundBrush.GetSafeHandle();
    }

    if (message == WM_CTLCOLORSTATIC) {

        UINT id   = GetWindowLongPtr((HWND) lParam, GWL_ID);
        int index = id - EXTENDED_CONTROL_RESOURCE_ID - 1;

        COLORREF rgb;
        HBRUSH hBrush;
        if (index == m_selectedComboBoxDropdownIndex)
        {
            rgb    = GetSysColor(COLOR_HIGHLIGHTTEXT);
            hBrush = reinterpret_cast<HBRUSH>(m_selectedBackgroundBrush.GetSafeHandle());
        }
        else
        {
            rgb    = m_textColors[index].ToCOLORREF();
            hBrush = reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        }

        HDC hDC    = reinterpret_cast<HDC>(wParam);
        CDC* pDC   = CDC::FromHandle(hDC);

        pDC->SetTextColor(rgb);
        pDC->SetBkMode(TRANSPARENT);
        return reinterpret_cast<DWORD_PTR>(hBrush);
    }

    if( message == WM_NOTIFY )
    {
        NMHDR* pNMHDR = (LPNMHDR)lParam;

        if( pNMHDR->code == MCN_SELECT && ( m_pParent->m_captureType == CaptureType::Date ) )
        {
            CString newFieldText;
            newFieldText = TranslateDateToString();
            m_pParent->m_pEdit->SendMessage(UWM::CSEntry::ControlsSetWindowText,(WPARAM)m_pParent->m_captureType,(LPARAM)&newFieldText);
            m_pParent->m_pEdit->SetFocus();
        }

        else if( pNMHDR->code == NM_CLICK && ( m_pParent->m_captureType == CaptureType::DropDown || m_pParent->m_captureType == CaptureType::ComboBox ) )
        {
            int thisSelection = ((LPNMLISTVIEW)lParam)->iItem;

            if( thisSelection >= 0 )
            {
                CString newFieldText;
                if( GetVSSelection(thisSelection, &newFieldText) )
                    m_pParent->m_pEdit->SendMessage(UWM::CSEntry::ControlsSetWindowText,(WPARAM)m_pParent->m_captureType,(LPARAM)&newFieldText);
            }

            m_pParent->m_pEdit->SetFocus();
        }
    }

    else if( message == WM_COMMAND && ( HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == STN_CLICKED ) )
    {
        int buttonClicked = LOWORD(wParam) - EXTENDED_CONTROL_RESOURCE_ID;

        if( m_pParent->m_captureType == CaptureType::RadioButton ||
            m_pParent->m_captureType == CaptureType::DropDown ||
            m_pParent->m_captureType == CaptureType::ComboBox )
        {
            if( buttonClicked >= 1 && buttonClicked <=std::max(m_buttons.GetSize(), m_labels.GetSize()) ) // otherwise it's a different button
            {
                DefWindowProc(message,wParam,lParam);

                CString newFieldText;
                if( GetVSSelection(buttonClicked - 1, &newFieldText) )
                {
                    m_pParent->m_pEdit->SendMessage(UWM::CSEntry::ControlsSetWindowText,(WPARAM)m_pParent->m_captureType,(LPARAM)&newFieldText);
                    m_pParent->m_pEdit->SetFocus();
                    return 0;
                }
            }
        }

        else if( m_pParent->m_captureType == CaptureType::CheckBox ||
                 m_pParent->m_captureType == CaptureType::ToggleButton )
        {
            if( buttonClicked >= 1 && buttonClicked <= m_buttons.GetSize() ) // otherwise it's a different button
            {
                AfxTrace(_T("CHECKBOX: %d current=%d handle=%x\n"), buttonClicked, m_buttons.GetAt(buttonClicked-1).GetCheck(),
                    lParam);
                // If they clicked on label or image instead of checkbox need to toggle checkbox,
                // otherwise Windows does the toggle for us
                if ((HWND) lParam == m_labels.GetAt(buttonClicked-1).m_hWnd ||
                    (HWND) lParam == m_images.GetAt(buttonClicked-1).m_hWnd) {
                    int currentState = m_buttons.GetAt(buttonClicked-1).GetCheck();
                    m_buttons.GetAt(buttonClicked-1).SetCheck(currentState == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
                    AfxTrace(_T("off box toggle to: %d\n"), m_buttons.GetAt(buttonClicked-1).GetCheck());
                }

                // make sure they haven't selected too many options
                int iNumSelected = 0;

                for( int i = 0; i < m_buttons.GetCount(); i++ )
                {
                    if(m_buttons.GetAt(i).GetCheck() == BST_CHECKED)
                        iNumSelected++;
                }

                if( m_pParent->m_captureType == CaptureType::CheckBox )
                {
                    if( (size_t)iNumSelected > m_pParent->m_responseProcessor->GetCheckboxMaxSelections() ) { // cancel the selection
                        AfxTrace(_T("Too many selected uncheck and return\n"));
                        m_buttons.GetAt(buttonClicked-1).SetCheck(BST_UNCHECKED);
                        return TRUE;
                    } else {
                        CString newFieldText;
                        newFieldText = TranslateCheckboxToString();
                        AfxTrace(_T("Update text %s\n"), (LPCTSTR)newFieldText);
                        m_pParent->m_pEdit->SendMessage(UWM::CSEntry::ControlsSetWindowText,(WPARAM)m_pParent->m_captureType,(LPARAM)&newFieldText);
                    }
                }

                else
                {
                    ASSERT(m_pParent->m_captureType == CaptureType::ToggleButton);
                    ASSERT(iNumSelected == 0 || iNumSelected == 1);

                    CString new_field_text;

                    if( iNumSelected == 0 || GetVSSelection(0, &new_field_text) )
                    {
                        m_pParent->m_pEdit->SendMessage(UWM::CSEntry::ControlsSetWindowText,
                            (WPARAM)m_pParent->m_captureType, (LPARAM)&new_field_text);
                    }
                }

                return 0;
            }
        }

        else if( m_pParent->m_captureType == CaptureType::NumberPad ) // GHM 20130418 for the number pad
        {
            const TCHAR characters[16] = { 55, 52, 49, 56, 53, 50, 57, 54, 51, 8, VK_OEM_MINUS, VK_DELETE, 48, VK_OEM_PERIOD, VK_LEFT, 13 };

            ASSERT(buttonClicked >= 0 && buttonClicked <= 16);
            TCHAR charPressed;

            if( buttonClicked == 13 ) // GHM 20130704 . or , (the decimal point)
                charPressed = m_pParent->m_bCommaDecimal ? VK_OEM_COMMA : VK_OEM_PERIOD;

            else
                charPressed = characters[buttonClicked];

            ::PostMessage(m_pParent->m_pEdit->m_hWnd,WM_KEYDOWN,charPressed,1);

            return 0;
        }
    }
    else if (message == WM_DESTROY) {
        // Pass message to base class WindowProc which will call PostNcDestroy and delete this object
        // is the preferred way of deleting the C++ obj associated with the HWND.
        // FormView::PostNcDestroy already calls "delete this" so we don't need to implement PostNcDestroy in CCapiControl
        // https://docs.microsoft.com/en-us/cpp/mfc/tn017-destroying-window-objects?view=vs-2019
        return CFormView::WindowProc(message, wParam, lParam);
    }
    else if (message == WM_NCDESTROY)
    {
        return CFormView::WindowProc(message, wParam, lParam);
    }

    else if (message == WM_VSCROLL || message == WM_HSCROLL) // GHM 20100622
        return CScrollView::WindowProc(message, wParam, lParam);

    return DefWindowProc(message,wParam,lParam);
}



BOOL CCapiControl::PreTranslateMessage(MSG * pMsg)
{
    // pass all keystrokes to the field
    if( pMsg->message == WM_KEYDOWN )
    {
        m_pParent->m_pEdit->PostMessage(WM_KEYDOWN,pMsg->wParam,pMsg->lParam);
        return TRUE;
    }

    else if( pMsg->message == WM_CHAR )
    {
        m_pParent->m_pEdit->PostMessage(WM_CHAR,pMsg->wParam,pMsg->lParam);
        return TRUE;
    }

    else
        return CFormView::PreTranslateMessage(pMsg);
}

void CCapiControl::TranslateStringToCheckbox(CString checkboxString)
{
    std::vector<size_t> checked_indices = m_pParent->m_responseProcessor->GetCheckboxResponseIndices(checkboxString);
    size_t checked_indices_iterator = 0;

    for( int i = 0; i < m_buttons.GetSize(); i++ )
    {
        bool select = false;

        if( checked_indices_iterator < checked_indices.size() && checked_indices[checked_indices_iterator] == (size_t)i )
        {
            select = true;
            checked_indices_iterator++;
        }

        m_buttons.GetAt(i).SetCheck(select ? BST_CHECKED : BST_UNCHECKED);
    }
}


CString CCapiControl::TranslateCheckboxToString()
{
    std::vector<size_t> checked_indices;

    for( int i = 0; i < m_buttons.GetSize() ; i++ )
    {
        if( m_buttons.GetAt(i).GetCheck() == BST_CHECKED )
            checked_indices.push_back(i);
    }

    if( checked_indices.size() > m_pParent->m_responseProcessor->GetCheckboxMaxSelections() )
        checked_indices.resize(m_pParent->m_responseProcessor->GetCheckboxMaxSelections());

    return m_pParent->m_responseProcessor->GetInputFromCheckboxIndices(checked_indices);
}



COleDateTime CCapiControl::TranslateStringToDate(CString fieldDateString)
{
    const CString& csFormat = m_pParent->m_captureInfo.GetExtended<DateCaptureInfo>().GetFormat();

    fieldDateString.TrimLeft();

    // we start with the current date and modify the day, month, and year as needed
    int iPosDay = csFormat.Find(_T("DD"));
    int iPosMonth = csFormat.Find(_T("MM"));
    int iPos4Year = csFormat.Find(_T("YYYY"));
    int iPos2Year = ( iPos4Year >= 0 ) ? -1 : csFormat.Find(_T("YY"));

    CString csDay;
    CString csMonth;
    CString csYear;

    if( iPosDay >= 0 )
        csDay = fieldDateString.Mid(iPosDay,2);

    if( iPosDay >= 0 )
        csMonth = fieldDateString.Mid(iPosMonth,2);

    if( iPos4Year >= 0 )
        csYear = fieldDateString.Mid(iPos4Year,4);

    else if( iPos2Year >= 0 )
        csYear = fieldDateString.Mid(iPos2Year,2);

    COleDateTime todayDate = CTime::GetCurrentTime().GetTime();
    int iYear = todayDate.GetYear();
    int iMonth = todayDate.GetMonth();
    int iDay = todayDate.GetDay();

    int iUserYear = _ttoi(csYear);
    int iUserMonth = _ttoi(csMonth);
    int iUserDay = _ttoi(csDay);

    if( csYear.GetLength() == 2 ) // two digit YY
    {
        if( iUserYear >= 70 )
            iUserYear += 1900;

        else
            iUserYear += 2000;
    }

    if( iUserYear >= 100 && iUserYear <= 9999 )
        iYear = iUserYear;

    if( iUserMonth >= 1 && iUserMonth <= 12 )
        iMonth = iUserMonth;

    if( iUserDay >= 1 )
        iDay = iUserDay;

    // make sure that the date is valid
    switch( iMonth )
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            iDay = std::min(31,iDay);
            break;

        case 4:
        case 6:
        case 9:
        case 11:
            iDay = std::min(30,iDay);
            break;

        case 2:
        {
            bool bIsLeapYear = ( ( iYear % 4 == 0 ) && ( iYear % 100 != 0 ) ) || ( iYear % 400 == 0 );
            iDay = std::min(bIsLeapYear ? 29 : 28,iDay);
            break;
        }
    }

    return COleDateTime(iYear,iMonth,iDay,0,0,0);
}


CString CCapiControl::TranslateDateToString() // GHM 20100615
{
    COleDateTime selectedDate;
    m_Calendar.GetCurSel(selectedDate);

    int iDate = (int)FormatDateToDouble(m_pParent->m_captureInfo.GetExtended<DateCaptureInfo>().GetFormat(),
        selectedDate.GetYear(),selectedDate.GetMonth(),selectedDate.GetDay());

    return FormatText(_T("%0*d"), m_pParent->m_pDictItem->GetLen(), iDate);
}

CSize CCapiControl::LayoutInGrid(const CArray< CArray<CWnd*>* > &columns, const CArray<UINT> &columnAlignments,
                                 int border, int colSpacingChars, int rowSpacingPx, const CArray<CWnd*> &rowBackgrounds,
                                 bool sizeOnly)
{
    CRect clientRect;
    GetClientRect(clientRect);
    int colSpacingPx = colSpacingChars * CharWidthInPixels();
    CArray<int> columnWidths;
    int numColumns = columns.GetSize();
    int numRows = columns.GetAt(0)->GetSize();
    columnWidths.SetSize(numColumns);
    int totalColumnWidth = 0;
    for (int c = 0; c < numColumns; ++c) {
        int maxWidth = 0;
        const CArray<CWnd*> &column = *(columns.GetAt(c));
        for (int r = 0; r < column.GetSize(); ++r) {
            CRect controlRect;
            CWnd *control = column.GetAt(r);
            if (control != NULL && ::IsWindow(control->GetSafeHwnd())) {
                control->GetClientRect(controlRect);
                maxWidth =std::max(maxWidth, controlRect.Width());
            }
        }
        totalColumnWidth += maxWidth;
        columnWidths.SetAt(c, maxWidth);
    }

    int totalWidth = 0;
    for (int c = 0; c < numColumns; ++c) {
        totalWidth += columnWidths.GetAt(c) + (c < numColumns - 1 ? colSpacingPx : 0);
    }

    int currentRowTop = border;
    int currentColumnLeft = border;
    for (int r = 0; r < numRows; ++r) {
        int maxHeight = 0;
        for (int c = 0; c < numColumns; ++c) {
            CRect controlRect;
            CWnd *control = columns.GetAt(c)->GetAt(r);
            if (control != NULL && ::IsWindow(control->GetSafeHwnd())) {
                columns.GetAt(c)->GetAt(r)->GetWindowRect(controlRect);
                maxHeight =std::max(maxHeight, controlRect.Height());
            }
        }

        if (!sizeOnly) {
            currentColumnLeft = border;
            for (int c = 0; c < numColumns; ++c) {
                CWnd *control = columns.GetAt(c)->GetAt(r);
                if (control != NULL && ::IsWindow(control->GetSafeHwnd())) {
                    CRect controlRect;
                    control->GetWindowRect(controlRect);
                    controlRect.OffsetRect(-controlRect.left, -controlRect.top);
                    int offset = 0;
                    switch (columnAlignments.GetAt(c)) {
                    case LVCFMT_LEFT:
                        offset = currentColumnLeft;
                        break;
                    case LVCFMT_RIGHT:
                        offset = currentColumnLeft + columnWidths.GetAt(c) - controlRect.Width();
                        break;
                    case LVCFMT_CENTER:
                        offset = currentColumnLeft + (columnWidths.GetAt(c) - controlRect.Width())/2;
                        break;
                    }
                    controlRect.OffsetRect(offset, currentRowTop + maxHeight/2 - controlRect.Height()/2);
                    control->SetWindowPos(NULL, controlRect.left,  controlRect.top,controlRect.Width(), controlRect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
                }
                currentColumnLeft += columnWidths.GetAt(c) + (c < numColumns - 1 ? colSpacingPx : 0);
            }

            // Update position of row background if it is there
            if (rowBackgrounds.GetCount() > r) {
                CRect rowRect(0, currentRowTop - rowSpacingPx/2, clientRect.Width(), currentRowTop + maxHeight  + rowSpacingPx/2);
                rowBackgrounds[r]->SetWindowPos(NULL, rowRect.left,  rowRect.top,rowRect.Width(), rowRect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }

        currentRowTop += maxHeight + (r < numRows - 1 ? rowSpacingPx: 0);
    }

    return CSize(totalWidth + border, currentRowTop + border);
}


void CCapiControl::CreateStaticControlWithImage(CStatic& static_control, UINT controlId, const CString& imagePath)
{
    if( imagePath.IsEmpty() )
        return;

    std::shared_ptr<CImage> image = ImageManager::GetImage(imagePath);

    if( image == nullptr )
        return;

    // Scale image to appropriate size
    constexpr int MaxImageDimension = 300;
    int resized_image_size = std::min(std::max(image->GetWidth(), image->GetHeight()), MaxImageDimension);
    std::unique_ptr<CImage> resized_image = ImageManager::GetResizedImageToSize(image, resized_image_size, m_backgroundBrush);

    // Create a static control and put scaled image in it
    CRect controlRect(0, 0, resized_image->GetWidth(), resized_image->GetHeight());
    static_control.Create(NULL,WS_CHILD|WS_BORDER|WS_VISIBLE|SS_BITMAP|SS_CENTERIMAGE|SS_NOTIFY, controlRect, this, controlId);
    static_control.SetBitmap(resized_image->Detach());
}


CSize CCapiControl::Filter(const wstring_view filter_sv)
{
    // Reset the scroll position
    BOOL horzBar,vertBar;
    CheckScrollBars(horzBar,vertBar);
    if(vertBar || horzBar)
        ScrollToPosition(CPoint(0,0));

    const std::vector<std::shared_ptr<const ValueSetResponse>>& responses = m_pParent->m_responseProcessor->GetResponses();

    CArray< CArray<CWnd*>* > controls;
    CArray<CWnd*> buttonsOrValues, labels, images, rowBackgrounds;

    // whitespace should match with everything
    const bool useFiltering = !SO::IsWhitespace(filter_sv);

    for( int i = 0; i < static_cast<int>(responses.size()); i++ )
    {
        const bool filteredOut = ( useFiltering && 
                                   SO::FindNoCase(responses[i]->GetLabel(), filter_sv) == wstring_view::npos );
        const int showOrHide = filteredOut ? SW_HIDE : SW_SHOW;
        m_labels[i].ShowWindow(showOrHide);

        if (m_rowBackgrounds.GetCount() > 0)
            m_rowBackgrounds[i].ShowWindow(showOrHide);
        if (m_values.GetCount() > 0)
            m_values[i].ShowWindow(showOrHide);
        if (m_buttons.GetCount() > 0)
            m_buttons[i].ShowWindow(showOrHide);
        if (m_images.GetCount() > 0 && ::IsWindow(m_images.GetAt(i).GetSafeHwnd()))
            m_images[i].ShowWindow(showOrHide);

        if (!filteredOut) {
            labels.Add(&m_labels.GetAt(i));
            if (m_buttons.GetCount() > 0)
                buttonsOrValues.Add(&m_buttons.GetAt(i));
            if (m_values.GetCount() > 0)
                buttonsOrValues.Add(&m_values.GetAt(i));
            if (m_images.GetCount() > 0)
                images.Add(&m_images.GetAt(i));
            if (m_rowBackgrounds.GetCount() > 0)
                rowBackgrounds.Add(&m_rowBackgrounds.GetAt(i));
        }
    }
    controls.Add(&buttonsOrValues);
    controls.Add(&labels);
    controls.Add(&images);
    CSize border(EXTENDED_CONTROL_BORDER_SIZE, EXTENDED_CONTROL_BORDER_SIZE);
    CArray<UINT> alignments;
    alignments.Add(LVCFMT_LEFT);
    alignments.Add(LVCFMT_LEFT);
    alignments.Add(LVCFMT_CENTER);
    CSize filteredSize = LayoutInGrid(controls, alignments, EXTENDED_CONTROL_BORDER_SIZE, EXTENDED_CONTROL_COL_SPACING, EXTENDED_CONTROL_ROW_SPACING, rowBackgrounds, true);

    CRect clientRect;
    GetParent()->GetClientRect(clientRect);

    SetScrollSizes(MM_TEXT, filteredSize);
    return LayoutInGrid(controls, alignments, EXTENDED_CONTROL_BORDER_SIZE, EXTENDED_CONTROL_COL_SPACING, EXTENDED_CONTROL_ROW_SPACING, rowBackgrounds, false);
}

int CCapiControl::CharWidthInPixels()
{
    CClientDC dc(this);
    dc.SelectObject(m_pParent->GetControlFont());
    return dc.GetTextExtent(_T("A")).cx;
}
