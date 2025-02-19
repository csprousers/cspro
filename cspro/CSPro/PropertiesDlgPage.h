#pragma once

#include <zUtilF/DialogValidators.h>


class PropertiesDlgPage
{
public:
    virtual ~PropertiesDlgPage() { }

    // called when the page is switched ... the property object should
    // be updated in case it is referenced on other pages
    virtual void FormToProperties() = 0;

    // called when the Reset button is pressed
    virtual void ResetProperties() = 0;


protected:
    // if a property page is based on a dialog that is also shown as a normal dialog, calling this function will shift all the
    // child windows, removing any margin that was part of the dialog, so the leftmost and topmost window are at positions 0
    void ShiftNonPropertyPageDialogChildWindows(CWnd* pWnd);


    // some helper classes for setting/retrieving data from the form
    struct ToForm
    {
        static constexpr int Check(bool value);
        static std::string Text(int value);
    };


    struct FromForm
    {
        static constexpr bool Check(int value);

        template<typename T>
        static T Text(const std::string& text_value, const char* description, std::optional<T> value_if_text_is_blank = std::nullopt);

        template<>
        static int Text<int>(const std::string& text_value, const char* description, std::optional<int> value_if_text_is_blank/* = std::nullopt*/);
    };
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr int PropertiesDlgPage::ToForm::Check(const bool value)
{
    return value ? BST_CHECKED : BST_UNCHECKED;
}


inline std::string PropertiesDlgPage::ToForm::Text(const int value)
{
    return IntToString(value);
}


constexpr bool PropertiesDlgPage::FromForm::Check(const int value)
{
    return ( value == BST_CHECKED );
}


template<>
int PropertiesDlgPage::FromForm::Text<int>(const std::string& text_value, const char* const description, const std::optional<int> value_if_text_is_blank/* = std::nullopt*/)
{
    if( SO::IsBlank(text_value) )
    {
        if( value_if_text_is_blank.has_value() )
            return *value_if_text_is_blank;

        throw CSProException("You cannot leave the field blank: %s", description);
    }

    if( !CIMSAString::IsInteger(text_value) )
        throw CSProException("You must enter an integer for the field: %s", description);

    return atoi(text_value.c_str());
}
