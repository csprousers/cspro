#include "StdAfx.h"
#include "PropertyBuilder.h"

namespace PropertyGrid { class StringProperty; }


class PropertyGrid::StringProperty : public CMFCPropertyGridProperty, public Property
{
public:
    StringProperty(std::shared_ptr<PropertyGridData<std::string>> data);

protected:
    // CMFCPropertyGridColorProperty overrides
    CString FormatProperty() override;
    BOOL HasButton() const override { return m_data->button_click_callback ? TRUE : FALSE; }

    // Property overrides
    void ValidateProperty() override;
    void SetProperty() override;
    void HandleButtonClick() override;

private:
    std::shared_ptr<PropertyGridData<std::string>> m_data;
};


CMFCPropertyGridProperty* PropertyGrid::PropertyBuilder<std::string>::ToProperty()
{
    return new StringProperty(m_data);
}


PropertyGrid::StringProperty::StringProperty(std::shared_ptr<PropertyGridData<std::string>> data)
    :   CMFCPropertyGridProperty(data->property_name, WindowsTC::ToOleVariant(*data->value), data->property_description),
        Property(data->allow_direct_edit),
        m_data(std::move(data))
{
}


CString PropertyGrid::StringProperty::FormatProperty()
{
    if( m_data->format_callback )
    {
        const std::string value = WindowsTC::FromOleVariant(GetValue());
        return m_data->format_callback(value);
    }

    else
    {
        return CMFCPropertyGridProperty::FormatProperty();
    }
}


void PropertyGrid::StringProperty::ValidateProperty()
{
    if( !m_data->validation_callback )
        return;

    try
    {
        const std::string value = WindowsTC::FromOleVariant(GetValue());
        m_data->validation_callback(value);
    }

    catch( const PropertyValidationException<std::string>& property_validation_exception )
    {
        const std::string& valid_value = property_validation_exception.GetValidValue();
        SetValue(WindowsTC::ToOleVariant(valid_value));
        throw;
    }
}


void PropertyGrid::StringProperty::SetProperty()
{
    const std::string value = WindowsTC::FromOleVariant(GetValue());
    m_data->update_callback(value);
}


void PropertyGrid::StringProperty::HandleButtonClick()
{
    const std::optional<std::string> new_value = m_data->button_click_callback();

    if( new_value.has_value() )
        SetValue(WindowsTC::ToOleVariant(*new_value));
}
