#pragma once

#include <zEngineF/zEngineF.h>
#include <zEngineO/Userbar.h>


struct PortableUserbarItem
{
    PortableUserbarItem(PortableUserbarItem&&) = default;
    PortableUserbarItem& operator=(PortableUserbarItem&&) = default;

    int id;
    std::wstring text;
    std::optional<Userbar::Action> action;
};


class CLASS_DECL_ZENGINEF PortableUserbar : public Userbar
{
public:
    PortableUserbar();
    PortableUserbar(const PortableUserbar&) = delete;
    PortableUserbar& operator=(const PortableUserbar&) = delete;

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

protected:
    virtual void OnDisplayOrItemChange() { }

protected:
    bool m_shownState;
    std::vector<PortableUserbarItem> m_items;
    std::optional<int> m_lastActivatedItemId;
};
