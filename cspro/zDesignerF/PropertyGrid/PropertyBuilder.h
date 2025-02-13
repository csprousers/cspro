#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/PropertyGrid/Property.h>
#include <zUtilO/PortableColor.h>


namespace PropertyGrid
{
    template<typename T>
    struct PropertyGridData
    {
        CString property_name;
        CString property_description;
        bool allow_direct_edit;
        std::optional<T> value;
        std::function<CString(const T&)> format_callback;
        std::function<void(const T&)> validation_callback;
        std::function<void(const T&)> update_callback;
        std::function<std::optional<T>()> button_click_callback;
    };


    namespace Type
    {
        struct Filename
        {
            CString filename;
            CString display_relative_to_filename;
        };

        struct ImageFilename : public Filename
        {
        };
    }


    template<typename T>
    class PropertyBuilderBase
    {
    protected:
        PropertyBuilderBase(const CString& property_name, const CString& property_description, std::optional<T> value = std::nullopt)
        {
            m_data = std::make_shared<PropertyGridData<T>>(PropertyGridData<T>
            {
                property_name,
                property_description,
                true,
                std::move(value)
            });
        }

    public:
        virtual ~PropertyBuilderBase() { }

        PropertyBuilderBase& SetValue(T value)
        {
            m_data->value = std::move(value);
            return *this;
        }

        PropertyBuilderBase& DisableDirectEdit()
        {
            m_data->allow_direct_edit = false;
            return *this;
        }

        PropertyBuilderBase& SetOnFormat(std::function<CString(const T&)> format_callback)
        {
            m_data->format_callback = std::move(format_callback);
            return *this;
        }

        PropertyBuilderBase& SetOnValidate(std::function<void(const T&)> validation_callback)
        {
            // if there are any errors on validation, a PropertyValidationException<T> exception
            // should be thrown with a valid value and an error message (which will be displayed
            // using PostMessage due to threading issues)
            m_data->validation_callback = std::move(validation_callback);
            return *this;
        }

        PropertyBuilderBase& SetOnUpdate(std::function<void(const T&)> update_callback)
        {
            m_data->update_callback = std::move(update_callback);
            return *this;
        }

        PropertyBuilderBase& SetOnButtonClick(std::function<std::optional<T>()> button_click_callback)
        {
            // the callback should return the value on success
            m_data->button_click_callback = std::move(button_click_callback);
            return *this;
        }

        CMFCPropertyGridProperty* Create()
        {
            ASSERT(m_data->value.has_value());

            // an update callback must be set if allowing direct edits
            if( m_data->allow_direct_edit )
                ASSERT(m_data->update_callback);

            CMFCPropertyGridProperty* property = ToProperty();
            
            if( !m_data->allow_direct_edit )
                property->AllowEdit(FALSE);

            return property;
        }

    protected:
        virtual CMFCPropertyGridProperty* ToProperty() = 0;

    protected:
        std::shared_ptr<PropertyGridData<T>> m_data;
    };


    template<typename T>
    class PropertyBuilder : public PropertyBuilderBase<T>
    {
    public:
        PropertyBuilder(const CString& property_name, const CString& property_description, std::optional<T> value = std::nullopt)
            :   PropertyBuilderBase<T>(property_name, property_description, std::move(value))
        {
        }

    protected:
        CMFCPropertyGridProperty* ToProperty() override;
    };


#pragma warning(push)
#pragma warning(disable:4661) // disable: 'identifier' : no suitable definition provided for explicit template instantiation request
                              // because the definitions for these are in .cpp files
    template class CLASS_DECL_ZDESIGNERF PropertyBuilder<bool>;
    template class CLASS_DECL_ZDESIGNERF PropertyBuilder<CString>;
    template class CLASS_DECL_ZDESIGNERF PropertyBuilder<PortableColor>;
    template class CLASS_DECL_ZDESIGNERF PropertyBuilder<Type::Filename>;
    template class CLASS_DECL_ZDESIGNERF PropertyBuilder<Type::ImageFilename>;
#pragma warning(pop)
}
