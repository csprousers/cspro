#pragma once


struct ICredentialStore
{
    virtual ~ICredentialStore() { }

    virtual void Store(const std::wstring& attribute, const std::wstring& secret_value) = 0;
    virtual std::wstring Retrieve(const std::wstring& attribute) = 0;
};
