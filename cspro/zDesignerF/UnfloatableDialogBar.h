#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zUToolO/OxSzDlgB.h>


// a subclass of COXSizeDialogBar that prevents the dialog bar from floating

class CLASS_DECL_ZDESIGNERF UnfloatableDialogBar : public COXSizeDialogBar
{
public:
    UnfloatableDialogBar(int nStyle);

protected:
    DECLARE_MESSAGE_MAP()

    void OnLButtonDown(UINT nFlags, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    void OnRButtonUp(UINT nFlags, CPoint point);
};
