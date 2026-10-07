#pragma once

#include <zUToolO/OxFWndDk.h>

class LogicCtrl;
class LogicDialogBar;
class LogicReferenceWnd;
class LogicView;


class ApplicationChildWnd : public COXMDIChildWndSizeDock
{
public:
    virtual LogicView* GetSourceLogicView() = 0;
    virtual LogicCtrl* GetSourceLogicCtrl() = 0;

    virtual LogicDialogBar& GetLogicDialogBar() = 0;
    virtual LogicReferenceWnd& GetLogicReferenceWnd() = 0;
};
