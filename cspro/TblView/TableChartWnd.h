#pragma once

#include <zTableF/TabChWnd.h>
#include <zUtilF/SplitterWndWithDifferingViews.h>
#include <tblview/ChartManager.h>

class HtmlViewerView;
namespace ActionInvoker { class ListenerHolder; }


class TableChartWnd : public CTableChildWnd
{
    DECLARE_DYNCREATE(TableChartWnd)

public:
    TableChartWnd();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

    void OnViewChart();
    void OnUpdateViewChart(CCmdUI* pCmdUI);

    LRESULT OnTableGridUpdated(WPARAM wParam, LPARAM lParam);

    // CTableChildWnd overrides
    bool IsTabViewActive() override { return true; }

private:
    SplitterWndWithDifferingViews m_splitterWnd;
    CTabView* m_tabView;
    HtmlViewerView* m_htmlViewerView;

    ChartManager m_chartManager;
    bool m_chartWasShowingOnPreviousChartableTable;
    std::unique_ptr<ActionInvoker::ListenerHolder> m_actionInvokerListenerHolder;
};
