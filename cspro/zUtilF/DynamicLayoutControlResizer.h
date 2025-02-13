#pragma once


// some controls, such as the WebView2 control, do not seem to respond to Dynamic Layout settings
// so this class can be used to resize the controls as if Sizing X = 100 and Sizing Y = 100
// (https://developercommunity.visualstudio.com/t/mfc-dynamic-layout-doesnt-work-on-activex-controls/619909)

class DynamicLayoutControlResizer
{
public:
    DynamicLayoutControlResizer(CWnd& parent_wnd, std::initializer_list<CWnd*> controls)
    {
        for( CWnd* ctrl : controls )
            m_controlsAndMargins.try_emplace(ctrl, CSize());

        ASSERT(!m_controlsAndMargins.empty() && m_controlsAndMargins.cbegin()->second.cx == 0);

        // calculate the initial size of the parent window
        ASSERT(parent_wnd.GetSafeHwnd() != nullptr);

        CRect parent_wnd_rect;
        parent_wnd.GetClientRect(parent_wnd_rect);

        m_parentWndSize = parent_wnd_rect.Size();
    }


    // a user of this class should call the DynamicLayoutControlResizer constructor once during an OnSize call
    // and then call this method after calling the parent class' OnSize method
    void OnSize(int cx, int cy)
    {
        for( auto& [ctrl, margin] : m_controlsAndMargins )
        {
            // calculate the margins the first time this is called
            if( margin.cx == 0 )
            {
                if( static_cast<HWND>(*ctrl) == nullptr )
                    continue;

                CRect ctrl_rect;
                ctrl->GetClientRect(ctrl_rect);

                margin = CSize(m_parentWndSize.cx - ctrl_rect.right, m_parentWndSize.cy - ctrl_rect.bottom);
                ASSERT(margin.cx != 0);
            }

            ctrl->SetWindowPos(nullptr, 0, 0,
                               cx - margin.cx, cy - margin.cy,
                               SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }

private:
    CSize m_parentWndSize;
    std::map<CWnd*, CSize> m_controlsAndMargins;
};
