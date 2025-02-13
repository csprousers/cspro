#include "StdAfx.h"
#include "PropertyBuilder.h"


namespace PropertyGrid
{
    class StringProperty : public CMFCPropertyGridProperty, public Property
    {
    public:
        StringProperty(std::shared_ptr<PropertyGridData<CString>> data);

    protected:
        // CMFCPropertyGridColorProperty overrides
        CString FormatProperty() override;
        BOOL HasButton() const override { return m_data->button_click_callback ? TRUE : FALSE;; }

        // Property overrides
        void ValidateProperty() override;
        void SetProperty() override;
        void HandleButtonClick() override;

    private:
        static COleVariant ToOleVariant(const CString& value);
        static CString FromOleVariant(const COleVariant& ole_variant);

    private:
        std::shared_ptr<PropertyGridData<CString>> m_data;
    };


    CMFCPropertyGridProperty* PropertyBuilder<CString>::ToProperty()
    {
        return new StringProperty(m_data);
    }


    StringProperty::StringProperty(std::shared_ptr<PropertyGridData<CString>> data)
        :   CMFCPropertyGridProperty(data->property_name, ToOleVariant(*data->value), data->property_description),
            Property(data->allow_direct_edit),
            m_data(data)
    {
    }


    CString StringProperty::FormatProperty()
    {
        if( m_data->format_callback )
        {
            CString value = FromOleVariant(GetValue());
            return m_data->format_callback(value);
        }

        else
        {
            return CMFCPropertyGridProperty::FormatProperty();
        }
    }

    void StringProperty::ValidateProperty()
    {
        if( !m_data->validation_callback )
            return;

        try
        {
            CString value = FromOleVariant(GetValue());
            m_data->validation_callback(value);
        }

        catch( const PropertyValidationException<CString>& property_validation_exception )
        {
            const CString& valid_value = property_validation_exception.GetValidValue();
            SetValue(ToOleVariant(valid_value));
            throw;
        }
    }


    void StringProperty::SetProperty()
    {
        CString value = FromOleVariant(GetValue());
        m_data->update_callback(value);
    }


    void StringProperty::HandleButtonClick()
    {
        std::optional<CString> new_value = m_data->button_click_callback();

        if( new_value.has_value() )
            SetValue(ToOleVariant(*new_value));
    }


    COleVariant StringProperty::ToOleVariant(const CString& value)
    {
        return COleVariant(value);
    }

    CString StringProperty::FromOleVariant(const COleVariant& ole_variant)
    {
        return CString(ole_variant);
    }
}
