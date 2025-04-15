#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/HtmlViewCtrl.h>


class ZHTML_API HtmlEditorCtrl : public HtmlViewCtrl
{
    DECLARE_DYNCREATE(HtmlEditorCtrl)

public:
    HtmlEditorCtrl();

    void SetUrl(std::string_view url_sv);

    const SharableString& GetText() const { return m_text; }
    void SetText(SharableString text);
    void Clear();

    struct Style
    {
        std::string tag;
        std::string name;
        std::string class_name;
        std::string css;
    };

    void SetStyles(std::vector<Style> styles);
    void ApplyStyle(const Style& style);
    const Style* GetStyle() const;

    bool IsDirty() const { return m_dirty; }

    bool CanCut() const;
    void Cut();
    bool CanCopy() const;
    void Copy();
    bool CanPaste() const;
    void Paste(bool with_formatting);
    void Undo();
    void Redo();
    void SelectAll();

    void Bold();
    bool IsBold() const;
    void Italic();
    bool IsItalic() const;
    void Underline();
    bool IsUnderline() const;

    enum class TextAlign { Left, Right, Center, Justify, Start, End, JustifyAll, MatchParent };
    TextAlign GetTextAlignment();
    void Align(TextAlign text_align);
    void AlignLeft();
    void AlignRight();
    void AlignCenter();
    void Justify();
    void RightToLeft();
    void LeftToRight();

    enum class ListStyle { None, Ordered, Unordered };
    ListStyle GetListStyle() const;
    void OrderedList();
    void UnorderedList();

    const std::string& GetFontName() const;
    void SetFont(std::string_view font_name_sv);

    int GetFontSize() const;
    void SetFontSize(int size);

    void SetForeColor(COLORREF color);

    void ToggleCodeView();
    bool GetCodeViewShowing() const;

    void InsertImage(std::string_view image_path_sv);

    void InsertTable(int rows, int columns);

    void InsertLink(std::string_view text_sv, std::string_view url_sv, bool open_in_new_window = false);

    struct Format
    {
        std::string font_family = "Arial";
        int font_size = 12;
        bool bold = false;
        bool italic = false;
        bool underline = false;
        TextAlign text_align = TextAlign::Left;
        ListStyle list_style = ListStyle::None;
        std::string class_name = "normal";
    };

    void SetSyntaxErrors(const std::map<std::string, std::string>& logic_to_error);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnEnable(BOOL bEnabled);

private:
    void OnTextChanged(std::string text);
    void OnSelectionChanged(bool is_empty, Format format);
    void OnContextMenu(CPoint location);
    void OnCodeViewToggled(bool codeViewShowing);
    void SendCtrlKeyShortcut(char key, bool shift = false);
    void OnWebMessageReceived(std::string_view message_sv);
    void SetupFocusNotifications();
    bool HandleAcceleratorKey(UINT message, UINT key, INT lParam);
    bool ShouldHandleAccelerator(UINT key, bool ctrl, bool shift, bool alt);
    void SendCommand(std::string_view command_sv);
    template<typename T>
    void SendCommand(std::string_view command_sv, T&& arg);
    void SendEditorMessage(std::string&& message_json);
    void ShowEditLinkDlg(std::string text, std::string url, bool open_in_new_window);

private:
    bool m_dirty;
    SharableString m_text;
    bool m_selection_empty;
    Format m_current_format;
    bool m_code_view_showing;
    std::vector<Style> m_styles;
    bool m_editor_ready;
    std::vector<std::string> m_messagesToSendWhenReady;
};
