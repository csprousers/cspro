#include "StdAfx.h"
#include "CSProException.h"


std::string CSProException::CreateUtf8Message(wstring_view message)
{
    return UTF8Convert::WideToUTF8(message);
}


std::wstring CSProException::GetErrorMessage(const std::exception& exception)
{
    return UTF8Convert::UTF8ToWide(exception.what());
}
