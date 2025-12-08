#pragma once

#include <zUtilO/WindowsWS.h>


// --------------------------------------------------------------------------
// RadioEnumHelper
// --------------------------------------------------------------------------

template<typename T>
class RadioEnumHelper
{
    static_assert(sizeof(T) == sizeof(int) || std::is_same_v<T, bool>);

public:
    RadioEnumHelper(std::vector<T> values);

    size_t GetNumberOptions() const { return m_values.size(); }

    int ToForm(T value) const;

    T FromForm(int value) const;

private:
    std::vector<T> m_values;
};


template<typename T>
void DDX_Radio(CDataExchange* pDX, int nIDC, const RadioEnumHelper<T>& radio_enum_helper, T& enum_value);

template<typename T>
void DDX_CBIndex(CDataExchange* pDX, int nIDC, const RadioEnumHelper<T>& radio_enum_helper, T& enum_value);

// When using std::optional<T>, the value m_values.size() is used for std::nullopt.
template<typename T>
void DDX_CBIndex(CDataExchange* pDX, int nIDC, const RadioEnumHelper<T>& radio_enum_helper, std::optional<T>& optional_enum_value);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
RadioEnumHelper<T>::RadioEnumHelper(std::vector<T> values)
    :   m_values(std::move(values))
{
}


template<typename T>
int RadioEnumHelper<T>::ToForm(const T value) const
{
    const auto& lookup = std::find(m_values.cbegin(), m_values.cend(), value);
    return ( lookup != m_values.cend() ) ? static_cast<int>(std::distance(m_values.cbegin(), lookup)) :
                                           ReturnProgrammingError(0);
}


template<typename T>
T RadioEnumHelper<T>::FromForm(const int value) const
{
    return static_cast<T>(m_values[value]);
}


template<typename T>
void DDX_Radio(CDataExchange* const pDX, const int nIDC, const RadioEnumHelper<T>& radio_enum_helper, T& enum_value)
{
    int dlg_value;

    if( pDX->m_bSaveAndValidate )
    {
        DDX_Radio(pDX, nIDC, dlg_value);
        enum_value = radio_enum_helper.FromForm(dlg_value);
    }

    else
    {
        dlg_value = radio_enum_helper.ToForm(enum_value);
        DDX_Radio(pDX, nIDC, dlg_value);
    }
}


template<typename T>
void DDX_CBIndex(CDataExchange* const pDX, const int nIDC, const RadioEnumHelper<T>& radio_enum_helper, T& enum_value)
{
    int dlg_value;

    if( pDX->m_bSaveAndValidate )
    {
        DDX_CBIndex(pDX, nIDC, dlg_value);
        enum_value = radio_enum_helper.FromForm(dlg_value);
    }

    else
    {
        dlg_value = radio_enum_helper.ToForm(enum_value);
        DDX_CBIndex(pDX, nIDC, dlg_value);
    }
}


template<typename T>
void DDX_CBIndex(CDataExchange* const pDX, const int nIDC, const RadioEnumHelper<T>& radio_enum_helper, std::optional<T>& optional_enum_value)
{
    int dlg_value;

    if( pDX->m_bSaveAndValidate )
    {
        DDX_CBIndex(pDX, nIDC, dlg_value);

        if( static_cast<size_t>(dlg_value) == radio_enum_helper.GetNumberOptions() )
        {
            optional_enum_value.reset();
        }

        else
        {
            optional_enum_value = radio_enum_helper.FromForm(dlg_value);
        }
    }

    else
    {
        dlg_value = optional_enum_value.has_value() ? radio_enum_helper.ToForm(*optional_enum_value) :
                                                      radio_enum_helper.GetNumberOptions();
        DDX_CBIndex(pDX, nIDC, dlg_value);
    }
}
