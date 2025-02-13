#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/ApplicationChildWnd.h>
#include <zUToolO/oxtbvw.h>
#include <zEdit2O/ReadOnlyEditCtrl.h>
#include <zLogicO/ParserMessage.h>
#include <zCapiO/CapiLogicParameters.h>


class CLASS_DECL_ZDESIGNERF CompilerOutputTabViewPage : public COXTabViewPage<ReadOnlyEditCtrl>
{
public:
    struct LogicErrorLocation
    {
        Logic::ParserMessage parser_message;
        std::optional<int> adjusted_line_number_for_bookmark;
    };

    CompilerOutputTabViewPage(ApplicationChildWnd* application_child_wnd);

    void AddLogicError(Logic::ParserMessage parser_message, std::optional<int> adjusted_line_number_for_bookmark = std::nullopt);
    void ClearLogicErrors();

    bool ProcessClicksForReferenceWindow() const override { return false; }

protected:
    DECLARE_MESSAGE_MAP()

    void OnLButtonDblClk(UINT nFlags, CPoint point);

private:
    ApplicationChildWnd* m_applicationChildWnd;
    std::vector<LogicErrorLocation> m_logicErrorLocations;
};
