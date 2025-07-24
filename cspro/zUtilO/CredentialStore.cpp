#include "StdAfx.h"
#include "CredentialStore.h"
#include <zPlatformO/PlatformInterface.h>


#ifdef WIN32

#include <wincred.h>


std::string CredentialStore::PrefixAttribute(const std::string_view attribute_sv)
{
    return std::string(attribute_sv);
}


void CredentialStore::Store(const std::string_view attribute_sv, const std::string& secret_value)
{
    std::wstring wide_prefixed_attribute = TC::ToWide(PrefixAttribute(attribute_sv));
    std::wstring wide_secret_value = TC::ToWide(secret_value);

    CREDENTIAL cred = { 0 };
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = wide_prefixed_attribute.data();
    cred.CredentialBlobSize = secret_value.size() * sizeof(wchar_t);
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(wide_secret_value.data());
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = nullptr;

    CredWrite(&cred, 0);
}


std::string CredentialStore::Retrieve(const std::string_view attribute_sv)
{
    const std::wstring wide_prefixed_attribute = TC::ToWide(PrefixAttribute(attribute_sv));

    PCREDENTIAL credential;

    if( CredRead(wide_prefixed_attribute.c_str(), CRED_TYPE_GENERIC, 0, &credential) )
    {
        std::string secret_value = TC::ToUtf8(reinterpret_cast<const wchar_t*>(credential->CredentialBlob), credential->CredentialBlobSize / sizeof(wchar_t));
        CredFree(credential);
        return secret_value;
    }

    return std::string();
}


#else

void CredentialStore::Store(const std::string_view attribute_sv, const std::string& secret_value)
{
    const std::string prefixed_attribute = PrefixAttribute(attribute_sv);

    PlatformInterface::GetInstance()->GetApplicationInterface()->StoreCredential(prefixed_attribute, secret_value);
}

std::string CredentialStore::Retrieve(const std::string_view attribute_sv)
{
    const std::string prefixed_attribute = PrefixAttribute(attribute_sv);

    return PlatformInterface::GetInstance()->GetApplicationInterface()->RetrieveCredential(prefixed_attribute);
}

#endif
