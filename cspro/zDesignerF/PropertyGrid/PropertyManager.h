#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/PropertyGrid/Property.h>


namespace PropertyGrid
{
    class CLASS_DECL_ZDESIGNERF PropertyManager
    {
    public:
        virtual ~PropertyManager() { }

        void OnPropertyChanged(CMFCPropertyGridProperty* pProp);

        void OnClickButton(CMFCPropertyGridProperty* pProp);

    protected:
        virtual void PushUndo() = 0;

        virtual void SetModified() = 0;
    };
}
