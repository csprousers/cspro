#pragma once

#include "PropertyBuilder.h"


namespace PropertyGrid
{
    template<typename T>
    class ListProperty : public CMFCPropertyGridProperty, public Property
    {
    public:
        ListProperty(std::shared_ptr<PropertyGridData<T>> data, std::shared_ptr<const std::vector<std::tuple<T, CString>>> options)
            :   CMFCPropertyGridProperty(data->property_name, ToOleVariant(*data->value, *options), data->property_description),
                Property(data->allow_direct_edit),
                m_data(data),
                m_options(options)
        {
        }

    protected:
        void ValidateProperty() override
        {
            if( !m_data->validation_callback )
                return;

            try
            {
                T value = FromOleVariant(GetValue());
                m_data->validation_callback(value);
            }

            catch( const PropertyValidationException<T>& property_validation_exception )
            {
                T valid_value = property_validation_exception.GetValidValue();
                SetValue(ToOleVariant(valid_value));
                throw;
            }
        }

        void SetProperty() override
        {
            T value = FromOleVariant(GetValue());
            m_data->update_callback(value);
        }

        void HandleButtonClick() override
        {
            ASSERT(!m_data->button_click_callback);
        }

    private:
        static COleVariant ToOleVariant(T value, const std::vector<std::tuple<T, CString>>& options)
        {
            const auto& options_search = std::find_if(options.cbegin(), options.cend(),
                [&](const auto& option) { return ( std::get<0>(option) == value ); });

            if( options_search != options.cend() )
                return COleVariant(std::get<1>(*options_search));

            else
            {
                ASSERT(false);
                return COleVariant(_T(""));
            }
        }

        COleVariant ToOleVariant(T value)
        {
            return ToOleVariant(value, *m_options);
        }

        COleVariant ToOleVariant(T value) const
        {
            const auto& options_search = std::find_if(m_options->cbegin(), m_options->cend(),
                [&](const auto& option) { return ( std::get<0>(option) == value ); });
            ASSERT(options_search != m_options->cend());

            return COleVariant(std::get<1>(*options_search));
        }

        T FromOleVariant(const COleVariant& ole_variant)
        {
            CString option_text(ole_variant);

            const auto& options_search = std::find_if(m_options->cbegin(), m_options->cend(),
                [&](const auto& option) { return ( std::get<1>(option) == option_text ); });
            ASSERT(options_search != m_options->cend());

            return std::get<0>(*options_search);
        }

    private:
        std::shared_ptr<PropertyGridData<T>> m_data;
        std::shared_ptr<const std::vector<std::tuple<T, CString>>> m_options;
    };


    template<typename T>
    class ListPropertyBuilder : public PropertyBuilderBase<T>
    {
    public:
        ListPropertyBuilder(CString property_name, const TCHAR* property_description, std::optional<T> value = std::nullopt)
            :   PropertyBuilderBase<T>(property_name, property_description, value),
                m_options(std::make_shared<std::vector<std::tuple<T, CString>>>())
        {
        }

        ListPropertyBuilder& AddOption(T option_value, const TCHAR* option_text)
        {
            ASSERT(std::find_if(m_options->cbegin(), m_options->cend(),
                   [&](const auto& option) { return ( std::get<0>(option) == option_value ||
                                                      std::get<1>(option).CompareNoCase(option_text) == 0 ); })
                   == m_options->cend());

            m_options->emplace_back(option_value, option_text);
            return *this;
        }

    private:
        CMFCPropertyGridProperty* ToProperty() override
        {
            // buttons are not possible because the button location is used to display the list
            ASSERT(!m_data->button_click_callback);

            // formatting is not allowed
            ASSERT(!m_data->format_callback);

            CMFCPropertyGridProperty* property = new ListProperty<T>(m_data, m_options);

            for( const auto& [ option_value, option_text ] : *m_options )
                property->AddOption(option_text);

            ASSERT(!m_options->empty() && property->GetOptionCount() == (int)m_options->size());

            property->AllowEdit(FALSE);

            return property;
        }

    private:
        std::shared_ptr<std::vector<std::tuple<T, CString>>> m_options;
    };
}
