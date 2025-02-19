#include "StdAfx.h"
#include "PropertyManager.h"


void PropertyGrid::PropertyManager::OnPropertyChanged(CMFCPropertyGridProperty* const pProp)
{
    Property* const property = dynamic_cast<Property*>(pProp);

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


void PropertyGrid::PropertyManager::OnClickButton(CMFCPropertyGridProperty* const pProp)
{
    Property* const property = dynamic_cast<Property*>(pProp);

    if( property == nullptr )
        return;

    property->HandleButtonClick();
}
