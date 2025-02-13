#pragma once

#include <zEngineO/Userbar.h>

class WindowsUserbar;


struct WindowsUserbarItem
{
    enum class Type { Button, Field, Text, Spacing };

    Type type;
    int id;
    std::optional<Userbar::Action> action;
    CRect rect;
    std::unique_ptr<CWnd> window;
    std::optional<COLORREF> color;

    WindowsUserbarItem(WindowsUserbarItem&&) = default;
    WindowsUserbarItem& operator=(WindowsUserbarItem&&) = default;

    ~WindowsUserbarItem()
    {
        if( window != nullptr )
            window->DestroyWindow();
    }
};


class WindowsUserbarWnd : public CWnd
{
public:
    WindowsUserbarWnd(WindowsUserbar& userbar)
        :   m_userbar(userbar)
    {
    }

protected:
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    WindowsUserbar& m_userbar;
};



class WindowsUserbar : public Userbar
{
    friend class WindowsUserbarWnd;

public:
    WindowsUserbar();
    ~WindowsUserbar();

    void Show() override;
    void Hide() override;
    void Clear() override;

    void Pause(bool pause) override;

    int AddButton(std::wstring text, std::optional<Action> action) override;
    int AddField(std::wstring text, std::optional<Action> action) override;
    int AddText(std::wstring text) override;
    int AddSpacing(int spacing) override;

    std::optional<std::wstring> GetFieldText(int id) override;
    std::optional<int> GetLastActivatedItem() const override { return m_lastActivatedItemId; }

    bool SetColor(COLORREF color, std::optional<int> id) override;

    bool Modify(int id, std::optional<std::wstring> text, std::optional<Action> action, std::optional<int> spacing) override;

    bool Remove(int id) override;

private:
    CFont* GetFont();

    template<typename T>
    std::optional<size_t> GetItemIndex(T id_or_hwnd) const;

    template<typename T>
    WindowsUserbarItem* GetItem(T id_or_hwnd);

    void MoveItems(size_t starting_index, int shift_amount);

    template<typename CF>
    int AddItem(WindowsUserbarItem::Type item_type, std::optional<Action> action, CF callback_function);

private:
    WindowsUserbarWnd* m_wnd;
    std::unique_ptr<CReBar> m_rebar;
    std::unique_ptr<CBrush> m_backgroundColor;
    CFont m_font;
    long m_height;
    long m_buttonHeight;
    long m_fieldHeight;

    enum class Status { Shown, Hidden, Paused };
    Status m_status;

    std::vector<WindowsUserbarItem> m_items;
    std::optional<int> m_lastActivatedItemId;
};
