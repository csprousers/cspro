#pragma once

#include <CSDocument/TextEditFrame.h>


class DocSetBuildHandlerFrame : public TextEditFrame
{
protected:
    DocSetBuildHandlerFrame() { }

public:
    virtual ~DocSetBuildHandlerFrame() { }

    virtual DocSetComponent::Type GetDocSetComponentType() const = 0;

    void PopulateBuildMenu(CMenu& popup_menu);

protected:
    virtual void AddFrameSpecificItemsToBuildMenu(DynamicMenuBuilder& dynamic_menu_builder) = 0;

protected:
    DECLARE_MESSAGE_MAP()

    void ShowHtmlAndRestoreScrollbarState(HtmlViewCtrl& html_view_ctrl, std::wstring url);
    LRESULT OnShowHtmlAndRestoreScrollbarState(WPARAM wParam, LPARAM lParam);    

    void OnUpdateDocumentSetMustExist(CCmdUI* pCmdUI);

    void OnCompileDocumentSet();
    void OnExportDocumentSet();

protected:
    std::tuple<std::wstring, int> m_currentUrlAndScrollbarStateToRestore;
};
