#include "StdAfx.h"
#include "CredentialStore.h"


// --------------------------------------------------------------------------
// CredentialStore
// --------------------------------------------------------------------------

std::string CredentialStore::PrefixAttribute(const std::string_view attribute_sv)
{
    return std::string(attribute_sv);
}


#ifdef WIN32

#include <wincred.h>


void CredentialStore::Store(const std::string_view attribute_sv, const std::string& secret_value)
{
    const std::wstring wide_prefixed_attribute = TC::ToWide(PrefixAttribute(attribute_sv));

    CREDENTIAL cred { 0 };
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<wchar_t*>(wide_prefixed_attribute.data());
    cred.CredentialBlobSize = uint32_cast(secret_value.length());
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(secret_value.data()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = nullptr;

    VERIFY(CredWrite(&cred, 0));
}


std::string CredentialStore::Retrieve(const std::string_view attribute_sv)
{
    const std::wstring wide_prefixed_attribute = TC::ToWide(PrefixAttribute(attribute_sv));

    PCREDENTIAL credential;

    if( CredRead(wide_prefixed_attribute.c_str(), CRED_TYPE_GENERIC, 0, &credential) )
    {
        std::string secret_value = ParseCredentialBlob(*credential);
        CredFree(credential);
        return secret_value;
    }

    return std::string();
}


template<typename CredentialT>
std::string CredentialStore::ParseCredentialBlob(const CredentialT& credential)
{
    // convert credentials stored prior to CSPro 8.1, which were stored as wide characters
    if( credential.CredentialBlobSize > 1 &&
        credential.CredentialBlob[1] == '\0' )
    {
        return TC::ToUtf8(reinterpret_cast<const wchar_t*>(credential.CredentialBlob),
                          credential.CredentialBlobSize / sizeof(wchar_t));
    }

    // CSPro 8.1+ credentials are stored in UTF-8
    return std::string(reinterpret_cast<const char*>(credential.CredentialBlob),
                       credential.CredentialBlobSize);
}

template CLASS_DECL_ZUTILO std::string CredentialStore::ParseCredentialBlob(const CREDENTIAL& credential);


#else

#include <zPlatformO/PlatformInterface.h>


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



// --------------------------------------------------------------------------
// DefinedPrefixCredentialStore
// --------------------------------------------------------------------------

DefinedPrefixCredentialStore::DefinedPrefixCredentialStore(std::string prefix)
    :   m_prefix(std::move(prefix))
{
}


std::string DefinedPrefixCredentialStore::PrefixAttribute(const std::string_view attribute_sv)
{
    return SO::Concatenate(m_prefix, attribute_sv);
}
