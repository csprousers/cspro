#pragma once

#include <zHtml/HtmlViewerView.h>
#include <zAction/Listener.h>
#include <CSView/ViewDoc.h>


class ViewView : public HtmlViewerView
{
    DECLARE_DYNCREATE(ViewView)

protected:
    ViewView(); // create from serialization only

public:
    const ViewDoc& GetViewDoc() const { return assert_cast<const ViewDoc&>(*GetDocument()); }
    ViewDoc& GetViewDoc()             { return assert_cast<ViewDoc&>(*GetDocument()); }

protected:
    void OnInitialUpdate() override;

private:
    void SetUpActionInvoker();

private:
    std::unique_ptr<ActionInvoker::ListenerHolder> m_actionInvokerListenerHolder;
};
