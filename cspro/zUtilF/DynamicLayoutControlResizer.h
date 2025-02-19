#pragma once


// Some controls, such as the WebView2 and Scintilla control, do not seem to respond to Dynamic Layout settings.
// This class can be used to resize the controls as if Sizing X = 100 and/or Sizing Y = 100.
// (https://developercommunity.visualstudio.com/t/mfc-dynamic-layout-doesnt-work-on-activex-controls/619909)

enum class SizingDirection { X, Y, XY };


class DynamicLayoutControlResizer
{
public:
    template<typename CT>
    DynamicLayoutControlResizer(CSize initial_client_size, const CT& control_objects);

    template<typename CT>
    DynamicLayoutControlResizer(CWnd& parent_wnd, const CT& control_objects);

    // A user of this class should call the DynamicLayoutControlResizer constructor once during an OnSize call
    // and then call this method after calling the parent class' OnSize method.
    void OnSize(int cx, int cy);

private:
    static CSize GetClientWindowSize(CWnd& parent_wnd);

    static std::tuple<CWnd*, SizingDirection> ProcessControlObject(CWnd* control);
    static std::tuple<CWnd*, SizingDirection> ProcessControlObject(const std::tuple<CWnd*, SizingDirection>& control_and_sizing_direction);

private:
    struct SizingCalculations { bool size_dimension[2]; int margin_or_size[2]; };

    CSize m_initialClientSize;
    std::vector<std::tuple<CWnd*, std::variant<SizingDirection, SizingCalculations>>> m_controls;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename CT>
DynamicLayoutControlResizer::DynamicLayoutControlResizer(CSize initial_client_size, const CT& control_objects)
    :   m_initialClientSize(std::move(initial_client_size))
{
    for( const auto& control_object : control_objects )
        m_controls.emplace_back(ProcessControlObject(control_object));

    ASSERT(!m_controls.empty());
}


template<typename CT>
DynamicLayoutControlResizer::DynamicLayoutControlResizer(CWnd& parent_wnd, const CT& control_objects)
    :   DynamicLayoutControlResizer(GetClientWindowSize(parent_wnd), control_objects)
{
}


inline CSize DynamicLayoutControlResizer::GetClientWindowSize(CWnd& parent_wnd)
{
    // calculate the initial size of the parent window
    ASSERT(parent_wnd.GetSafeHwnd() != nullptr);

    CRect parent_wnd_rect;
    parent_wnd.GetClientRect(parent_wnd_rect);

    return parent_wnd_rect.Size();
}


inline std::tuple<CWnd*, SizingDirection> DynamicLayoutControlResizer::ProcessControlObject(CWnd* const control)
{
    return std::make_tuple(control, SizingDirection::XY);
}


inline std::tuple<CWnd*, SizingDirection> DynamicLayoutControlResizer::ProcessControlObject(const std::tuple<CWnd*, SizingDirection>& control_and_sizing_direction)
{
    return std::make_tuple(std::get<0>(control_and_sizing_direction), std::get<1>(control_and_sizing_direction));
}


inline void DynamicLayoutControlResizer::OnSize(const int cx, const int cy)
{
    for( auto& [control, sizing_direction_or_calculations] : m_controls )
    {
        // calculate the margins or sizes the first time this is called
        if( std::holds_alternative<SizingDirection>(sizing_direction_or_calculations) )
        {
            if( control->GetSafeHwnd() == nullptr )
                continue;

            CRect control_rect;
            control->GetClientRect(control_rect);

            const bool size_x = ( std::get<SizingDirection>(sizing_direction_or_calculations) != SizingDirection::Y );
            const bool size_y = ( std::get<SizingDirection>(sizing_direction_or_calculations) != SizingDirection::X );

            sizing_direction_or_calculations =
                SizingCalculations
                {
                    {
                        size_x,
                        size_y
                    },
                    {
                        size_x ? ( m_initialClientSize.cx - control_rect.right ) : ( control_rect.Width() ),
                        size_y ? ( m_initialClientSize.cy - control_rect.bottom ) : ( control_rect.Height() )
                    }
                };
        }

        const SizingCalculations& sc = std::get<SizingCalculations>(sizing_direction_or_calculations);

        control->SetWindowPos(nullptr, 0, 0,
                              sc.size_dimension[0] ? ( cx - sc.margin_or_size[0] ) : sc.margin_or_size[0],
                              sc.size_dimension[1] ? ( cy - sc.margin_or_size[1] ) : sc.margin_or_size[1],
                              SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }
}
