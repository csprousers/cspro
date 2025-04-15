#include "stdafx.h"
#include "HtmlEditorCtrl.h"
#include "InsertLinkDlg.h"
#include <zToolsO/WinClipboard.h>
#include <WebView2.h>
#include <wrl.h>


CREATE_ENUM_JSON_SERIALIZER(HtmlEditorCtrl::TextAlign,
    { HtmlEditorCtrl::TextAlign::Left,        "left" },
    { HtmlEditorCtrl::TextAlign::Right,       "right" },
    { HtmlEditorCtrl::TextAlign::Center,      "center" },
    { HtmlEditorCtrl::TextAlign::Justify,     "justify" },
    { HtmlEditorCtrl::TextAlign::Start,       "start" },
    { HtmlEditorCtrl::TextAlign::End,         "end" },
    { HtmlEditorCtrl::TextAlign::JustifyAll,  "justify-all" },
    { HtmlEditorCtrl::TextAlign::MatchParent, "match-parent" },
    { HtmlEditorCtrl::TextAlign::Center,      "-webkit-center" })

CREATE_ENUM_JSON_SERIALIZER(HtmlEditorCtrl::ListStyle,
    { HtmlEditorCtrl::ListStyle::None,        "none" },
    { HtmlEditorCtrl::ListStyle::Ordered,     "ordered" },
    { HtmlEditorCtrl::ListStyle::Unordered,   "unordered" })

namespace
{
    template<>
    struct JsonSerializer<HtmlEditorCtrl::Format>
    {
        static HtmlEditorCtrl::Format CreateFromJson(const JsonNode& json_node)
        {
            HtmlEditorCtrl::Format val;
            val.font_size = json_node.GetOrDefault("font-size", 10);
            ASSERT(val.font_size > 0);
            val.font_family = json_node.GetOrDefault<std::string>("font-family", "Arial");
            val.bold = json_node.GetOrDefault<bool>("font-bold", false);
            val.italic = json_node.GetOrDefault<bool>("font-italic", false);
            val.underline = json_node.GetOrDefault<bool>("font-underline", false);
            val.text_align = json_node.GetOrDefault<HtmlEditorCtrl::TextAlign>("text-align", HtmlEditorCtrl::TextAlign::Left);
            val.list_style = json_node.GetOrDefault<HtmlEditorCtrl::ListStyle>("list-style", HtmlEditorCtrl::ListStyle::None);
            val.class_name = json_node.GetOrConstruct<std::string>("class");
            return val;
        }
    };

    template<>
    struct JsonSerializer<HtmlEditorCtrl::Style>
    {
        static void WriteJson(JsonWriter& json_writer, const HtmlEditorCtrl::Style& style)
        {
            json_writer.BeginObject()
                       .Write("tag", style.tag)
                       .Write("title", style.name)
                       .Write("className", style.class_name)
                       .Write("style", style.css)
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
    AddWebEventObserver([this](const std::wstring_view message_sv) { OnWebMessageReceived(TC::ToUtf8(message_sv)); });
    SetAcceleratorKeyHandler([this](UINT message, UINT key, INT lParam) { return HandleAcceleratorKey(message, key, lParam); });
}


void HtmlEditorCtrl::SetUrl(const std::string_view url_sv)
{
    NavigateTo(url_sv);
}


void HtmlEditorCtrl::SetText(SharableString text)
{
    if( *m_text != *text )
    {
        SendEditorMessage(Json::CreateObjectString(
            {
                { JK::action, "setText" },
                { JK::value,  std::string_view(*text) }
            }));

        m_text = std::move(text);
    }

    m_dirty = false;
}


void HtmlEditorCtrl::Clear()
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { JK::action, "clear" }
        }));

    m_text.Reset();
    m_dirty = false;
}


void HtmlEditorCtrl::SetStyles(std::vector<Style> styles)
{
    SendCommand("customizableStyle.setStyles", styles);
    m_styles = std::move(styles);
}


void HtmlEditorCtrl::ApplyStyle(const Style& style)
{
    SendCommand("customizableStyle.applyStyle", style.name);
}


const HtmlEditorCtrl::Style* HtmlEditorCtrl::GetStyle() const
{
    const auto& lookup = std::find_if(m_styles.cbegin(), m_styles.cend(),
                                      [&](const Style& s) { return ( s.class_name == m_current_format.class_name ); });

    return ( lookup != m_styles.cend() ) ? &(*lookup) :
                                           nullptr;
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
    return ( WinClipboard::HasHtml() || WinClipboard::HasText() || WinClipboard::HasImage() );
}


void HtmlEditorCtrl::Paste(const bool with_formatting)
{
    const bool simulated_shift_key = !with_formatting;
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
    SendCommand("bold");
}


bool HtmlEditorCtrl::IsBold() const
{
    return m_current_format.bold;
}


void HtmlEditorCtrl::Italic()
{
    SendCommand("italic");
}


void HtmlEditorCtrl::Underline()
{
    SendCommand("underline");
}


bool HtmlEditorCtrl::IsUnderline() const
{
    return m_current_format.underline;
}


HtmlEditorCtrl::TextAlign HtmlEditorCtrl::GetTextAlignment()
{
    return m_current_format.text_align;
}


void HtmlEditorCtrl::Align(const TextAlign text_align)
{
    switch( text_align )
    {
        case TextAlign::Left:
            AlignLeft();
            break;

        case TextAlign::Right:
            AlignRight();
            break;

        case TextAlign::Center:
            AlignCenter();
            break;

        default:
            ASSERT(false);
    }
}


void HtmlEditorCtrl::AlignLeft()
{
    SendCommand("justifyLeft");
}


void HtmlEditorCtrl::AlignRight()
{
    SendCommand("justifyRight");
}


void HtmlEditorCtrl::AlignCenter()
{
    SendCommand("justifyCenter");
}


void HtmlEditorCtrl::Justify()
{
    SendCommand("justifyFull");
}


void HtmlEditorCtrl::RightToLeft()
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { JK::action, "rightToLeft" }
        }));
}


void HtmlEditorCtrl::LeftToRight()
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { JK::action, "leftToRight" }
        }));
}


HtmlEditorCtrl::ListStyle HtmlEditorCtrl::GetListStyle() const
{
    return m_current_format.list_style;
}


void HtmlEditorCtrl::OrderedList()
{
    SendCommand("insertOrderedList");
}


void HtmlEditorCtrl::UnorderedList()
{
    SendCommand("insertUnorderedList");
}


const std::string& HtmlEditorCtrl::GetFontName() const
{
    return m_current_format.font_family;
}


void HtmlEditorCtrl::SetFont(const std::string_view font_name_sv)
{
    SendCommand("fontName", font_name_sv);
}


int HtmlEditorCtrl::GetFontSize() const
{
    return m_current_format.font_size;
}


void HtmlEditorCtrl::SetFontSize(const int size)
{
    SendCommand("fontSize", size);
}


void HtmlEditorCtrl::SetForeColor(const COLORREF color)
{
    const JsonNode color_info_json_node = Json::CreateObject(
        {
            { "foreColor", PortableColor::FromCOLORREF(color).ToStringRGB() }
        });

    SendCommand("editor.color", color_info_json_node);
}


void HtmlEditorCtrl::ToggleCodeView()
{
    SendCommand("codeview.toggle");
}


bool HtmlEditorCtrl::GetCodeViewShowing() const
{
    return m_code_view_showing;
}


bool HtmlEditorCtrl::IsItalic() const
{
    return m_current_format.italic;
}


void HtmlEditorCtrl::InsertImage(const std::string_view image_path_sv)
{
    SendCommand("insertImage", image_path_sv);
}


void HtmlEditorCtrl::InsertTable(const int rows, const int columns)
{
    SendCommand("insertTable", FormatText("%dx%d", rows, columns));
}


void HtmlEditorCtrl::ShowEditLinkDlg(std::string text, std::string url, const bool open_in_new_window)
{
    InsertLinkDlg insert_link_dlg(std::move(text), std::move(url));

    if( insert_link_dlg.DoModal() == IDOK )
    {
        SendEditorMessage(Json::CreateObjectString(
            {
                { JK::action,    "editLinkDialogDismissed" },
                { "cancelled",   false },
                { JK::text,      insert_link_dlg.GetText() },
                { JK::url,       insert_link_dlg.GetUrl() },
                { "isNewWindow", open_in_new_window }
            }));
    }

    else
    {
        SendEditorMessage(Json::CreateObjectString(
            {
                { JK::action,  "editLinkDialogDismissed" },
                { "cancelled", true }
            }));
    }
}


void HtmlEditorCtrl::InsertLink(const std::string_view text_sv, const std::string_view url_sv, const bool open_in_new_window/* = false*/)
{
    const JsonNode link_info_json_node = Json::CreateObject(
        {
            { JK::text,        text_sv },
            { JK::url,         url_sv },
            { "isNewWindow",   open_in_new_window },
            { "checkProtocol", true }
        });

    SendCommand("editor.createLink", link_info_json_node);
}


void HtmlEditorCtrl::SetSyntaxErrors(const std::map<std::string, std::string>& logic_to_error)
{
    Json::ObjectCreator arg;

    for( const auto& [logic, error] : logic_to_error )
        arg.Set(logic, error);

    SendCommand("capifill.setSyntaxErrors", arg.GetJsonNode());
}


void HtmlEditorCtrl::OnEnable(const BOOL bEnabled)
{
    HtmlViewCtrl::OnEnable(bEnabled);

    SendCommand(bEnabled ? "enable" : "disable");
}


void HtmlEditorCtrl::OnWebMessageReceived(const std::string_view message_sv)
{
    try {
        const JsonNode json_node = Json::Parse(message_sv);
        const std::string_view action_sv = json_node.Get<std::string_view>(JK::action);

        if (action_sv == "documentLoaded") {
            m_editor_ready = true;
            for (const std::string& msg : m_messagesToSendWhenReady) {
                PostWebMessageAsJson(msg);
            }
        }
        else if (action_sv == "textChanged") {
            OnTextChanged(json_node.GetOrConstruct<std::string>("value"));
        }
        else if (action_sv == "selectionChanged") {
            const bool is_empty = json_node.Get<bool>("empty");
            Format current_style = json_node.Get<Format>("currentStyle");
            OnSelectionChanged(is_empty, std::move(current_style));
        }
        else if (action_sv == "contextMenu") {
            const int clientX = json_node.Get<int>("clientX");
            const int clientY = json_node.Get<int>("clientY");
            OnContextMenu(CPoint(clientX, clientY));
        }
        else if (action_sv == "codeViewToggled") {
            const bool codeView = json_node.Get<bool>("codeView");
            OnCodeViewToggled(codeView);
        }
        else if (action_sv == "showEditLinkDialog") {
            const JsonNode link_info_json_node = json_node["linkInfo"];
            ShowEditLinkDlg(link_info_json_node.Get<std::string>("text"),
                            link_info_json_node.Get<std::string>("url"),
                            link_info_json_node.GetOrDefault("isNewWindow", false));
        }

    } catch( const JsonParseException& ) {
        ASSERT(false);
        // ignore errors
    }
}


void HtmlEditorCtrl::OnTextChanged(std::string text)
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


void HtmlEditorCtrl::SendEditorMessage(std::string&& message_json)
{
    if( m_editor_ready )
    {
        PostWebMessageAsJson(message_json);
    }

    else
    {
        m_messagesToSendWhenReady.emplace_back(std::move(message_json));
    }
}


void HtmlEditorCtrl::SendCommand(const std::string_view command_sv)
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { JK::action,  "summernoteCommand" },
            { "command", command_sv }
        }));
}


template<typename T>
void HtmlEditorCtrl::SendCommand(const std::string_view command_sv, T&& arg)
{
    SendEditorMessage(Json::CreateObjectString(
        {
            { JK::action, "summernoteCommand" },
            { "command",  command_sv },
            { "arg",      std::forward<T>(arg) }
        }));
}
