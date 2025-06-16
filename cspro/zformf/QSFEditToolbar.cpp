#include "StdAfx.h"
#include "QSFEditToolbar.h"
#include "QSFEditToolBarComboBoxButtons.h"
#include "TableToolbarButton.h"
#include <zCapiO/CapiStyle.h>


BEGIN_MESSAGE_MAP(QSFEditToolbar, CMFCToolBar)
    ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
END_MESSAGE_MAP()


CSize QSFEditToolbar::GetBaseUnits(CFont* const pFont)
{
    if( pFont == nullptr )
        return ReturnProgrammingError(CSize(0, 0));

    ASSERT(pFont->GetSafeHandle() != nullptr);

    CDC dc_screen;
    dc_screen.Attach(::GetDC(NULL));

    CFont* const old_font = dc_screen.SelectObject(pFont);

    TEXTMETRIC tm;
    dc_screen.GetTextMetrics(&tm);

    dc_screen.SelectObject(old_font);

    return CSize(tm.tmAveCharWidth, tm.tmHeight);
}


void QSFEditToolbar::SetStyles(const std::vector<HtmlEditorCtrl::Style>& styles)
{
    QSFEditToolBarStyledComboBoxButton* const style_combo = DYNAMIC_DOWNCAST(QSFEditToolBarStyledComboBoxButton, GetButton(CommandToIndex(IDC_STYLE)));

    if( style_combo != nullptr )
    {
        const HtmlEditorCtrl::Style current_selection = m_styles[style_combo->GetCurSel() >= 0 ? style_combo->GetCurSel() : 0];
        m_styles = styles;

        style_combo->RemoveAllItems();

        for( const HtmlEditorCtrl::Style& style : m_styles )
        {
            const std::optional<COLORREF> color = CssStyleParser::TextColor(style.css);
            style_combo->AddItem(TC::ToWide(style.name).c_str(), CssStyleParser::ToLogfont(style.css), color.value_or(GetSysColor(COLOR_WINDOWTEXT)));
        }

        const auto& new_selection = std::find_if(m_styles.begin(), m_styles.end(),
                                                 [&](const HtmlEditorCtrl::Style& s) { return ( s.class_name == current_selection.class_name ); });

        const int selection_index = ( new_selection != m_styles.cend() ) ? std::distance(new_selection, m_styles.begin()) :
                                                                           0;
        style_combo->SelectItem(selection_index, false);
    }

    else
    {
        m_styles = styles;
    }
}


const HtmlEditorCtrl::Style& QSFEditToolbar::GetSelectedStyle() const
{
    CMFCToolBarComboBoxButton* const button = DYNAMIC_DOWNCAST(QSFEditToolBarStyledComboBoxButton, GetButton(CommandToIndex(IDC_STYLE)));
    return m_styles[button->GetCurSel()];
}


void QSFEditToolbar::SetSelectedStyle(const HtmlEditorCtrl::Style& style)
{
    CMFCToolBarComboBoxButton* const button = DYNAMIC_DOWNCAST(QSFEditToolBarStyledComboBoxButton, GetButton(CommandToIndex(IDC_STYLE)));
    const std::wstring wide_name = TC::ToWide(style.name);
    const int num_items = button->GetCount();

    for( int i = 0; i < num_items; ++i )
    {
        if( button->GetItem(i) == wide_name )
        {
            if( button->GetCurSel() != i )
                button->SelectItem(i, TRUE);

            return;
        }
    }
}


void QSFEditToolbar::SetButtonVisible(const UINT id, const BOOL visible)
{
    const int nIndex = CommandToIndex(id);
    CMFCToolBarButton* const pButton = GetButton(nIndex);
    pButton->SetVisible(visible);
    AdjustSizeImmediate();
}


// To hold the colours and their names
struct ColourTableEntry
{
    BYTE red;
    BYTE green;
    BYTE blue;
    wchar_t* szName;
};


constexpr ColourTableEntry crColours[] =
{
     { 0x00, 0x00, 0x00, L"Black" },
     { 0x42, 0x42, 0x42, L"Tundora" },
     { 0x63, 0x63, 0x63, L"Dove Gray" },
     { 0x9C, 0x9C, 0x94, L"Star Dust" },
     { 0xCE, 0xC6, 0xCE, L"Pale Slate" },
     { 0xEF, 0xEF, 0xEF, L"Gallery" },
     { 0xF7, 0xF7, 0xF7, L"Alabaster" },
     { 0xFF, 0xFF, 0xFF, L"White" },
     { 0xFF, 0x00, 0x00, L"Red" },
     { 0xFF, 0x9C, 0x00, L"Orange Peel" },
     { 0xFF, 0xFF, 0x00, L"Yellow" },
     { 0x00, 0xFF, 0x00, L"Green" },
     { 0x00, 0xFF, 0xFF, L"Cyan" },
     { 0x00, 0x00, 0xFF, L"Blue" },
     { 0x9C, 0x00, 0xFF, L"Electric Violet" },
     { 0xFF, 0x00, 0xFF, L"Magenta" },
     { 0xF7, 0xC6, 0xCE, L"Azalea" },
     { 0xFF, 0xE7, 0xCE, L"Karry" },
     { 0xFF, 0xEF, 0xC6, L"Egg White" },
     { 0xD6, 0xEF, 0xD6, L"Zanah" },
     { 0xCE, 0xDE, 0xE7, L"Botticelli" },
     { 0xCE, 0xE7, 0xF7, L"Tropical Blue" },
     { 0xD6, 0xD6, 0xE7, L"Mischka" },
     { 0xE7, 0xD6, 0xDE, L"Twilight" },
     { 0xE7, 0x9C, 0x9C, L"Tonys Pink" },
     { 0xFF, 0xC6, 0x9C, L"Peach Orange" },
     { 0xFF, 0xE7, 0x9C, L"Cream Brulee" },
     { 0xB5, 0xD6, 0xA5, L"Sprout" },
     { 0xA5, 0xC6, 0xCE, L"Casper" },
     { 0x9C, 0xC6, 0xEF, L"Perano" },
     { 0xB5, 0xA5, 0xD6, L"Cold Purple" },
     { 0xD6, 0xA5, 0xBD, L"Careys Pink" },
     { 0xE7, 0x63, 0x63, L"Mandy" },
     { 0xF7, 0xAD, 0x6B, L"Rajah" },
     { 0xFF, 0xD6, 0x63, L"Dandelion" },
     { 0x94, 0xBD, 0x7B, L"Olivine" },
     { 0x73, 0xA5, 0xAD, L"Gulf Stream" },
     { 0x6B, 0xAD, 0xDE, L"Viking" },
     { 0x8C, 0x7B, 0xC6, L"Blue Marguerite" },
     { 0xC6, 0x7B, 0xA5, L"Puce" },
     { 0xCE, 0x00, 0x00, L"Guardsman Red" },
     { 0xE7, 0x94, 0x39, L"Fire Bush" },
     { 0xEF, 0xC6, 0x31, L"Golden Dream" },
     { 0x6B, 0xA5, 0x4A, L"Chelsea Cucumber" },
     { 0x4A, 0x7B, 0x8C, L"Smalt Blue" },
     { 0x39, 0x84, 0xC6, L"Boston Blue" },
     { 0x63, 0x4A, 0xA5, L"Butterfly Bush" },
     { 0xA5, 0x4A, 0x7B, L"Cadillac" },
     { 0x9C, 0x00, 0x00, L"Sangria" },
     { 0xB5, 0x63, 0x08, L"Mai Tai" },
     { 0xBD, 0x94, 0x00, L"Buddha Gold" },
     { 0x39, 0x7B, 0x21, L"Forest Green" },
     { 0x10, 0x4A, 0x5A, L"Eden" },
     { 0x08, 0x52, 0x94, L"Venice Blue" },
     { 0x31, 0x18, 0x73, L"Meteorite" },
     { 0x73, 0x18, 0x42, L"Claret" },
     { 0x63, 0x00, 0x00, L"Rosewood" },
     { 0x7B, 0x39, 0x00, L"Cinnamon" },
     { 0x84, 0x63, 0x00, L"Olive" },
     { 0x29, 0x52, 0x18, L"Parsley" },
     { 0x08, 0x31, 0x39, L"Tiber" },
     { 0x00, 0x31, 0x63, L"Midnight Blue" },
     { 0x21, 0x10, 0x4A, L"Valentino" },
     { 0x4A, 0x10, 0x31, L"Loulou" },
};


CMFCColorMenuButton* QSFEditToolbar::CreateColorButton()
{
    const ColourTableEntry* const colors_end = crColours + _countof(crColours);

    if( m_palColorPicker.GetSafeHandle() == nullptr )
    {
        // Create the palette
        const size_t pal_buffer_size = sizeof(LOGPALETTE) - sizeof(PALETTEENTRY) + ( sizeof(PALETTEENTRY) * _countof(crColours) );
        auto pal_buffer = std::make_unique_for_overwrite<std::byte[]>(pal_buffer_size);

        LOGPALETTE* const pLogPalette = reinterpret_cast<LOGPALETTE*>(pal_buffer.get());
        pLogPalette->palVersion = 0x300;
        pLogPalette->palNumEntries = _countof(crColours);

        PALETTEENTRY* palPalEntry = pLogPalette->palPalEntry;

        for( const ColourTableEntry* colors_itr = crColours; colors_itr != colors_end ; ++colors_itr )
        {
            palPalEntry->peRed = colors_itr->red;
            palPalEntry->peGreen = colors_itr->green;
            palPalEntry->peBlue = colors_itr->blue;
            palPalEntry->peFlags = 0;
            ++palPalEntry;
        }

        m_palColorPicker.CreatePalette(pLogPalette);
    }

    CMFCColorMenuButton* const pColorButton = new CMFCColorMenuButton(ID_FORMAT_COLOR, L"Text Color...", &m_palColorPicker);

    pColorButton->EnableOtherButton(L"More Colors...");
    pColorButton->SetColumnsNumber(8);

    // Initialize color names:
    for( const ColourTableEntry* colors_itr = crColours; colors_itr != colors_end ; ++colors_itr )
        CMFCColorMenuButton::SetColorName(RGB(colors_itr->red, colors_itr->green, colors_itr->blue), colors_itr->szName);

    return pColorButton;
}


void QSFEditToolbar::SetFontFace(const std::wstring& font_name)
{
    QSFEditToolBarStyledComboBoxButton* button = DYNAMIC_DOWNCAST(QSFEditToolBarStyledComboBoxButton, GetButton(CommandToIndex(IDC_FONTFACE)));
    const int num_items = button->GetCount();

    for( int i = 0; i < num_items; ++i )
    {
        if( font_name == button->GetItem(i) )
        {
            if( button->GetCurSel() != i )
                button->SelectItem(i, TRUE);

            return;
        }
    }

    // not found - add to list
    LOGFONT logfont;
    memset(&logfont, 0, sizeof(LOGFONT));
    _tcscpy(logfont.lfFaceName, font_name.c_str());
    button->AddItem(font_name.c_str(), logfont);
    button->SelectItem(num_items, TRUE);
}


std::string QSFEditToolbar::GetFontFace() const
{
    CMFCToolBarComboBoxButton* const button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_FONTFACE)));
    return TC::ToUtf8(button->GetItem(button->GetCurSel()));
}


void QSFEditToolbar::SetFontSize(const int font_size)
{
    ASSERT(font_size > 0);
    const CString size_str = FormatText<CString>(L"%d", font_size);

    QSFEditNumericSortToolBarComboBoxButton* const button = DYNAMIC_DOWNCAST(QSFEditNumericSortToolBarComboBoxButton, GetButton(CommandToIndex(IDC_FONTSIZE)));
    const int num_items = button->GetCount();

    for( int i = 0; i < num_items; ++i )
    {
        if( button->GetItem(i) == size_str )
        {
            if( button->GetCurSel() != i )
                button->SelectItem(i, TRUE);

            return;
        }
    }

    // not found - add to list
    button->AddSortedItem(size_str);
    button->Invalidate(); // for some reason need to force redraw of combo box here or it doesn't update
}


int QSFEditToolbar::GetFontSize() const
{
    CMFCToolBarComboBoxButton* const button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_FONTSIZE)));
    return _ttoi(button->GetItem(button->GetCurSel()));
}


COLORREF QSFEditToolbar::GetForeColor() const
{
    CMFCColorMenuButton* const button = DYNAMIC_DOWNCAST(CMFCColorMenuButton, GetButton(CommandToIndex(ID_FORMAT_COLOR)));
    return button->GetColor();
}


CSize QSFEditToolbar::GetTableDimensions() const
{
    TableToolbarButton* const button = DYNAMIC_DOWNCAST(TableToolbarButton, GetButton(CommandToIndex(ID_INSERT_TABLE)));
    return button->GetDimensions();
}


void QSFEditToolbar::SetLanguages(const std::vector<Language>& languages)
{
    CMFCToolBarComboBoxButton* const button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_EDIT_LANG)));
    button->RemoveAllItems();

    for( const Language& language : languages )
        button->AddItem(TC::ToWide(language.GetLabel()).c_str());
}


void QSFEditToolbar::SetLanguage(const Language& language)
{
    CMFCToolBarComboBoxButton* const button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_EDIT_LANG)));
    const int num_items = button->GetCount();

    for( int i = 0; i < num_items; ++i )
    {
        if( SO::Equals(language.GetLabel(), button->GetItem(i)) )
        {
            if( button->GetCurSel() != i )
                button->SelectItem(i, TRUE);

            return;
        }
    }
}


std::string QSFEditToolbar::GetLanguageLabel() const
{
    CMFCToolBarComboBoxButton* const button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_EDIT_LANG)));
    return TC::ToUtf8(button->GetItem(button->GetCurSel()));
}


int QSFEditToolbar::GetImageIndex(UINT /*command*/)
{
    return GetButton(CommandToIndex(ID_INSERT_TABLE))->GetImage();
}


void QSFEditToolbar::OnReset()
{
    CFont fnt;
    fnt.Attach(GetStockObject(DEFAULT_GUI_FONT));
    CSize base_units = GetBaseUnits(&fnt);

    QSFEditToolBarStyledComboBoxButton style_combo(IDC_STYLE, GetImageIndex(IDC_STYLE), CBS_DROPDOWN);
    for (const HtmlEditorCtrl::Style& style : m_styles) {
        std::optional<COLORREF> color = CssStyleParser::TextColor(style.css);
        style_combo.AddItem(TC::ToWide(style.name).c_str(), CssStyleParser::ToLogfont(style.css), color.value_or(GetSysColor(COLOR_WINDOWTEXT)));
    }
    ReplaceButton(IDC_STYLE, style_combo);

    QSFEditToolBarStyledComboBoxButton font_name_combo(IDC_FONTFACE, GetImageIndex(IDC_FONTFACE),
        CBS_DROPDOWNLIST, (2 * LF_FACESIZE * base_units.cx) / 2);
    LOGFONT logfont;
    memset(&logfont, 0, sizeof(LOGFONT));
    for (const wchar_t* const font_name : CapiStyle::DefaultFontNames) {
        _tcscpy(logfont.lfFaceName, font_name);
        font_name_combo.AddItem(font_name, logfont);
    }
    ReplaceButton(IDC_FONTFACE, font_name_combo);

    QSFEditNumericSortToolBarComboBoxButton font_size_combo(IDC_FONTSIZE, GetImageIndex(IDC_FONTSIZE),
        CBS_DROPDOWNLIST, 10 * base_units.cx + 10);
    for (auto font_size : CapiStyle::DefaultFontSizes)
        font_size_combo.AddSortedItem(UTF8_TODO::GetCString(IntToString(font_size)));
    font_size_combo.SelectItem(0);
    ReplaceButton(IDC_FONTSIZE, font_size_combo);

    CMFCColorMenuButton* pColorButton = CreateColorButton();
    ReplaceButton(ID_FORMAT_COLOR, *pColorButton);
    delete pColorButton;

    QSFEditToolBarComboBoxButton language_combo(IDC_EDIT_LANG, GetImageIndex(IDC_EDIT_LANG));
    ReplaceButton(IDC_EDIT_LANG, language_combo);

    TableToolbarButton table_button(ID_INSERT_TABLE, GetImageIndex(ID_INSERT_TABLE), L"Table");
    ReplaceButton(ID_INSERT_TABLE, table_button);
}


LRESULT QSFEditToolbar::OnIdleUpdateCmdUI(WPARAM /*wParam*/, LPARAM lParam)
{
    return IsWindowVisible() ? __super::OnIdleUpdateCmdUI(0, lParam) :
                               0;
}
