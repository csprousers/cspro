#pragma once

#include <zDesignerF/PropertyGrid/PropertyValidationException.h>
#include <afxpropertygridctrl.h>


namespace PropertyGrid
{
    class Property
    {
    public:
        Property(bool process_property_changed_event)
            :   m_processPropertyChangedEvent(process_property_changed_event)
        {
        }

        virtual ~Property() { }

        bool ProcessPropertyChangedEvent() const { return m_processPropertyChangedEvent; }

        virtual void ValidateProperty() = 0;

        virtual void SetProperty() = 0;

        virtual void HandleButtonClick() = 0;

    private:
        bool m_processPropertyChangedEvent;
    };
}
