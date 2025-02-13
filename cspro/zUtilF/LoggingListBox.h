#pragma once

#include <zUtilF/zUtilF.h>
#include <mutex>


// --------------------------------------------------------------------------
// LoggingListBox
//
// a CListBox subclass designed to show the results of logging activity;
// the Clear and AddText methods can be called from any thread
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILF LoggingListBox : public CListBox
{
public:
    LoggingListBox();
    virtual ~LoggingListBox() { }

    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) override;

    void Clear();

    void AddText(std::wstring text);

protected:
    virtual void AddAdditionalContextMenuItems(CMenu& popup_menu);

protected:
    DECLARE_MESSAGE_MAP()

    void PreSubclassWindow() override;

	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) override;
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;

    void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    void OnTimer(UINT_PTR nIDEvent);

    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    void OnContextMenu(CWnd* pWnd, CPoint pos);

    LRESULT OnLoggingListBoxUpdate(WPARAM wParam, LPARAM lParam);

    void OnCopySelectedLinesToClipboard();
    void OnUpdateCopySelectedLinesToClipboard(CCmdUI* pCmdUI);

    void OnClearLines();
    void OnUpdateClearLines(CCmdUI* pCmdUI);

    void OnSelectAllLines();
    void OnUpdateSelectAllLines(CCmdUI* pCmdUI);

    void OnSaveLines();
    void OnUpdateSaveLines(CCmdUI* pCmdUI);

private:
    LOGFONT m_logfont;
    CFont m_font;
    std::tuple<size_t, LONG> m_maxLineLengthAndHorizontalExtent;

    int m_scrollLinesDelta;
    int m_pendingMouseWheelActions;
    bool m_userScrolledManually;

    std::vector<std::unique_ptr<std::wstring>> m_lines;
    std::mutex m_linesMutex;
};
