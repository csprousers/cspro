#include "StdAfx.h"
#include "PropertyManager.h"


namespace PropertyGrid
{
    void PropertyManager::OnPropertyChanged(CMFCPropertyGridProperty* pProp)
    {
        Property* property = dynamic_cast<Property*>(pProp);

        if( property == nullptr || !property->ProcessPropertyChangedEvent() )
            return;

        // if the validation fails, store the error message and display it using PostMessage due to threading issues
        try
        {
            property->ValidateProperty();
        }

        catch( const PropertyValidationExceptionBase& property_validation_exception )
        {
            ErrorMessage::PostMessageForDisplay(property_validation_exception);
            return;
        }

        // set the valid value
        PushUndo();
        property->SetProperty();
        SetModified();
    }


    void PropertyManager::OnClickButton(CMFCPropertyGridProperty* pProp)
    {
        Property* property = dynamic_cast<Property*>(pProp);

        if( property == nullptr )
            return;

        property->HandleButtonClick();
    }
}
