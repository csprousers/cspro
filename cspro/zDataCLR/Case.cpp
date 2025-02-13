#include "Stdafx.h"
#include "Case.h"
#include "DataViewerController.h"


CSPro::Data::Case::Case(std::shared_ptr<::Case> data_case)
    :   m_case(new std::shared_ptr<::Case>(std::move(data_case)))
{
}


CSPro::Data::Case::!Case()
{
    delete m_case;
}


Case& CSPro::Data::Case::GetNativeReference()
{
    ::Case& data_case = *(*m_case);
    return data_case;
}


System::String^ CSPro::Data::Case::Key::get()
{
    const ::Case& data_case = *(*m_case);
    return gcnew System::String(data_case.GetKey());
}


System::String^ CSPro::Data::Case::KeyForSingleLineDisplay::get()
{
    // turn \n -> ␤
    return Key->Replace('\n', NewlineSubstitutor::UnicodeNL);
}
