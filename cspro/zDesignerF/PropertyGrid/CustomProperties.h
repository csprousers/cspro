#pragma once

#include <zDesignerF/zDesignerF.h>
#include <afxpropertygridctrl.h>


namespace PropertyGrid
{
    // ------------------------------------------
    // HeadingProperty
    // 
    // the heading is displayed in bold text
    // ------------------------------------------
    class CLASS_DECL_ZDESIGNERF HeadingProperty : public CMFCPropertyGridProperty
    {
    public:
        HeadingProperty(const CString& heading_name);

        void OnDrawName(CDC* pDC, CRect rect) override;
    };



    // ------------------------------------------
    // ReadOnlyTextProperty
    // 
    // the name and value text are light gray
    // and the value is read only
    // ------------------------------------------
    class CLASS_DECL_ZDESIGNERF ReadOnlyTextProperty : public CMFCPropertyGridProperty
    {
    public:
        ReadOnlyTextProperty(const CString& property_name, const TCHAR* property_description, CString value);

        void OnDrawName(CDC* pDC, CRect rect) override;
        void OnDrawValue(CDC* pDC, CRect rect) override;

    private:
        static const COLORREF GrayTextColor = RGB(105, 105, 105);
    };
}
