#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zUToolO/oxtbvw.h>


// a class for controlling the compiler output / messages tabs

class CLASS_DECL_ZDESIGNERF TabViewContainer : public COXTabViewContainer
{
public:
    void UpdateScrollState() override;
    void UpdateScrollInfo() override;
};
