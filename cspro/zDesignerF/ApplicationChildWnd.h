#pragma once

#include <zUToolO/OxFWndDk.h>

class CLogicCtrl;
class CLogicView;
class LogicDialogBar;
class LogicReferenceWnd;


class ApplicationChildWnd : public COXMDIChildWndSizeDock
{
public:
    virtual CLogicView* GetSourceLogicView() = 0;
    virtual CLogicCtrl* GetSourceLogicCtrl() = 0;

    virtual LogicDialogBar& GetLogicDialogBar() = 0;
    virtual LogicReferenceWnd& GetLogicReferenceWnd() = 0;
};
