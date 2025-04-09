#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/PropertyGrid/Property.h>
#include <zUtilO/PortableColor.h>

namespace PropertyGrid { template<typename T> class PropertyBuilder;
                         template<typename T> class PropertyBuilderBase;
                         template<typename T> struct PropertyGridData;
                         namespace Type { struct Filename;
                                          struct ImageFilePath; } }


template<typename T>
struct PropertyGrid::PropertyGridData
{
    CString property_name;
    std::wstring property_description;
    bool allow_direct_edit;
    std::optional<T> value;
    std::function<CString(const T&)> format_callback;
    std::function<void(const T&)> validation_callback;
    std::function<void(const T&)> update_callback;
    std::function<std::optional<T>()> button_click_callback;
};


struct PropertyGrid::Type::Filename
{
    CString filename;
    CString display_relative_to_filename;
};


struct PropertyGrid::Type::ImageFilePath : public Filename
{
};


template<typename T>
class PropertyGrid::PropertyBuilderBase
{
protected:
    PropertyBuilderBase(const CString& property_name, std::wstring property_description, std::optional<T> value = std::nullopt);

public:
    virtual ~PropertyBuilderBase() { }

    PropertyBuilderBase& SetValue(T value);

    PropertyBuilderBase& DisableDirectEdit();

    PropertyBuilderBase& SetOnFormat(std::function<CString(const T&)> format_callback);

    // if there are any errors on validation, a PropertyValidationException<T> exception
    // should be thrown with a valid value and an error message (which will be displayed
    // using PostMessage due to threading issues)
    PropertyBuilderBase& SetOnValidate(std::function<void(const T&)> validation_callback);

    PropertyBuilderBase& SetOnUpdate(std::function<void(const T&)> update_callback);

    // the callback should return the value on success
    PropertyBuilderBase& SetOnButtonClick(std::function<std::optional<T>()> button_click_callback);

    CMFCPropertyGridProperty* Create();

protected:
    virtual CMFCPropertyGridProperty* ToProperty() = 0;

protected:
    std::shared_ptr<PropertyGridData<T>> m_data;
};


template<typename T>
class PropertyGrid::PropertyBuilder : public PropertyBuilderBase<T>
{
public:
    PropertyBuilder(const CString& property_name, std::wstring property_description, std::optional<T> value = std::nullopt);

protected:
    CMFCPropertyGridProperty* ToProperty() override;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
PropertyGrid::PropertyBuilderBase<T>::PropertyBuilderBase(const CString& property_name, std::wstring property_description, std::optional<T> value/* = std::nullopt*/)
    :   m_data(std::make_unique<PropertyGridData<T>>(PropertyGridData<T>
            {
                property_name,
                std::move(property_description),
                true,
                std::move(value)
            }))
{
}


template<typename T>
PropertyGrid::PropertyBuilderBase<T>& PropertyGrid::PropertyBuilderBase<T>::SetValue(T value)
{
    m_data->value = std::move(value);
    return *this;
}


template<typename T>
PropertyGrid::PropertyBuilderBase<T>& PropertyGrid::PropertyBuilderBase<T>::DisableDirectEdit()
{
    m_data->allow_direct_edit = false;
    return *this;
}


template<typename T>
PropertyGrid::PropertyBuilderBase<T>& PropertyGrid::PropertyBuilderBase<T>::SetOnFormat(std::function<CString(const T&)> format_callback)
{
    m_data->format_callback = std::move(format_callback);
    return *this;
}


template<typename T>
PropertyGrid::PropertyBuilderBase<T>& PropertyGrid::PropertyBuilderBase<T>::SetOnValidate(std::function<void(const T&)> validation_callback)
{
    m_data->validation_callback = std::move(validation_callback);
    return *this;
}


template<typename T>
PropertyGrid::PropertyBuilderBase<T>& PropertyGrid::PropertyBuilderBase<T>::SetOnUpdate(std::function<void(const T&)> update_callback)
{
    m_data->update_callback = std::move(update_callback);
    return *this;
}


template<typename T>
PropertyGrid::PropertyBuilderBase<T>& PropertyGrid::PropertyBuilderBase<T>::SetOnButtonClick(std::function<std::optional<T>()> button_click_callback)
{
    m_data->button_click_callback = std::move(button_click_callback);
    return *this;
}


template<typename T>
CMFCPropertyGridProperty* PropertyGrid::PropertyBuilderBase<T>::Create()
{
    ASSERT(m_data->value.has_value());

    // an update callback must be set if allowing direct edits
    ASSERT(!m_data->allow_direct_edit || m_data->update_callback);

    CMFCPropertyGridProperty* const property = ToProperty();

    if( !m_data->allow_direct_edit )
        property->AllowEdit(FALSE);

    return property;
}



#pragma warning(push)
#pragma warning(disable:4661) // disable: 'identifier' : no suitable definition provided for explicit template instantiation request
                              // because the definitions for these are in .cpp files

template<typename T>
PropertyGrid::PropertyBuilder<T>::PropertyBuilder(const CString& property_name, std::wstring property_description, std::optional<T> value/* = std::nullopt*/)
    :   PropertyBuilderBase<T>(property_name, std::move(property_description), std::move(value))
{
}

template class CLASS_DECL_ZDESIGNERF PropertyGrid::PropertyBuilder<bool>;
template class CLASS_DECL_ZDESIGNERF PropertyGrid::PropertyBuilder<CString>;
template class CLASS_DECL_ZDESIGNERF PropertyGrid::PropertyBuilder<std::string>;
template class CLASS_DECL_ZDESIGNERF PropertyGrid::PropertyBuilder<PortableColor>;
template class CLASS_DECL_ZDESIGNERF PropertyGrid::PropertyBuilder<PropertyGrid::Type::Filename>;
template class CLASS_DECL_ZDESIGNERF PropertyGrid::PropertyBuilder<PropertyGrid::Type::ImageFilePath>;

#pragma warning(pop)
