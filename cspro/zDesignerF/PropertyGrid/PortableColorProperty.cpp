#include "StdAfx.h"
#include "PropertyBuilder.h"


namespace PropertyGrid
{
    class PortableColorProperty : public CMFCPropertyGridColorProperty, public Property
    {
    public:
        PortableColorProperty(std::shared_ptr<PropertyGridData<PortableColor>> data);

    protected:
        // CMFCPropertyGridColorProperty overrides
        BOOL OnEdit(LPPOINT lptClick) override;
        BOOL OnUpdateValue() override;
        CString FormatProperty() override;

        // Property overrides
        void ValidateProperty() override;
        void SetProperty() override;
        void HandleButtonClick() override { ASSERT(!m_data->button_click_callback); }

    private:
        std::shared_ptr<PropertyGridData<PortableColor>> m_data;
    };


    CMFCPropertyGridProperty* PropertyBuilder<PortableColor>::ToProperty()
    {
        // buttons are handled by CMFCPropertyGridColorProperty
        ASSERT(!m_data->button_click_callback);

        return new PortableColorProperty(m_data);
    }


    PortableColorProperty::PortableColorProperty(std::shared_ptr<PropertyGridData<PortableColor>> data)
        :   CMFCPropertyGridColorProperty(data->property_name, data->value->ToCOLORREF(), nullptr, data->property_description),
            Property(data->allow_direct_edit),
            m_data(data)
    {
        EnableOtherButton(_T("More Colors..."));
    }


    BOOL PortableColorProperty::OnEdit(LPPOINT lptClick)
    {
        // don't use CMFCPropertyGridColorProperty's OnEdit because it restricts 
        // the characters that can be entered (e.g., '#' cannot be entered)
        return CMFCPropertyGridProperty::OnEdit(lptClick);
    }


    BOOL PortableColorProperty::OnUpdateValue()
    {
        // modified from CMFCPropertyGridColorProperty::OnUpdateValue
        if( m_pWndInPlace == nullptr )
        	return FALSE;

        CString color_text;
        m_pWndInPlace->GetWindowText(color_text);

        std::optional<PortableColor> portable_color = PortableColor::FromString(color_text);

        // if the color text was not valid, try again with a # before any text
        if( !portable_color.has_value() && !color_text.IsEmpty() && color_text[0] != _T('#') )
        {
            color_text.Insert(0, _T('#'));
            portable_color = PortableColor::FromString(color_text);
        }

        // set to black if invalid
        if( !portable_color.has_value() )
            portable_color = PortableColor::Black;

        if( m_Color != portable_color->ToCOLORREF() )
        {
            m_Color = portable_color->ToCOLORREF();
    		m_pWndList->OnPropertyChanged(this);
        }
	    
        return TRUE;
    }        


    CString PortableColorProperty::FormatProperty()
    {
        PortableColor value = PortableColor::FromCOLORREF(GetColor());

        if( m_data->format_callback )
        {
            return m_data->format_callback(value);
        }

        else
        {
            // format the colors so that names are used when possible
            return WS2CS(value.ToString());
        }
    }


    void PortableColorProperty::ValidateProperty()
    {
        if( !m_data->validation_callback )
            return;

        try
        {
            PortableColor value = PortableColor::FromCOLORREF(GetColor());
            m_data->validation_callback(value);
        }

        catch( const PropertyValidationException<PortableColor>& property_validation_exception )
        {
            const PortableColor& valid_value = property_validation_exception.GetValidValue();
            SetColor(valid_value.ToCOLORREF());
            throw;
        }
    }


    void PortableColorProperty::SetProperty()
    {
        PortableColor value = PortableColor::FromCOLORREF(GetColor());
        m_data->update_callback(value);
    }
}
