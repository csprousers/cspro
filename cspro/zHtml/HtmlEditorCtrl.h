#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/HtmlViewCtrl.h>


class ZHTML_API HtmlEditorCtrl : public HtmlViewCtrl
{
    DECLARE_DYNCREATE(HtmlEditorCtrl)

public:
    HtmlEditorCtrl();

    void SetUrl(std::wstring url);

    void SetText(wstring_view text);
    void Clear();

    const std::wstring& GetText() const { return m_text; }

    struct Style
    {
        std::wstring tag;
        std::wstring name;
        std::wstring className;
        std::wstring css;
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

    const std::wstring& GetFontName() const;
    void SetFont(wstring_view font_name);

    int GetFontSize() const;
    void SetFontSize(int size);

    void SetForeColor(COLORREF color);

    void ToggleCodeView();
    bool GetCodeViewShowing() const;

    void InsertImage(wstring_view image_path);

    void InsertTable(const CSize& size);

    void ShowInsertLinkDialog();
    void InsertLink(wstring_view url, wstring_view text, bool open_in_new_window = false);

    struct Format
    {
        std::wstring font_family = _T("Arial");
        int font_size = 12;
        bool bold = false;
        bool italic = false;
        bool underline = false;
        TextAlign text_align = TextAlign::Left;
        ListStyle list_style = ListStyle::None;
        std::wstring class_name = _T("normal");
    };

    void SetSyntaxErrors(const std::map<std::wstring, std::wstring>& logic_to_error);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnEnable(BOOL bEnabled);

private:
    void OnTextChanged(std::wstring text);
    void OnSelectionChanged(bool is_empty, Format format);
    void OnContextMenu(CPoint location);
    void OnCodeViewToggled(bool codeViewShowing);
    void SendCtrlKeyShortcut(char key, bool shift = false);
    void OnWebMessageReceived(wstring_view message);
    void SetupFocusNotifications();
    bool HandleAcceleratorKey(UINT message, UINT key, INT lParam);
    bool ShouldHandleAccelerator(UINT key, bool ctrl, bool shift, bool alt);
    void SendCommand(wstring_view command);
    template <typename T>
    void SendCommand(wstring_view command, T arg);
    void SendEditorMessage(const std::wstring& message_json);
    void ShowEditLinkDlg(std::wstring url, std::wstring text, bool open_in_new_window);

    bool m_dirty;
    std::wstring m_text;
    bool m_selection_empty;
    Format m_current_format;
    bool m_code_view_showing;
    std::vector<Style> m_styles;
    bool m_editor_ready;
    std::vector<std::wstring> m_messagesToSendWhenReady;
};
