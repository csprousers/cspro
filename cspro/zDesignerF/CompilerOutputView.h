#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zEditO/LogicView.h>
#include <zLogicO/ParserMessage.h>


class CLASS_DECL_ZDESIGNERF CompilerOutputView : public LogicView
{
    DECLARE_DYNAMIC(CompilerOutputView)

    class DoubleClickProcessingLogicCtrl;

protected:
    CompilerOutputView() noexcept;

public:
    struct LogicErrorLocation
    {
        Logic::ParserMessage parser_message;
        std::optional<int> adjusted_line_number_for_bookmark;
    };

    void AddLogicError(Logic::ParserMessage parser_message, std::optional<int> adjusted_line_number_for_bookmark = std::nullopt);
    void ClearLogicErrors();

protected:
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

    // LogicView overrides
    std::unique_ptr<LogicCtrl> CreateLogicCtrl() override;
    void InitializeLogicControl() override;

private:
    void ProcessLButtonDblClk(bool control_pressed);

private:
    DesignerDoc* m_designerDoc; // non-null after OnUpdate's Hint::DesignerDocLoaded
    std::vector<LogicErrorLocation> m_logicErrorLocations;
};
