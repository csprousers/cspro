#include "StdAfx.h"
#include "DynamicLayoutControlResizer.h"


DynamicLayoutControlResizer::DynamicLayoutControlResizer(CWnd& parent_wnd)
    :   m_parentWnd(parent_wnd)
{
    // calculate the initial size of the parent window
    ASSERT(parent_wnd.GetSafeHwnd() != nullptr);

    CRect parent_wnd_rect;
    parent_wnd.GetClientRect(parent_wnd_rect);

    m_initialClientSize = parent_wnd_rect.Size();
}


HWND DynamicLayoutControlResizer::GetHWnd(std::variant<HWND, CWnd*, int>& control) const
{
    if( std::holds_alternative<HWND>(control) )
        return std::get<HWND>(control);

    HWND hWnd;

    if( std::holds_alternative<CWnd*>(control) )
    {
        hWnd = std::get<CWnd*>(control)->GetSafeHwnd();
    }

    else
    {
        ASSERT(std::holds_alternative<int>(control));
        hWnd = GetDlgItem(m_parentWnd.m_hWnd, std::get<int>(control));
    }

    // when finally non-null, store the handle for future use
    if( hWnd != nullptr )
        control = hWnd;

    return hWnd;
}


void DynamicLayoutControlResizer::CreateSizingCalculations(ControlData& control_data) const
{
    ASSERT(std::holds_alternative<HWND>(control_data.control) && std::get<HWND>(control_data.control) != nullptr);
    ASSERT(std::holds_alternative<SizingDirection>(control_data.data));

    CRect client_rect;
    GetClientRect(std::get<HWND>(control_data.control), client_rect);

    // including in the margin calculation the window rectangle's difference in
    // width and/or height from the client rectangle ensures that the control is sized properly;
    // without this adjustment a control like CEdit would not size properly against a CComboBox
    // because of the difference in the controls' window/client rectangles
    CRect window_rect;
    GetWindowRect(std::get<HWND>(control_data.control), window_rect);
    m_parentWnd.ScreenToClient(window_rect);

    const SizingDirection sizing_direction = std::get<SizingDirection>(control_data.data);
    auto& data = control_data.data.emplace<std::array<SizingCalculations, 2>>();

    data[0].size_dimension = ( sizing_direction != SizingDirection::Y );
    data[0].margin_or_size = client_rect.Width();

    if( data[0].size_dimension )
        data[0].margin_or_size += m_initialClientSize.cx - client_rect.right - window_rect.Width();

    data[1].size_dimension = ( sizing_direction != SizingDirection::X );
    data[1].margin_or_size = client_rect.Height();

    if( data[1].size_dimension )
        data[1].margin_or_size += m_initialClientSize.cy - client_rect.bottom - window_rect.Height();
}


void DynamicLayoutControlResizer::CreateMovingCalculations(ControlData& control_data) const
{
    ASSERT(std::holds_alternative<HWND>(control_data.control) && std::get<HWND>(control_data.control) != nullptr);
    ASSERT(std::holds_alternative<MovingDirection>(control_data.data));

    CRect control_rect;
    GetWindowRect(std::get<HWND>(control_data.control), control_rect);
    m_parentWnd.ScreenToClient(control_rect);

    const MovingDirection moving_direction = std::get<MovingDirection>(control_data.data);
    auto& data = control_data.data.emplace<std::array<MovingCalculations, 2>>();

    data[0].min_pos = control_rect.left;

    if( moving_direction != MovingDirection::Y )
        data[0].margin = m_initialClientSize.cx - control_rect.left;

    data[1].min_pos = control_rect.top;

    if( moving_direction != MovingDirection::X )
        data[1].margin = m_initialClientSize.cy - control_rect.top;
}


void DynamicLayoutControlResizer::OnSize(const int cx, const int cy)
{
    for( ControlData& control_data : m_controls )
    {
        HWND hWnd = GetHWnd(control_data.control);

        // calculate the margins or sizes the first time this is called
        if( control_data.data.index() < 2 )
        {
            if( hWnd == nullptr )
            {
                continue;
            }

            else if( std::holds_alternative<SizingDirection>(control_data.data) )
            {
                CreateSizingCalculations(control_data);
            }

            else
            {
                ASSERT(std::holds_alternative<MovingDirection>(control_data.data));
                CreateMovingCalculations(control_data);
            }
        }

        ASSERT(hWnd != nullptr);

        if( std::holds_alternative<std::array<SizingCalculations, 2>>(control_data.data) )
        {
            const auto& data = std::get<std::array<SizingCalculations, 2>>(control_data.data);

            ::SetWindowPos(hWnd,
                           nullptr,
                           0,
                           0,
                           data[0].size_dimension ? ( cx - data[0].margin_or_size ) : data[0].margin_or_size,
                           data[1].size_dimension ? ( cy - data[1].margin_or_size ) : data[1].margin_or_size,
                           SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
        }

        else
        {
            ASSERT((std::holds_alternative<std::array<MovingCalculations, 2>>(control_data.data)));
            const auto& data = std::get<std::array<MovingCalculations, 2>>(control_data.data);

            ::SetWindowPos(hWnd,
                           nullptr,
                           data[0].margin.has_value() ? std::max(data[0].min_pos, cx - *data[0].margin) : data[0].min_pos,
                           data[1].margin.has_value() ? std::max(data[1].min_pos, cy - *data[1].margin) : data[1].min_pos,
                           0,
                           0,
                           SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }
}


void DynamicLayoutControlResizer::SetWindowPos(HWND hWnd, CRect& client_rect, const UINT nFlags/* = SWP_NOACTIVATE | SWP_NOZORDER*/)
{
    ASSERT(hWnd != nullptr);

    // OnSize should have been called and the calculations created prior to this method being called
    ASSERT(std::count_if(m_controls.cbegin(), m_controls.cend(),
        [](const ControlData& control_data)
        {
            return ( !std::holds_alternative<HWND>(control_data.control) ||
                     control_data.data.index() < 2 );
        }) == 0);

    // if this is a registered control, modify its calculations
    const auto& lookup = std::find_if(m_controls.begin(), m_controls.end(),
                                      [&](const ControlData& control_data) { return ( hWnd == std::get<HWND>(control_data.control) ); });

    if( lookup != m_controls.end() )
    {
        ControlData& control_data = *lookup;

        CRect current_client_rect;
        GetWindowRect(hWnd, &current_client_rect);

        if( std::holds_alternative<std::array<SizingCalculations, 2>>(control_data.data) )
        {
            auto& data = std::get<std::array<SizingCalculations, 2>>(control_data.data);

            if( data[0].size_dimension )
                data[0].margin_or_size += current_client_rect.Width() - client_rect.Width();

            if( data[1].size_dimension )
                data[1].margin_or_size += current_client_rect.Height() - client_rect.Height();
        }

        else
        {
            ASSERT((std::holds_alternative<std::array<MovingCalculations, 2>>(control_data.data)));
            auto& data = std::get<std::array<MovingCalculations, 2>>(control_data.data);

            m_parentWnd.ScreenToClient(current_client_rect);

            ASSERT(current_client_rect.Width() == client_rect.Width());
            ASSERT(current_client_rect.Height() == client_rect.Height());

            const int x_difference = client_rect.left - current_client_rect.left;
            data[0].min_pos += x_difference;

            if( data[1].margin.has_value() )
                *data[0].margin -= x_difference;

            const int y_difference = client_rect.top - current_client_rect.top;
            data[1].min_pos += y_difference;

            if( data[1].margin.has_value()  )
                *data[1].margin -= y_difference;
        }
    }

    ::SetWindowPos(hWnd, nullptr, client_rect.left, client_rect.top, client_rect.Width(), client_rect.Height(), nFlags);
}
