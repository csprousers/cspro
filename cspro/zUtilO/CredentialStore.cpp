#include "StdAfx.h"
#include "CredentialStore.h"
#include <zPlatformO/PlatformInterface.h>


#ifdef WIN32
#include <wincred.h>

void CredentialStore::Store(const std::wstring& attribute, const std::wstring& secret_value)
{
    std::wstring prefixed_attribute = PrefixAttribute(attribute);

    CREDENTIAL cred = { 0 };
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<TCHAR*>(prefixed_attribute.c_str());
    cred.CredentialBlobSize = secret_value.size() * sizeof(TCHAR);
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<TCHAR*>(secret_value.c_str()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = NULL;

    CredWrite(&cred, 0);
}


std::wstring CredentialStore::Retrieve(const std::wstring& attribute)
{
    std::wstring prefixed_attribute = PrefixAttribute(attribute);

    PCREDENTIAL credential;

    if( CredRead(prefixed_attribute.c_str(), CRED_TYPE_GENERIC, 0, &credential) )
    {
        std::wstring secret_value(reinterpret_cast<const TCHAR*>(credential->CredentialBlob), credential->CredentialBlobSize / sizeof(TCHAR));
        CredFree(credential);
        return secret_value;
    }

    return std::wstring();
}


void CredentialStore::ClearAll(const std::function<bool(size_t)>* confirmation_callback/* = nullptr*/, const std::wstring& attribute_prefix/* = _T("CSPro")*/)
{
    DWORD number_credentials = 0;
    PCREDENTIAL* credentials = nullptr;

    CredEnumerate(std::wstring(attribute_prefix + _T("*")).c_str(), 0, &number_credentials, &credentials);

    if( confirmation_callback == nullptr || (*confirmation_callback)(static_cast<size_t>(number_credentials)) )
    {
        for( DWORD i = 0; i < number_credentials; ++i )
            CredDelete(credentials[i]->TargetName, CRED_TYPE_GENERIC, 0);
    }

    CredFree(credentials);
}


#else

void CredentialStore::Store(const std::wstring& attribute, const std::wstring& secret_value)
{
    std::wstring prefixed_attribute = PrefixAttribute(attribute);

    PlatformInterface::GetInstance()->GetApplicationInterface()->StoreCredential(prefixed_attribute, secret_value);
}

std::wstring CredentialStore::Retrieve(const std::wstring& attribute)
{
    std::wstring prefixed_attribute = PrefixAttribute(attribute);

    return PlatformInterface::GetInstance()->GetApplicationInterface()->RetrieveCredential(prefixed_attribute);
}

#endif
