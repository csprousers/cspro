#include "StdAfx.h"
#include "PropertyBuilder.h"


namespace PropertyGrid
{
    class BoolProperty : public CMFCPropertyGridProperty, public Property
    {
    public:
        BoolProperty(std::shared_ptr<PropertyGridData<bool>> data);

    protected:
        // Property overrides
        void ValidateProperty() override;
        void SetProperty() override;
        void HandleButtonClick() override { ASSERT(!m_data->button_click_callback); }

    private:
        static COleVariant ToOleVariant(bool value);
        static bool FromOleVariant(const COleVariant& ole_variant);

    private:
        std::shared_ptr<PropertyGridData<bool>> m_data;
    };


    CMFCPropertyGridProperty* PropertyBuilder<bool>::ToProperty()
    {
        // buttons are not possible because true/false will be displayed as a list
        ASSERT(!m_data->button_click_callback);

        // formatting can be implemented if needed
        ASSERT(!m_data->format_callback);

        return new BoolProperty(m_data);
    }


    BoolProperty::BoolProperty(std::shared_ptr<PropertyGridData<bool>> data)
        :   CMFCPropertyGridProperty(data->property_name, ToOleVariant(*data->value), data->property_description),
            Property(data->allow_direct_edit),
            m_data(data)
    {
    }


    void BoolProperty::ValidateProperty()
    {
        if( !m_data->validation_callback )
            return;

        try
        {
            bool value = FromOleVariant(GetValue());
            m_data->validation_callback(value);
        }

        catch( const PropertyValidationException<bool>& property_validation_exception )
        {
            bool valid_value = property_validation_exception.GetValidValue();
            SetValue(ToOleVariant(valid_value));
            throw;
        }
    }


    void BoolProperty::SetProperty()
    {
        bool value = FromOleVariant(GetValue());
        m_data->update_callback(value);
    }


    COleVariant BoolProperty::ToOleVariant(bool value)
    {
        return COleVariant(value ? VARIANT_TRUE : VARIANT_FALSE, (VARTYPE)VT_BOOL);
    }

    bool BoolProperty::FromOleVariant(const COleVariant& ole_variant)
    {
        return ( ole_variant.boolVal == VARIANT_TRUE );
    }
}
