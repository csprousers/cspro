#pragma once

#include <zUtilF/LoggingListBox.h>


class OutputWnd : public CDockablePane
{
public:
    void Clear() { m_loggingListBox.Clear(); }

    void AddText(std::wstring text)     { m_loggingListBox.AddText(std::move(text)); }
    void AddText(std::string_view text) { m_loggingListBox.AddText(UTF8Convert::UTF8ToWide(text)); }

protected:
	DECLARE_MESSAGE_MAP()

	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnSize(UINT nType, int cx, int cy);

private:
    LoggingListBox m_loggingListBox;
};
