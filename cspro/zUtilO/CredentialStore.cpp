#include "StdAfx.h"
#include "CredentialStore.h"
#include <zPlatformO/PlatformInterface.h>


#ifdef WIN32

#include <wincred.h>


void CredentialStore::Store(const std::string& attribute, const std::string& secret_value)
{
    std::wstring wide_prefixed_attribute = TC::ToWide(PrefixAttribute(attribute));
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


std::string CredentialStore::Retrieve(const std::string& attribute)
{
    const std::wstring wide_prefixed_attribute = TC::ToWide(PrefixAttribute(attribute));

    PCREDENTIAL credential;

    if( CredRead(wide_prefixed_attribute.c_str(), CRED_TYPE_GENERIC, 0, &credential) )
    {
        std::string secret_value = TC::ToUtf8(reinterpret_cast<const wchar_t*>(credential->CredentialBlob), credential->CredentialBlobSize / sizeof(wchar_t));
        CredFree(credential);
        return secret_value;
    }

    return std::string();
}


void CredentialStore::ClearAll(const std::function<bool(size_t)>* confirmation_callback/* = nullptr*/, const std::string& attribute_prefix/* = "CSPro"*/)
{
    DWORD number_credentials = 0;
    PCREDENTIAL* credentials = nullptr;

    CredEnumerate(std::wstring(TC::ToWide(attribute_prefix) + L"*").c_str(), 0, &number_credentials, &credentials);

    if( confirmation_callback == nullptr || (*confirmation_callback)(static_cast<size_t>(number_credentials)) )
    {
        for( DWORD i = 0; i < number_credentials; ++i )
            CredDelete(credentials[i]->TargetName, CRED_TYPE_GENERIC, 0);
    }

    CredFree(credentials);
}


#else

void CredentialStore::Store(const std::string& attribute, const std::string& secret_value)
{
    const std::string prefixed_attribute = PrefixAttribute(attribute);

    PlatformInterface::GetInstance()->GetApplicationInterface()->StoreCredential(prefixed_attribute, secret_value);
}

std::string CredentialStore::Retrieve(const std::string& attribute)
{
    const std::string prefixed_attribute = PrefixAttribute(attribute);

    return PlatformInterface::GetInstance()->GetApplicationInterface()->RetrieveCredential(prefixed_attribute);
}

#endif
