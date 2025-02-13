#pragma once

#include <zDesignerF/PropertyGrid/PropertyBuilder.h>
#include <zToolsO/Tools.h>


namespace PropertyGrid
{
    template<typename T>
    class FilenamePropertyBase : public CMFCPropertyGridProperty, public Property
    {
    public:
        FilenamePropertyBase(std::shared_ptr<PropertyGridData<T>> data);

    protected:
        // CMFCPropertyGridColorProperty overrides
        CString FormatProperty() override;
        BOOL HasButton() const override { return TRUE; }

        // Property overrides
        void ValidateProperty() override;
        void SetProperty() override;
        void HandleButtonClick() override;

        COleVariant ToOleVariant(const T& filename);
        T FromOleVariant(const COleVariant& ole_variant);

    protected:
        std::shared_ptr<PropertyGridData<T>> m_data;
    };


    template<typename T>
    FilenamePropertyBase<T>::FilenamePropertyBase(std::shared_ptr<PropertyGridData<T>> data)
        :   CMFCPropertyGridProperty(data->property_name, ToOleVariant(*data->value), data->property_description),
            Property(data->allow_direct_edit),
            m_data(data)
    {
        ASSERT(m_data->value.has_value() && !m_data->value->display_relative_to_filename.IsEmpty());
    }


    template<typename T>
    CString FilenamePropertyBase<T>::FormatProperty()
    {
        T value = FromOleVariant(GetValue());

        if( m_data->format_callback )
        {
            return m_data->format_callback(value);
        }

        else
        {
            return GetRelativeFNameForDisplay<CString>(m_data->value->display_relative_to_filename, value.filename);
        }
    }


    template<typename T>
    void FilenamePropertyBase<T>::ValidateProperty()
    {
        try
        {
            T value = FromOleVariant(GetValue());

            // default validation
            if( !value.filename.IsEmpty() && !PortableFunctions::FileIsRegular(value.filename) )
            {
                throw PropertyValidationException<T>(FromOleVariant(COleVariant(CString())),
                    FormatText(_T("No file could be located at %s"), (LPCTSTR)value.filename));
            }

            // custom validation
            if( m_data->validation_callback )
                m_data->validation_callback(value);
        }

        catch( const PropertyValidationException<T>& property_validation_exception )
        {
            T valid_value = property_validation_exception.GetValidValue();
            SetValue(ToOleVariant(valid_value));
            throw;
        }
    }


    template<typename T>
    void FilenamePropertyBase<T>::SetProperty()
    {
        T value = FromOleVariant(GetValue());
        m_data->update_callback(value);
    }


    template<typename T>
    void FilenamePropertyBase<T>::HandleButtonClick()
    {
        std::optional<T> new_value;

        if( m_data->button_click_callback )
        {
            new_value = m_data->button_click_callback();
        }

        else
        {
            CFileDialog file_dlg(TRUE, nullptr, FromOleVariant(GetValue()).filename,
                                 OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, nullptr, m_pWndList);

            if( file_dlg.DoModal() == IDOK )
                new_value = FromOleVariant(COleVariant(file_dlg.GetPathName()));
        }

        if( new_value.has_value() )
        {
            SetValue(ToOleVariant(*new_value));
            m_pWndList->OnPropertyChanged(this);
        }
    }


    template<typename T>
    COleVariant FilenamePropertyBase<T>::ToOleVariant(const T& filename)
    {
        return COleVariant(filename.filename);
    }

    template<typename T>
    T FilenamePropertyBase<T>::FromOleVariant(const COleVariant& ole_variant)
    {
        return T
        {
            WS2CS(MakeFullPath(GetWorkingFolder(m_data->value->display_relative_to_filename), CS2WS(CString(ole_variant)))),
            m_data->value->display_relative_to_filename
        };
    }
}
