#pragma once

#include <zUtilO/CredentialStore.h>


class SyncCredentialStore : public CredentialStore
{
protected:
    std::wstring PrefixAttribute(const std::wstring& attribute) override
    {
        return _T("CSPro_sync_") + attribute;
    }
};
