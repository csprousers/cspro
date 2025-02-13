#include "StdAfx.h"
#include "PortableUserbar.h"


PortableUserbar::PortableUserbar()
    :   m_shownState(false)
{
}


void PortableUserbar::Show()
{
    m_shownState = true;
    OnDisplayOrItemChange();
}


void PortableUserbar::Hide()
{
    m_shownState = false;
    OnDisplayOrItemChange();
}


void PortableUserbar::Clear()
{
    m_shownState = false;
    m_items.clear();
    OnDisplayOrItemChange();
}


void PortableUserbar::Pause(bool /*pause*/)
{
}


int PortableUserbar::AddButton(std::wstring text, std::optional<Action> action)
{
    if( action.has_value() && std::holds_alternative<Userbar::ControlAction>(*action) )
        throw FeatureNotImplemented();

    int id = m_items.empty() ? StartingId : ( m_items.back().id + 1 );

    // add the button
    m_items.emplace_back(PortableUserbarItem { id, std::move(text), std::move(action) });

    OnDisplayOrItemChange();

    return id;
}


int PortableUserbar::AddField(std::wstring /*text*/, std::optional<Action> /*action*/)
{
    throw FeatureNotImplemented();
}


int PortableUserbar::AddText(std::wstring /*text*/)
{
    throw FeatureNotImplemented();
}


int PortableUserbar::AddSpacing(int /*spacing*/)
{
    throw FeatureNotImplemented();
}


std::optional<std::wstring> PortableUserbar::GetFieldText(int /*id*/)
{
    return std::nullopt;
}


bool PortableUserbar::SetColor(COLORREF /*color*/, std::optional<int> /*id*/)
{
    return false;
}


bool PortableUserbar::Modify(int id, std::optional<std::wstring> text, std::optional<Action> action, std::optional<int> /*spacing*/)
{
    auto lookup = std::find_if(m_items.begin(), m_items.end(),
                               [&](const PortableUserbarItem& item) { return ( item.id == id ); });
    bool success = false;

    if( lookup != m_items.cend() )
    {
        if( text.has_value() )
        {
            lookup->text = std::move(*text);
            success = true;
        }

        if( action.has_value() )
        {
            lookup->action = std::move(*action);

            if( std::holds_alternative<std::unique_ptr<UserFunctionArgumentEvaluator>>(*lookup->action) )
                success = true;
        }
    }

    if( success )
        OnDisplayOrItemChange();

    return success;
}


bool PortableUserbar::Remove(int id)
{
    const auto& lookup = std::find_if(m_items.begin(), m_items.end(),
                                      [&](const PortableUserbarItem& item) { return ( item.id == id ); });

    if( lookup != m_items.cend() )
    {
        m_items.erase(lookup);
        OnDisplayOrItemChange();
        return true;
    }

    return false;
}
