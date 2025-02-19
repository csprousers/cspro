#include "StdAfx.h"
#include "QSFEditStyleDlg.h"
#include <regex>


BEGIN_MESSAGE_MAP(QSFEditStyleDlg, CDialog)
    ON_LBN_SELCHANGE(IDC_LIST_STYLES, OnLbnSelchangeStyle)
    ON_CBN_SELCHANGE(IDC_COMBO_FONT, UpdateSelectedStyle)
    ON_CBN_EDITCHANGE(IDC_COMBO_FONT, UpdateSelectedStyle)
    ON_CBN_SELCHANGE(IDC_COMBO_FONT_STYLE, UpdateSelectedStyle)
    ON_CBN_EDITCHANGE(IDC_COMBO_FONT_STYLE, UpdateSelectedStyle)
    ON_CBN_SELCHANGE(IDC_COMBO_FONT_SIZE, UpdateSelectedStyle)
    ON_CBN_EDITCHANGE(IDC_COMBO_FONT_SIZE, UpdateSelectedStyle)
    ON_BN_CLICKED(IDC_UNDERLINE, UpdateSelectedStyle)
    ON_CBN_SELCHANGE(IDC_COMBO_COLOR, UpdateSelectedStyle)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_ADD_STYLE, OnAddStyle)
    ON_BN_CLICKED(IDC_DELETE_STYLE, OnDeleteStyle)
    ON_EN_KILLFOCUS(IDC_STYLE_NAME, OnStyleNameKillFocus)
END_MESSAGE_MAP()


QSFEditStyleDlg::QSFEditStyleDlg(std::vector<CapiStyle> styles, CWnd* const pParent)
    :   CDialog(IDD_QSFSTYLEDLG, pParent),
        m_styles(std::move(styles))
{
}


void QSFEditStyleDlg::OnLbnSelchangeStyle()
{
    CapiStyle& style = GetSelectedStyle();
    UpdateControlsToMatchToStyle(style);
    UpdateSample(style);
    m_delete_button.EnableWindow(m_style_list.GetCurSel() != 0);
}


void QSFEditStyleDlg::UpdateSelectedStyle()
{
    CapiStyle updated_style = StyleFromControls();
    CapiStyle& current_style = GetSelectedStyle();
    if (!updated_style.name.empty())
        current_style.name = updated_style.name;
    current_style.css = updated_style.css;
    UpdateSample(current_style);
}


void QSFEditStyleDlg::OnStyleNameKillFocus()
{
    CString new_name;
    m_edit_style_name.GetWindowText(new_name);
    if (new_name.Trim().IsEmpty()) {
        AfxMessageBox(L"Please enter a name for the style");
    }
    else {
        UpdateSelectedStyle();
        int selected_index = m_style_list.GetCurSel();
        m_style_list.DeleteString(selected_index);
        m_style_list.InsertString(selected_index, new_name);
        m_style_list.SetCurSel(selected_index);
    }
}


std::string QSFEditStyleDlg::MakeCssClassName(std::string class_name)
{
    ASSERT(!class_name.empty());

    if( !std::isalpha(class_name.front()) )
        class_name[0] = 'Z';

    std::regex invalid("[^a-zA-Z0-9_-]");
    return std::regex_replace(class_name, invalid, "-");
}


void QSFEditStyleDlg::OnOK()
{
    // Make sure that names are unique
    std::set<std::string> names;

    for( CapiStyle& style : m_styles )
    {
        if( names.find(style.name) != names.cend() )
        {
            ErrorMessage::Display(FormatText("There are multiple styles with name %s. "
                                             "Please rename them so that style names are unique.", style.name.c_str()));
            return;
        }

        names.insert(style.name);
    }

    // Fill in the class names for any new styles
    std::set<std::string> class_names;

    for( CapiStyle& style : m_styles )
    {
        if( style.class_name.empty() )
        {
            style.class_name = MakeCssClassName(style.name);

            // Make sure class name is unique
            int n = 2;

            while( class_names.find(style.class_name) != class_names.cend() )
            {
                style.class_name = FormatText("%s%d", MakeCssClassName(style.name).c_str(), n++);
            }
        }

        class_names.insert(style.class_name);
    }

    CDialog::OnOK();
}


void QSFEditStyleDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIST_STYLES, m_style_list);
    DDX_Control(pDX, IDC_COMBO_FONT, m_font_name_combo);
    DDX_Control(pDX, IDC_COMBO_FONT_STYLE, m_font_style_combo);
    DDX_Control(pDX, IDC_COMBO_FONT_SIZE, m_font_size_combo);
    DDX_Control(pDX, IDC_UNDERLINE, m_underline_button);
    DDX_Control(pDX, IDC_COMBO_COLOR, m_color_combo);
    DDX_Control(pDX, IDC_SAMPLE, m_sample);
    DDX_Control(pDX, IDC_STYLE_NAME, m_edit_style_name);
    DDX_Control(pDX, IDC_ADD_STYLE, m_add_button);
    DDX_Control(pDX, IDC_DELETE_STYLE, m_delete_button);
}


BOOL QSFEditStyleDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    for (const CapiStyle& style : m_styles)
        m_style_list.AddString(TC::ToWide(style.name).c_str());

    for (const wchar_t* const font_name : CapiStyle::DefaultFontNames) {
        LOGFONT logfont;
        memset(&logfont, 0, sizeof(LOGFONT));
        _tcscpy(logfont.lfFaceName, font_name);
        int index = m_font_name_combo.AddString(font_name);
        m_font_name_combo.SetStyle(index, logfont);
    }

    m_font_style_combo.AddString(_T("Regular"));
    m_font_style_combo.AddString(_T("Italic"));
    m_font_style_combo.AddString(_T("Bold"));
    m_font_style_combo.AddString(_T("Bold Italic"));

    for (int font_size : CapiStyle::DefaultFontSizes)
        m_font_size_combo.AddString(UTF8_TODO::GetCString(IntToString(font_size)));

    m_style_list.SetCurSel(0);
    UpdateControlsToMatchToStyle(m_styles[0]);
    UpdateSample(m_styles[0]);
    m_delete_button.EnableWindow(FALSE);

    m_add_button.SetImage(IDC_ADD_STYLE);
    m_add_button.SizeToContent();
    m_delete_button.SetImage(IDC_DELETE_STYLE, IDC_DELETE_STYLE, IDC_DELETE_STYLE_DISABLED);
    m_delete_button.SizeToContent();

    // Line up the delete button to be 8 px to right of add button
    CRect add_button_rect;
    m_add_button.GetWindowRect(add_button_rect);
    CRect delete_button_rect;
    m_delete_button.GetWindowRect(delete_button_rect);
    delete_button_rect.OffsetRect(add_button_rect.right - delete_button_rect.left + 8, 0);
    ScreenToClient(delete_button_rect);
    m_delete_button.MoveWindow(delete_button_rect);

    return 0;
}


CapiStyle& QSFEditStyleDlg::GetSelectedStyle()
{
    int selected = m_style_list.GetCurSel();
    return m_styles[selected];
}


void QSFEditStyleDlg::UpdateControlsToMatchToStyle(const CapiStyle& style)
{
    WindowsUtf8::SetText(m_edit_style_name, style.name);

    const std::optional<std::string> font_name = CssStyleParser::FontName(style.css);

    if( font_name.has_value() )
    {
        const std::wstring wide_font_name = TC::ToWide(*font_name);
        if(m_font_name_combo.FindStringExact(-1, wide_font_name.c_str()) == CB_ERR) {
            m_font_name_combo.AddString(wide_font_name.c_str());
        }
        m_font_name_combo.SelectString(-1, wide_font_name.c_str());
    }

    const std::optional<int> font_size = CssStyleParser::FontSize(style.css);

    if( font_size.has_value() )
    {
        CString font_size_string = UTF8_TODO::GetCString(IntToString(*font_size));
        if (m_font_size_combo.FindStringExact(-1, font_size_string) == CB_ERR) {
            m_font_size_combo.AddString(font_size_string);
        }
        m_font_size_combo.SelectString(-1, font_size_string);
    }

    const bool bold = CssStyleParser::Bold(style.css);
    const bool italic = CssStyleParser::Italic(style.css);

    if( bold && italic )
    {
        m_font_style_combo.SelectString(-1, L"Bold Italic");
    }

    else if( italic )
    {
        m_font_style_combo.SelectString(-1, L"Italic");
    }

    else if( bold )
    {
        m_font_style_combo.SelectString(-1, L"Bold");
    }

    else
    {
        m_font_style_combo.SelectString(-1, L"Regular");
    }

    const bool underline = CssStyleParser::Underline(style.css);
    m_underline_button.SetCheck(underline ? BST_CHECKED : BST_UNCHECKED);

    const std::optional<COLORREF> color = CssStyleParser::TextColor(style.css);

    if( color.has_value() )
    {
        const int color_index = m_color_combo.FindColor(*color);

        if( color_index != CB_ERR )
        {
            m_color_combo.SetCurSel(color_index);
        }

        else
        {
            m_color_combo.AddColor(L"", *color);
        }
    }

    else
    {
        m_color_combo.SetCurSel(0);
    }
}


CapiStyle QSFEditStyleDlg::StyleFromControls()
{
    CapiStyle style
    {
        WindowsUtf8::GetText(m_edit_style_name)
    };

    const std::string font_name = UTF8_TODO::GetUtf8(GetSelectedFontName());

    if( !font_name.empty() )
        style.css.append(FormatText("font-family: %s;", font_name.c_str()));

    if( IsBoldSelected() )
        style.css.append("font-weight: bold;");

    if( IsItalicSelected() )
        style.css.append("font-style: italic;");

    const std::optional<int> font_size = GetSelectedFontSize();

    if( font_size.has_value() )
        style.css.append(FormatText("font-size: %dpx;", *font_size));

    if(m_underline_button.GetCheck() == BST_CHECKED )
        style.css.append("text-decoration: underline;");

    const std::optional<COLORREF> color = m_color_combo.GetSelColor();

    if( color.has_value() )
    {
        style.css.append(FormatText("color: #%02x%02x%02x;", static_cast<unsigned int>(GetRValue(*color)),
                                                             static_cast<unsigned int>(GetGValue(*color)),
                                                             static_cast<unsigned int>(GetBValue(*color))));
    }

    return style;
}


void QSFEditStyleDlg::UpdateSample(const CapiStyle& style)
{
    LOGFONT lf = CssStyleParser::ToLogfont(style.css);
    CFont font;
    font.CreateFontIndirect(&lf);
    m_sample.SetFont(&font);
}


CString QSFEditStyleDlg::GetSelectedFontName() const
{
    CString font_name;
    if (m_font_name_combo.GetCurSel() != CB_ERR) {
        m_font_name_combo.GetLBText(m_font_name_combo.GetCurSel(), font_name);
    }
    else {
        m_font_name_combo.GetWindowText(font_name);
    }
    return font_name;
}


CString QSFEditStyleDlg::GetSelectedFontStyle() const
{
    CString font_style;
    if (m_font_style_combo.GetCurSel() != CB_ERR) {
        m_font_style_combo.GetLBText(m_font_style_combo.GetCurSel(), font_style);
    }
    else {
        m_font_style_combo.GetWindowText(font_style);
    }
    return font_style;
}


std::optional<int> QSFEditStyleDlg::GetSelectedFontSize() const
{
    CString font_size;
    if (m_font_size_combo.GetCurSel() != CB_ERR) {
        m_font_size_combo.GetLBText(m_font_size_combo.GetCurSel(), font_size);
    }
    else {
        m_font_size_combo.GetWindowText(font_size);
    }
    return font_size.IsEmpty() ? std::optional<int>() : std::stoi(std::wstring(font_size));
}


bool QSFEditStyleDlg::IsBoldSelected() const
{
    CString font_style = GetSelectedFontStyle();
    return font_style == L"Bold" || font_style == L"Bold Italic";
}


bool QSFEditStyleDlg::IsItalicSelected() const
{
    CString font_style = GetSelectedFontStyle();
    return font_style == L"Italic" || font_style == L"Bold Italic";
}


HBRUSH QSFEditStyleDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{

    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    if (nCtlColor == CTLCOLOR_STATIC && pWnd == &m_sample) {
        std::optional<COLORREF> color = m_color_combo.GetSelColor();
        if (color) {
            pDC->SetTextColor(*color);
        }
    }

    return hbr;
}


void QSFEditStyleDlg::OnAddStyle()
{
    CapiStyle new_style = GetSelectedStyle();
    new_style.name = "New Style";
    int n = 2;
    while (std::find_if(m_styles.begin(), m_styles.end(), [&new_style](const CapiStyle& s) { return SO::EqualsNoCase(s.name, new_style.name); }) != m_styles.end()) {
        new_style.name = FormatText("New Style %d", n++);
    }
    new_style.class_name.clear(); // gets filled in OnOk after user has edited name
    m_styles.emplace_back(new_style);
    m_style_list.AddString(TC::ToWide(new_style.name).c_str());
    m_style_list.SetCurSel(m_styles.size() - 1);
    OnLbnSelchangeStyle();
}


void QSFEditStyleDlg::OnDeleteStyle()
{
    int selected_index = m_style_list.GetCurSel();
    m_styles.erase(m_styles.begin() + selected_index);
    m_style_list.DeleteString(selected_index);
    m_style_list.SetCurSel(std::min(selected_index, m_style_list.GetCount() - 1));
    OnLbnSelchangeStyle();
    m_delete_button.EnableWindow(m_style_list.GetCurSel() != 0);
}
