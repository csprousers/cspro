#include "StdAfx.h"
#include "QSFEditStyleDlg.h"
#include <regex>


namespace {

    CString MakeCssClassName(const CString& s)
    {
        CString className = s;
        if (!is_alpha(className[0]))
            className.SetAt(0, L'Z');
        std::wregex invalid(L"[^a-zA-Z0-9_-]");
        return CString(std::regex_replace(className.GetString(), invalid, L"-").c_str());
    }
}

IMPLEMENT_DYNAMIC(QSFEditStyleDlg, CDialog)

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

QSFEditStyleDlg::QSFEditStyleDlg(std::vector<CapiStyle> styles, CWnd* pParent)
    : CDialog(QSFEditStyleDlg::IDD, pParent)
{
    m_styles = std::move(styles);
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
    if (!updated_style.m_name.IsEmpty())
        current_style.m_name = updated_style.m_name;
    current_style.m_css = updated_style.m_css;
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

void QSFEditStyleDlg::OnOK()
{
    // Make sure that names are unique
    std::set<CString> names;
    for (CapiStyle& style : m_styles) {
        if (names.find(style.m_name) != names.end()) {
            AfxMessageBox(FormatText(_T("There are multiple styles with name %s. Please rename them so that style names are unique."), style.m_name.GetString()), MB_ICONEXCLAMATION);
            return;
        }
        names.insert(style.m_name);
    }

    // Fill in the class names for any new styles
    std::set<CString> class_names;
    for (CapiStyle& style : m_styles) {
        if (style.m_class_name.IsEmpty()) {
            style.m_class_name = MakeCssClassName(style.m_name);
            // Make sure class name is unique
            int n = 2;
            while (class_names.find(style.m_class_name) != class_names.end()) {
                style.m_class_name = FormatText(L"%s%d", MakeCssClassName(style.m_name).GetString(), n++);
            }
        }
        class_names.insert(style.m_class_name);
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
        m_style_list.AddString(style.m_name);

    for (const CString& font_name : CapiStyle::DefaultFontNames) {
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
        m_font_size_combo.AddString(IntToString(font_size));

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
    m_edit_style_name.SetWindowText(style.m_name);

    std::optional<std::wstring> font_name = CssStyleParser::FontName(style.m_css);
    if (font_name) {
        if (m_font_name_combo.FindStringExact(-1, font_name->c_str()) == CB_ERR) {
            m_font_name_combo.AddString(font_name->c_str());
        }
        m_font_name_combo.SelectString(-1, font_name->c_str());
    }

    std::optional<int> font_size = CssStyleParser::FontSize(style.m_css);
    if (font_size) {
        CString font_size_string = IntToString(*font_size);
        if (m_font_size_combo.FindStringExact(-1, font_size_string) == CB_ERR) {
            m_font_size_combo.AddString(font_size_string);
        }
        m_font_size_combo.SelectString(-1, font_size_string);
    }

    bool bold = CssStyleParser::Bold(style.m_css);
    bool italic = CssStyleParser::Italic(style.m_css);
    if (bold && italic) {
        m_font_style_combo.SelectString(-1, L"Bold Italic");
    }
    else if (italic) {
        m_font_style_combo.SelectString(-1, L"Italic");
    }
    else if (bold) {
        m_font_style_combo.SelectString(-1, L"Bold");
    }
    else {
        m_font_style_combo.SelectString(-1, L"Regular");
    }

    bool underline = CssStyleParser::Underline(style.m_css);
    m_underline_button.SetCheck(underline ? BST_CHECKED : BST_UNCHECKED);

    std::optional<COLORREF> color = CssStyleParser::TextColor(style.m_css);
    if (color) {
        int color_index = m_color_combo.FindColor(*color);
        if (color_index != CB_ERR) {
            m_color_combo.SetCurSel(color_index);
        }
        else {
            m_color_combo.AddColor(L"", *color);
        }
    }
    else {
        m_color_combo.SetCurSel(0);
    }
}

CapiStyle QSFEditStyleDlg::StyleFromControls()
{
    CapiStyle style;

    m_edit_style_name.GetWindowText(style.m_name);

    CString font_name = GetSelectedFontName();
    if (!font_name.IsEmpty())
        style.m_css += FormatText(L"font-family: %s;", font_name.GetString());

    if (IsBoldSelected())
        style.m_css += L"font-weight: bold;";
    if (IsItalicSelected())
        style.m_css += L"font-style: italic;";

    std::optional<int> font_size = GetSelectedFontSize();
    if (font_size)
        style.m_css += FormatText(L"font-size: %dpx;", font_size);

    bool underline = m_underline_button.GetCheck() == BST_CHECKED;
    if (underline) {
        style.m_css += L"text-decoration: underline;";
    }

    std::optional<COLORREF> color = m_color_combo.GetSelColor();
    if (color) {
        style.m_css += FormatText(L"color: #%02x%02x%02x;",
                        GetRValue(*color), GetGValue(*color), GetBValue(*color));
    }
    return style;
}

void QSFEditStyleDlg::UpdateSample(const CapiStyle& style)
{
    CFont font;
    LOGFONT lf = CssStyleParser::ToLogfont(style.m_css);
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
    new_style.m_name = L"New Style";
    int n = 2;
    while (std::find_if(m_styles.begin(), m_styles.end(), [&new_style](const CapiStyle& s) { return s.m_name.CompareNoCase(new_style.m_name) == 0; }) != m_styles.end()) {
        new_style.m_name = FormatText(L"New Style %d", n++);
    }
    new_style.m_class_name = CString(); // gets filled in OnOk after user has edited name
    m_styles.emplace_back(new_style);
    m_style_list.AddString(new_style.m_name);
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
