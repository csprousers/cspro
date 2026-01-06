#pragma once

#include <zUtilF/LoggingListBox.h>


class OutputWnd : public CDockablePane
{
public:
    void Clear() { m_loggingListBox.Clear(); }

    template<typename... Args>
    void AddText(Args&&... args) { m_loggingListBox.AddText(std::forward<Args>(args)...); }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnSize(UINT nType, int cx, int cy);

private:
    LoggingListBox m_loggingListBox;
};
