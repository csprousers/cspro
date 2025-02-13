#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/ICredentialStore.h>


class CLASS_DECL_ZUTILO CredentialStore : public ICredentialStore
{
public:
    void Store(const std::wstring& attribute, const std::wstring& secret_value) override;

    std::wstring Retrieve(const std::wstring& attribute) override;

#ifdef WIN32
    static void ClearAll(const std::function<bool(size_t)>* confirmation_callback = nullptr, const std::wstring& attribute_prefix = _T("CSPro"));
#endif

protected:
    virtual std::wstring PrefixAttribute(const std::wstring& attribute) = 0;
};
