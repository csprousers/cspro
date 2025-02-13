#include "stdafx.h"
#include "HtmlEditorCtrl.h"
#include "InsertLinkDlg.h"
#include <zToolsO/WinClipboard.h>
#include <WebView2.h>
#include <wrl.h>


CREATE_ENUM_JSON_SERIALIZER(HtmlEditorCtrl::TextAlign,
    { HtmlEditorCtrl::TextAlign::Left,        L"left" },
    { HtmlEditorCtrl::TextAlign::Right,       L"right" },
    { HtmlEditorCtrl::TextAlign::Center,      L"center" },
    { HtmlEditorCtrl::TextAlign::Justify,     L"justify" },
    { HtmlEditorCtrl::TextAlign::Start,       L"start" },
    { HtmlEditorCtrl::TextAlign::End,         L"end" },
    { HtmlEditorCtrl::TextAlign::JustifyAll,  L"justify-all" },
    { HtmlEditorCtrl::TextAlign::MatchParent, L"match-parent" },
    { HtmlEditorCtrl::TextAlign::Center,      L"-webkit-center" })

CREATE_ENUM_JSON_SERIALIZER(HtmlEditorCtrl::ListStyle,
    { HtmlEditorCtrl::ListStyle::None,        L"none" },
    { HtmlEditorCtrl::ListStyle::Ordered,     L"ordered" },
    { HtmlEditorCtrl::ListStyle::Unordered,   L"unordered" })

namespace
{
    template<>
    struct JsonSerializer<HtmlEditorCtrl::Format>
    {
        static HtmlEditorCtrl::Format CreateFromJson(const JsonNode<wchar_t>& json_node)
        {
            HtmlEditorCtrl::Format val;
            val.font_size = json_node.GetOrDefault(L"font-size", 10);
            ASSERT(val.font_size > 0);
            val.font_family = json_node.GetOrDefault(L"font-family", std::wstring(L"Arial"));
            val.bold = json_node.GetOrDefault<bool>(L"font-bold", false);
            val.italic = json_node.GetOrDefault<bool>(L"font-italic", false);
            val.underline = json_node.GetOrDefault<bool>(L"font-underline", false);
            val.text_align = json_node.GetOrDefault<HtmlEditorCtrl::TextAlign>(L"text-align", HtmlEditorCtrl::TextAlign::Left);
            val.list_style = json_node.GetOrDefault<HtmlEditorCtrl::ListStyle>(L"list-style", HtmlEditorCtrl::ListStyle::None);
            val.class_name = json_node.GetOrDefault(L"class", SO::EmptyString);
            return val;
        }
    };

    template<>
    struct JsonSerializer<HtmlEditorCtrl::Style>
    {
        static void WriteJson(JsonWriter& json_writer, const HtmlEditorCtrl::Style& style)
        {
            json_writer
                .BeginObject()
                .Write(L"tag", style.tag)
                .Write(L"title", style.name)
                .Write(L"className", style.className)
                .Write(L"style", style.css)
                .EndObject();
        }
    };
}



IMPLEMENT_DYNCREATE(HtmlEditorCtrl, HtmlViewCtrl)

BEGIN_MESSAGE_MAP(HtmlEditorCtrl, HtmlViewCtrl)
    ON_WM_ENABLE()
END_MESSAGE_MAP()


HtmlEditorCtrl::HtmlEditorCtrl()
    :   m_dirty(false),
        m_selection_empty(true),
        m_code_view_showing(false),
        m_editor_ready(false)
{
    AddWebViewCreatedObserver([this]() { SetupFocusNotifications(); });
    AddWebEventObserver([this](const std::wstring& event_json) { OnWebMessageReceived(event_json); });
    SetAcceleratorKeyHandler([this](UINT message, UINT key, INT lParam) { return HandleAcceleratorKey(message, key, lParam); });
}


void HtmlEditorCtrl::SetUrl(std::wstring url)
{
    NavigateTo(std::move(url));
}


void HtmlEditorCtrl::SetText(wstring_view text)
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { L"action", L"setText" },
            { L"value",  text }
        }));

    m_text = text;
    m_dirty = false;
}


void HtmlEditorCtrl::Clear()
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { L"action",  L"clear" }
        }));

    m_text.clear();
    m_dirty = false;
}


void HtmlEditorCtrl::SetStyles(std::vector<Style> styles)
{
    SendCommand(L"customizableStyle.setStyles", styles);
    m_styles = std::move(styles);
}


void HtmlEditorCtrl::ApplyStyle(const Style& style)
{
    SendCommand(L"customizableStyle.applyStyle", style.name);
}


const HtmlEditorCtrl::Style* HtmlEditorCtrl::GetStyle() const
{
    auto style = std::find_if(m_styles.cbegin(), m_styles.cend(),
                              [&](const Style& s) { return s.className == m_current_format.class_name; });
    return style != m_styles.cend() ? &(*style) : nullptr;
}


bool HtmlEditorCtrl::CanCut() const
{
    return !m_selection_empty;
}


bool HtmlEditorCtrl::CanCopy() const
{
    return !m_selection_empty;
}


void HtmlEditorCtrl::Cut()
{
    SendCtrlKeyShortcut('X');
}


void HtmlEditorCtrl::Copy()
{
    SendCtrlKeyShortcut('C');
}


bool HtmlEditorCtrl::CanPaste() const
{
    return WinClipboard::HasHtml() || WinClipboard::HasText() || WinClipboard::HasImage();
}


void HtmlEditorCtrl::Paste(bool with_formatting)
{
    bool simulated_shift_key = !with_formatting;
    SendCtrlKeyShortcut('V', simulated_shift_key);
}


void HtmlEditorCtrl::Undo()
{
    SendCtrlKeyShortcut('Z');
}


void HtmlEditorCtrl::Redo()
{
    SendCtrlKeyShortcut('Y');
}


void HtmlEditorCtrl::SelectAll()
{
    SendCtrlKeyShortcut('A');
}


void HtmlEditorCtrl::Bold()
{
    SendCommand(L"bold");
}

bool HtmlEditorCtrl::IsBold() const
{
    return m_current_format.bold;
}


void HtmlEditorCtrl::Italic()
{
    SendCommand(L"italic");
}


void HtmlEditorCtrl::Underline()
{
    SendCommand(L"underline");
}


bool HtmlEditorCtrl::IsUnderline() const
{
    return m_current_format.underline;
}


HtmlEditorCtrl::TextAlign HtmlEditorCtrl::GetTextAlignment()
{
    return m_current_format.text_align;
}


void HtmlEditorCtrl::AlignLeft()
{
    SendCommand(L"justifyLeft");
}


void HtmlEditorCtrl::AlignRight()
{
    SendCommand(L"justifyRight");
}


void HtmlEditorCtrl::AlignCenter()
{
    SendCommand(L"justifyCenter");
}


void HtmlEditorCtrl::Justify()
{
    SendCommand(L"justifyFull");
}


void HtmlEditorCtrl::RightToLeft()
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { L"action",  L"rightToLeft" }
        }));
}


void HtmlEditorCtrl::LeftToRight()
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { L"action",  L"leftToRight" }
        }));
}


HtmlEditorCtrl::ListStyle HtmlEditorCtrl::GetListStyle() const
{
    return m_current_format.list_style;
}


void HtmlEditorCtrl::OrderedList()
{
    SendCommand(L"insertOrderedList");
}


void HtmlEditorCtrl::UnorderedList()
{
    SendCommand(L"insertUnorderedList");
}


const std::wstring& HtmlEditorCtrl::GetFontName() const
{
    return m_current_format.font_family;
}

void HtmlEditorCtrl::SetFont(wstring_view font_name)
{
    SendCommand(L"fontName", font_name);
}


int HtmlEditorCtrl::GetFontSize() const
{
    return m_current_format.font_size;
}


void HtmlEditorCtrl::SetFontSize(int size)
{
    SendCommand(L"fontSize", size);
}


void HtmlEditorCtrl::SetForeColor(COLORREF color)
{
    auto color_info = Json::CreateObject(
        {
            { L"foreColor", PortableColor::FromCOLORREF(color).ToStringRGB() }
        });

    SendCommand(L"editor.color", color_info);
}


void HtmlEditorCtrl::ToggleCodeView()
{
    SendCommand(L"codeview.toggle");
}


bool HtmlEditorCtrl::GetCodeViewShowing() const
{
    return m_code_view_showing;
}


bool HtmlEditorCtrl::IsItalic() const
{
    return m_current_format.italic;
}


void HtmlEditorCtrl::InsertImage(wstring_view image_path)
{
    SendCommand(L"insertImage", image_path);
}


void HtmlEditorCtrl::InsertTable(const CSize& size)
{
    SendCommand(L"insertTable", FormatText(L"%dx%d", size.cx, size.cy));
}


void HtmlEditorCtrl::ShowInsertLinkDialog()
{
    InsertLinkDlg link_dlg;

    if( link_dlg.DoModal() != IDOK )
        return;

    InsertLink(link_dlg.m_url, link_dlg.m_text);
}


void HtmlEditorCtrl::ShowEditLinkDlg(std::wstring url, std::wstring text, bool open_in_new_window)
{
    InsertLinkDlg link_dlg;

    // don't override the dialog's default URL
    if( !url.empty() )
        link_dlg.m_url = std::move(url);

    link_dlg.m_text = std::move(text);

    if( link_dlg.DoModal() == IDOK )
    {
        SendEditorMessage(Json::CreateObjectString(
            {
                { L"action",      L"editLinkDialogDismissed" },
                { L"cancelled",   false },
                { L"url",         link_dlg.m_url },
                { L"text",        link_dlg.m_text },
                { L"isNewWindow", open_in_new_window }
            }));
    }

    else
    {
        SendEditorMessage(Json::CreateObjectString(
            {
                { L"action",    L"editLinkDialogDismissed" },
                { L"cancelled", true }
            }));
    }
}


void HtmlEditorCtrl::InsertLink(wstring_view url, wstring_view text, bool open_in_new_window)
{
    auto link_info = Json::CreateObject(
        {
            { L"url",           url },
            { L"text",          text },
            { L"isNewWindow",   open_in_new_window },
            { L"checkProtocol", true }
        });

    SendCommand(L"editor.createLink", link_info);
}


void HtmlEditorCtrl::SetSyntaxErrors(const std::map<std::wstring, std::wstring>& logic_to_error)
{
    Json::ObjectCreator arg;

    for( const auto& [logic, error] : logic_to_error )
        arg.Set(logic, error);

    SendCommand(L"capifill.setSyntaxErrors", arg.GetJsonNode());
}


void HtmlEditorCtrl::OnEnable(BOOL bEnabled)
{
    HtmlViewCtrl::OnEnable(bEnabled);
    SendCommand(bEnabled ? L"enable" : L"disable");
}


void HtmlEditorCtrl::OnWebMessageReceived(wstring_view message)
{
    try {
        auto json = Json::Parse(message);
        auto action = json.Get<std::wstring_view>(L"action");

        if (action == L"documentLoaded") {
            m_editor_ready = true;
            for (const std::wstring& msg : m_messagesToSendWhenReady) {
                PostWebMessageAsJson(msg);
            }
        }
        else if (action == L"textChanged") {
            std::wstring value = json.GetOrDefault(L"value", SO::EmptyString);
            OnTextChanged(std::move(value));
        }
        else if (action == L"selectionChanged") {
            bool is_empty = json.Get<bool>(L"empty");
            auto current_style = json.Get<Format>(L"currentStyle");
            OnSelectionChanged(is_empty, std::move(current_style));
        }
        else if (action == L"contextMenu") {
            int clientX = json.Get<int>(L"clientX");
            int clientY = json.Get<int>(L"clientY");
            OnContextMenu(CPoint(clientX, clientY));
        }
        else if (action == L"codeViewToggled") {
            bool codeView = json.Get<bool>(L"codeView");
            OnCodeViewToggled(codeView);
        }
        else if (action == L"showEditLinkDialog") {
            auto linkInfo = json[L"linkInfo"];
            std::wstring text = linkInfo.Get<std::wstring>(L"text");
            std::wstring url = linkInfo.Get<std::wstring>(L"url");
            bool isNewWindow = linkInfo.GetOrDefault(L"isNewWindow", false);
            ShowEditLinkDlg(url, text, isNewWindow);
        }

    } catch( const JsonParseException& ) {
        ASSERT(false);
        // ignore errors
    }
}


void HtmlEditorCtrl::OnTextChanged(std::wstring text)
{
    m_dirty = true;
    m_text = std::move(text);
    GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), EN_CHANGE), (LPARAM)m_hWnd);
}


void HtmlEditorCtrl::OnSelectionChanged(bool is_empty, Format format)
{
    m_selection_empty = is_empty;
    m_current_format = std::move(format);
}


void HtmlEditorCtrl::OnContextMenu(CPoint location)
{
    ClientToScreen(&location);
    SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, MAKELPARAM(location.x, location.y));
}


void HtmlEditorCtrl::OnCodeViewToggled(bool code_view_showing)
{
    m_code_view_showing = code_view_showing;
}


void HtmlEditorCtrl::SendCtrlKeyShortcut(char key, bool shift)
{
    // Simulate hitting ctrl+key
    INPUT ip = { 0 };
    ip.type = INPUT_KEYBOARD;

    ip.ki.wVk = VK_CONTROL;
    SendInput(1, &ip, sizeof(INPUT));

    if (shift) {
        ip.ki.wVk = VK_SHIFT;
        SendInput(1, &ip, sizeof(INPUT));
    }

    ip.ki.wVk = key;
    SendInput(1, &ip, sizeof(INPUT));

    ip.ki.dwFlags = KEYEVENTF_KEYUP;

    ip.ki.wVk = key;
    SendInput(1, &ip, sizeof(INPUT));

    if (shift) {
        ip.ki.wVk = VK_SHIFT;
        SendInput(1, &ip, sizeof(INPUT));
    }

    ip.ki.wVk = VK_CONTROL;
    SendInput(1, &ip, sizeof(INPUT));
}


void HtmlEditorCtrl::SetupFocusNotifications()
{
    // Send focus notifications to parent to emulate standard edit controls.
    HRESULT hr = GetController()->add_LostFocus(Microsoft::WRL::Callback<ICoreWebView2FocusChangedEventHandler>(
        [this](ICoreWebView2Controller*, IUnknown*) -> HRESULT
        {
            GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), EN_KILLFOCUS), (LPARAM)m_hWnd);
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));

    hr = GetController()->add_GotFocus(Microsoft::WRL::Callback<ICoreWebView2FocusChangedEventHandler>(
        [this](ICoreWebView2Controller*, IUnknown*) -> HRESULT
        {
            GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), EN_SETFOCUS), (LPARAM)m_hWnd);
            return S_OK;
        }).Get(), nullptr);
    ASSERT(SUCCEEDED(hr));
}


bool HtmlEditorCtrl::HandleAcceleratorKey(UINT message, UINT key, INT lParam)
{
    if (ShouldHandleAccelerator(key, GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, GetKeyState(VK_MENU) < 0)) {
        return false;
    }
    else {
        PostMessage(message, key, lParam);
        return true;
    }
}


bool HtmlEditorCtrl::ShouldHandleAccelerator(UINT key, bool ctrl, bool shift, bool alt)
{
    switch (key) {
        // Shortcuts used by the editor
    case 'Z': // undo/redo
    case 'Y': // redo
    case 'A': // select all
    case 'B': // bold
    case 'U': // underline
    case 'X': // cut
    case 'C': // copy
        return ctrl && !shift && !alt; // ctrl only for these shortcuts 
    case 'V': // paste/paste without format
        return ctrl && !alt;
    case 'I': // italic or dev tools (ctrl+shift+I)
        return ctrl && !alt; // ctrl+shift allowed for these
    case VK_F1:
    case VK_F2:
    case VK_F3:
    case VK_F4:
    case VK_F5:
    case VK_F6:
    case VK_F7:
    case VK_F8:
    case VK_F9:
    case VK_F10:
    case VK_F11:
    case VK_F12:
        // Webview doesn't need function keys
        return false;
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_CONTROL:
    case VK_SHIFT:
    case VK_MENU:
    case VK_PRIOR: // pg up
    case VK_NEXT: // pg down
    case VK_END:
    case VK_HOME:
        return true;
    default:
        // Other shortcuts with ctrl or alt are not passed to webview and are instead posted
        // to this process for handling. 
        return !ctrl && !alt;
    }

}


void HtmlEditorCtrl::SendEditorMessage(const std::wstring& message_json)
{
    if (m_editor_ready) {
        PostWebMessageAsJson(message_json);
    }
    else {
        m_messagesToSendWhenReady.emplace_back(message_json);
    }
}


void HtmlEditorCtrl::SendCommand(wstring_view command)
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { L"action",  L"summernoteCommand" },
            { L"command", command }
        }));
}


template <typename T>
void HtmlEditorCtrl::SendCommand(wstring_view command, T arg)
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { L"action",  L"summernoteCommand" },
            { L"command", command },
            { L"arg",     arg }
        }));
}
