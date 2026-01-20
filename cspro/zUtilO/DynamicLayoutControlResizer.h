#pragma once

#include <zUtilO/zUtilO.h>
#include <array>


// Some controls, such as the WebView2 and Scintilla controls, do not seem to respond to Dynamic Layout settings.
// This class can be used to resize the controls as if Sizing X = 100 and/or Sizing Y = 100.
// This class can also handle moving controls as if Moving X = 100 and/or Moving Y = 100.
// (https://developercommunity.visualstudio.com/t/mfc-dynamic-layout-doesnt-work-on-activex-controls/619909)

enum class SizingDirection { X, Y, XY };
enum class MovingDirection { X, Y, XY };


class CLASS_DECL_ZUTILO DynamicLayoutControlResizer
{
public:
    DynamicLayoutControlResizer(CWnd& parent_wnd, const CSize* initial_client_size = nullptr);

    template<typename CT>
    DynamicLayoutControlResizer(CWnd& parent_wnd, const CT& control_objects, const CSize* initial_client_size = nullptr);

    // Add a control to be sized or moved.
    DynamicLayoutControlResizer& Add(std::variant<HWND, CWnd*, int> control, SizingDirection sizing_direction);
    DynamicLayoutControlResizer& Add(std::variant<HWND, CWnd*, int> control, MovingDirection moving_direction);

    // A user of this class should call the DynamicLayoutControlResizer constructor once during an OnSize call,
    // call Add as desired, and then call this method after calling the parent class' OnSize method.
    void OnSize(int cx, int cy);
    void OnSize(CWnd& wnd);

    // This method calls SetWindowPos on the window. If the window has been registered as a control
    // for dynamic sizing/moving, its calculations will be adjusted based on the new window size.
    void SetWindowPos(HWND hWnd, CRect& client_rect, UINT nFlags = SWP_NOACTIVATE | SWP_NOZORDER);

private:
    struct SizingCalculations
    {
        bool size_dimension;
        int margin_or_size;
    };

    struct MovingCalculations
    {
        int min_pos;
        std::optional<int> margin;
    };

    struct ControlData
    {
        std::variant<HWND, CWnd*, int> control;
        std::variant<SizingDirection, MovingDirection, std::array<SizingCalculations, 2>, std::array<MovingCalculations, 2>> data;
    };

private:
    static ControlData ProcessControlObject(HWND control);
    static ControlData ProcessControlObject(CWnd* control);
    static ControlData ProcessControlObject(const std::tuple<HWND, SizingDirection>& control_and_sizing_direction);
    static ControlData ProcessControlObject(const std::tuple<CWnd*, SizingDirection>& control_and_sizing_direction);

    HWND GetHWnd(std::variant<HWND, CWnd*, int>& control) const;

    void CreateSizingCalculations(ControlData& control_data) const;
    void CreateMovingCalculations(ControlData& control_data) const;

private:
    CWnd& m_parentWnd;
    CSize m_initialClientSize;
    std::vector<ControlData> m_controls;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename CT>
DynamicLayoutControlResizer::DynamicLayoutControlResizer(CWnd& parent_wnd, const CT& control_objects, const CSize* const initial_client_size/* = nullptr*/)
    :   DynamicLayoutControlResizer(parent_wnd, initial_client_size)
{
    for( const auto& control_object : control_objects )
        m_controls.emplace_back(ProcessControlObject(control_object));

    ASSERT(!m_controls.empty());
}


inline DynamicLayoutControlResizer& DynamicLayoutControlResizer::Add(std::variant<HWND, CWnd*, int> control, const SizingDirection sizing_direction)
{
    m_controls.emplace_back(ControlData { std::move(control), sizing_direction });
    return *this;
}


inline DynamicLayoutControlResizer& DynamicLayoutControlResizer::Add(std::variant<HWND, CWnd*, int> control, const MovingDirection moving_direction)
{
    m_controls.emplace_back(ControlData { std::move(control), moving_direction });
    return *this;
}


inline DynamicLayoutControlResizer::ControlData DynamicLayoutControlResizer::ProcessControlObject(HWND hWnd)
{
    return ControlData { hWnd, SizingDirection::XY };
}


inline DynamicLayoutControlResizer::ControlData DynamicLayoutControlResizer::ProcessControlObject(CWnd* const control)
{
    return ControlData { control, SizingDirection::XY };
}


inline DynamicLayoutControlResizer::ControlData DynamicLayoutControlResizer::ProcessControlObject(const std::tuple<HWND, SizingDirection>& control_and_sizing_direction)
{
    return ControlData { std::get<0>(control_and_sizing_direction), std::get<1>(control_and_sizing_direction) };
}


inline DynamicLayoutControlResizer::ControlData DynamicLayoutControlResizer::ProcessControlObject(const std::tuple<CWnd*, SizingDirection>& control_and_sizing_direction)
{
    return ControlData { std::get<0>(control_and_sizing_direction), std::get<1>(control_and_sizing_direction) };
}
