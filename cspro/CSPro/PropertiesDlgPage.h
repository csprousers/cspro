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
        static constexpr int Check(bool value)
        {
            return value ? BST_CHECKED : BST_UNCHECKED;
        }

        static CString Text(int value)
        {
            return IntToString(value);
        }
    };


    struct FromForm
    {
        static constexpr bool Check(int value)
        {
            return ( value == BST_CHECKED );
        }

        template<typename T> static T Text(NullTerminatedString text_value, const TCHAR* description, std::optional<T> value_if_text_is_blank = std::nullopt);

        template<> static int Text<int>(NullTerminatedString text_value, const TCHAR* description, std::optional<int> value_if_text_is_blank/* = std::nullopt*/)
        {
            if( SO::IsBlank(text_value) )
            {
                if( value_if_text_is_blank.has_value() )
                    return *value_if_text_is_blank;

                throw CSProException(_T("You cannot leave the field blank: %s"), description);
            }

            if( !CIMSAString::IsInteger(text_value) )
                throw CSProException(_T("You must enter an integer for the field: %s"), description);

            return _ttoi(text_value.c_str());
        }
    };
};
