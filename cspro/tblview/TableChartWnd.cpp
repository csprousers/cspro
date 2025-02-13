#include "StdAfx.h"
#include "TableChartWnd.h"
#include <zHtml/HtmlViewerView.h>
#include <zAction/AccessToken.h>
#include <zAction/WebController.h>


IMPLEMENT_DYNCREATE(TableChartWnd, CTableChildWnd)

BEGIN_MESSAGE_MAP(TableChartWnd, CTableChildWnd)
    ON_COMMAND(ID_VIEW_CHART, OnViewChart)
    ON_UPDATE_COMMAND_UI(ID_VIEW_CHART, OnUpdateViewChart)
    ON_MESSAGE(UWM::Table::TableGridUpdated, OnTableGridUpdated)
END_MESSAGE_MAP()


TableChartWnd::TableChartWnd()
    :   m_tabView(nullptr),
        m_htmlViewerView(nullptr),
        m_chartWasShowingOnPreviousChartableTable(false)
{
}


BOOL TableChartWnd::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
    if( !m_splitterWnd.Create(this, 1, 2, CSize(1, 1), pContext, WS_CHILD | WS_VISIBLE | SPLS_DYNAMIC_SPLIT) )
        return FALSE;

    m_tabView = assert_cast<CTabView*>(m_splitterWnd.GetPane(0, 0));

    return TRUE;
}


void TableChartWnd::OnViewChart()
{
    ASSERT(( m_htmlViewerView != nullptr ) == ( m_splitterWnd.GetColumnCount() == 2 ));

    // if the chart view is showing, hide it
    if( m_htmlViewerView != nullptr )
    {
        m_htmlViewerView = nullptr;
        m_splitterWnd.DeleteColumn(1);
    }

    // otherwise create the view and show it on the right half of the window
    else
    {
        CRect rect;
        GetClientRect(rect);
        m_splitterWnd.SplitColumn(rect.Width() / 2, RUNTIME_CLASS(HtmlViewerView));

        m_htmlViewerView = assert_cast<HtmlViewerView*>(m_splitterWnd.GetPane(0, 1));

        // set up the Action Invoker to serve the frequency JSON
        HtmlViewCtrl& html_view_ctrl = m_htmlViewerView->GetHtmlViewCtrl();

        ActionInvoker::WebController& web_controller = html_view_ctrl.RegisterCSProHostObject();
        web_controller.GetCaller().AddAccessTokenOverride(std::wstring(ActionInvoker::AccessToken::Charting_FrequencyView_sv));

        web_controller.GetListener().SetOnGetInputDataCallback(
            [&]()
            {
                return m_chartManager.GetTableFrequencyJson(m_tabView->GetGrid());
            });

        html_view_ctrl.NavigateTo(m_chartManager.GetFrequencyViewUrl());
    }

    m_chartWasShowingOnPreviousChartableTable = false;
}


void TableChartWnd::OnUpdateViewChart(CCmdUI* pCmdUI)
{
    bool table_supports_charting = m_chartManager.TableSupportsCharting(m_tabView->GetGrid());

    pCmdUI->Enable(table_supports_charting);
    pCmdUI->SetCheck(table_supports_charting && m_htmlViewerView != nullptr);
}


LRESULT TableChartWnd::OnTableGridUpdated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // if the table supports charting...
    if( m_chartManager.TableSupportsCharting(m_tabView->GetGrid()) )
    {
        // ...update the chart
        if( m_htmlViewerView != nullptr )
        {
            m_htmlViewerView->GetHtmlViewCtrl().Reload();
        }

        // ...or, if the chart was previously showing for a chartable table, bring back the chart
        else if( m_chartWasShowingOnPreviousChartableTable )
        {
            OnViewChart();
        }
    }

    // if the current table does not support charting, hide the chart view
    else if( m_htmlViewerView != nullptr )
    {
        OnViewChart();
        m_chartWasShowingOnPreviousChartableTable = true;
    }

    return 1;
}
