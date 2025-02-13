#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/SQLiteRepository.h>


class ZDATAO_API EncryptedSQLiteRepository : public SQLiteRepository
{
public:
    constexpr static size_t PasswordHashSize             = 256;
    constexpr static size_t PasswordMinimumLength        = 4;
    constexpr static const size_t PasswordHashIterations = 1024;
    constexpr static const uint8_t FixedSalt[]           = { 0xef, 'I', 0x92, 'N', 0x21, 'S', 0xed, 'O', 0x4b, 'D', 0xa0, 'E', 0x9a, 0xec, 0x81, 0x05 };
    constexpr static std::string_view EncryptionType_sv  = "aes256:";


    EncryptedSQLiteRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type, DeviceId deviceId)
        :   SQLiteRepository(DataRepositoryType::EncryptedSQLite, std::move(case_access), access_type, deviceId)
    {
    }

    static int OpenSQLiteDatabaseFile(const CDataDict* data_dictionary, const ConnectionString& connection_string, sqlite3** ppDb, int flags);

    static std::unique_ptr<CDataDict> GetEmbeddedDictionary(const ConnectionString& connection_string);

protected:
    const TCHAR* GetFileExtension() const override;
    int OpenSQLiteDatabase(const ConnectionString& connection_string, sqlite3** ppDb, int flags) override;
};
