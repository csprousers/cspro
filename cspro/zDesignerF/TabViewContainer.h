#pragma once

#include <zUToolO/oxtbvw.h>


// --------------------------------------------------------------------------
// TabViewContainer
//
// A class for controlling the compiler output / messages tabs.
// --------------------------------------------------------------------------

class TabViewContainer : public COXTabViewContainer
{
public:
    BOOL SetActivePageIndex(int nIndex) override;

    void UpdateScrollState() override;
    void UpdateScrollInfo() override;
};
