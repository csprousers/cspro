#pragma once

#include <zToolsO/EnumHelpers.h>


namespace ViewOptionsHelper
{
    // Returns the command ID.
    UINT GetCommandId(const std::variant<UINT, CCmdUI*>& data);

    // Handles a menu option that results in calling a function.
    template<typename T>
    bool RouteCommand(const std::variant<UINT, CCmdUI*>& data, T& content_creator, bool (T::*command_processor)());

    // Handles a menu option that represents a boolean value.
    // The boolean value will be flipped when processing the command.
    template<bool update_page = true>
    bool HandleBooleanCheck(const std::variant<UINT, CCmdUI*>& data, bool& check);

    // Handles a menu option that represents a value that can be checked.
    // The value will be set to the value when processing the command.
    template<bool update_page = true, typename T>
    bool HandleCheck(const std::variant<UINT, CCmdUI*>& data, T& value, T this_value);

    // Handles an accelerator key that toggles between a group of options.
    template<bool update_page = true, typename T>
    bool HandleAccelerator(const std::variant<UINT, CCmdUI*>& data, T& value);

    // Handles a menu option unknown to the view.
    bool HandleUnknownCommand(const std::variant<UINT, CCmdUI*>& data);
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline UINT ViewOptionsHelper::GetCommandId(const std::variant<UINT, CCmdUI*>& data)
{
    return std::holds_alternative<CCmdUI*>(data) ? std::get<CCmdUI*>(data)->m_nID :
                                                   std::get<UINT>(data);
}


template<typename T>
bool ViewOptionsHelper::RouteCommand(const std::variant<UINT, CCmdUI*>& data, T& content_creator, bool (T::*command_processor)())
{
    ASSERT(command_processor != nullptr);

    if( std::holds_alternative<UINT>(data) )
    {
        return (content_creator.*command_processor)();
    }

    return true;
}


template<bool update_page/* = true*/>
bool ViewOptionsHelper::HandleBooleanCheck(const std::variant<UINT, CCmdUI*>& data, bool& check)
{
    if( std::holds_alternative<CCmdUI*>(data) )
    {
        std::get<CCmdUI*>(data)->SetCheck(check);
    }

    else
    {
        check = !check;
    }

    return update_page;
}


template<bool update_page/* = true*/, typename T>
bool ViewOptionsHelper::HandleCheck(const std::variant<UINT, CCmdUI*>& data, T& value, const T this_value)
{
    static_assert(sizeof(T) <= sizeof(int));

    if( std::holds_alternative<CCmdUI*>(data) )
    {
        std::get<CCmdUI*>(data)->SetCheck(( value == this_value ));
    }

    else
    {
        if( value == this_value )
            return false;

        value = this_value;
    }

    return update_page;
}


template<bool  update_page/* = true*/, typename T>
bool ViewOptionsHelper::HandleAccelerator(const std::variant<UINT, CCmdUI*>& data, T& value)
{
    static_assert(FirstInEnum<T>() != LastInEnum<T>());

    if( std::holds_alternative<UINT>(data) )
    {
        if( value == LastInEnum<T>() )
        {
            value = FirstInEnum<T>();
        }

        else
        {
            IncrementEnum(value);
        }
    }

    return update_page;
}


inline bool ViewOptionsHelper::HandleUnknownCommand(const std::variant<UINT, CCmdUI*>& data)
{
    if( std::holds_alternative<CCmdUI*>(data) )
        std::get<CCmdUI*>(data)->Enable(FALSE);

    return false;
}
