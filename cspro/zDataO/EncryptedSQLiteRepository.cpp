#include "stdafx.h"
#include "EncryptedSQLiteRepository.h"
#include "EncryptedSQLiteRepositoryPasswordManager.h"
#include <zSql/Encryption.h>
#include <zUtilO/Interapp.h>


EncryptedSQLiteRepository::EncryptedSQLiteRepository(std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type, DeviceId device_id)
    :   SQLiteRepository(DataRepositoryType::EncryptedSQLite, std::move(case_access), access_type, std::move(device_id))
{
}


const char* EncryptedSQLiteRepository::GetFileExtension() const
{
    return FileExtensions::Data::EncryptedCSProDB;
}


int EncryptedSQLiteRepository::OpenSQLiteDatabaseFile(const CDataDict* const dictionary, const ConnectionString& connection_string, sqlite3** ppDb, const int flags)
{
    if( !SqliteEncryption::IsEnabled() )
        throw DataRepositoryException::IOError(SqliteEncryption::NoSeeExceptionMessage);

    const std::string& file_path = connection_string.GetFilePath();
    int open_result = 0;

    const EncryptedSQLiteRepositoryPasswordManager::OpenByPasswordHashCallback file_open_by_password_hash_callback =
        [&](const std::byte* const password_hash)
        {
            // to get the SQLite key, prefix the password hash with the encryption type
            std::vector<char> key(EncryptionType_sv.length() + PasswordHashSize);
            memcpy(key.data(), EncryptionType_sv.data(), EncryptionType_sv.length());
            memcpy(key.data() + EncryptionType_sv.length(), password_hash, PasswordHashSize);

            const bool file_exists = PortableFunctions::FileExists(file_path);

            sqlite3* db;
            open_result = SQLiteRepository::OpenSQLiteDatabaseFile(connection_string, &db, flags);

            if( open_result == SQLITE_OK )
            {
                try
                {
                    open_result = SqliteEncryption::sqlite3_key(db, key.data(), key.size());
                }

                catch(...)
                {
                    // SqliteEncryption::sqlite3_key can throw an exception, but we should
                    // never get here if the SEE is not part of this build
                    ASSERT(false);
                    return false;
                }

                // if the file already exists, check that the password is correct with a simple query
                if( file_exists && open_result == SQLITE_OK )
                {
                    sqlite3_stmt* stmt = nullptr;
                    sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, nullptr);
                    const int key_check_result = sqlite3_step(stmt);
                    sqlite3_finalize(stmt);

                    if( key_check_result == SQLITE_NOTADB )
                    {
                        sqlite3_close(db);
                        return false;
                    }
                }
            }

            *ppDb = db;

            return true;
        };

    const EncryptedSQLiteRepositoryPasswordManager::OpenByPasswordCallback file_open_by_password_callback =
        [&](const std::string& password, const EncryptedSQLiteRepositoryPasswordManager::SuccessfulOpenCallback* const successful_open_callback)
        {
            // check that the password is long enough
            if( SO::WideLength(password) < PasswordMinimumLength )
            {
                const SharableString formatter = MGF::GetMessageText(94301, "Passwords must be at least %d characters");
                throw DataRepositoryException::EncryptionError(formatter->c_str(), static_cast<int>(PasswordMinimumLength));
            }

            // generate the password hash with the fixed salt; it would be ideal to have a randomly
            // generated salt, but because there is no place to store this, we will use a fixed salt
            // even though this does not add cryptographic value
            const std::vector<std::byte> password_hash = Hash::Create(
                reinterpret_cast<const std::byte*>(password.c_str()),
                password.length(),
                reinterpret_cast<const std::byte*>(FixedSalt),
                _countof(FixedSalt),
                PasswordHashSize,
                PasswordHashIterations
            );

            const bool success = file_open_by_password_hash_callback(password_hash.data());

            if( success && successful_open_callback != nullptr )
            {
                const EncryptedSQLiteRepositoryPasswordManager::GetEmbeddedDictionaryCallback get_embedded_dictionary =
                    [&]() -> std::unique_ptr<CDataDict>
                    {
                        try
                        {
                            return ReadDictionaryFromDatabase(*ppDb);
                        }
                        catch(...) { }

                        return nullptr;
                    };

                (*successful_open_callback)(password_hash.data(), get_embedded_dictionary);
            }

            return success;
        };

    // check the password specified in the connection string, if available;
    // if not, prompt for a password (or retrieve it from the saved credentials)
    const std::string* const connection_string_password = connection_string.GetProperty(CSProperty::password);

    if( connection_string_password != nullptr )
    {
        if( !file_open_by_password_callback(*connection_string_password, nullptr) )
        {
            const SharableString formatter = MGF::GetMessageText(94302, "The connection string contained an invalid password for the file %s");
            throw DataRepositoryException::EncryptionError(formatter->c_str(), PortableFunctions::PathGetFilename(file_path).c_str());
        }
    }

    else
    {
        EncryptedSQLiteRepositoryPasswordManager password_manager(dictionary, file_path, file_open_by_password_callback, file_open_by_password_hash_callback);
        password_manager.GetPassword();
    }

    return open_result;
}


int EncryptedSQLiteRepository::EncryptedSQLiteRepository::OpenSQLiteDatabase(const ConnectionString& connection_string, sqlite3** ppDb, const int flags)
{
    const CDataDict& dictionary = m_caseAccess->GetDataDict();
    return OpenSQLiteDatabaseFile(&dictionary, connection_string, ppDb, flags);
}


std::unique_ptr<CDataDict> EncryptedSQLiteRepository::GetEmbeddedDictionary(const ConnectionString& connection_string)
{
    std::unique_ptr<CDataDict> dictionary;
    sqlite3* db = nullptr;

    if( OpenSQLiteDatabaseFile(nullptr, connection_string, &db, SQLITE_OPEN_READONLY) == SQLITE_OK )
    {
        dictionary = ReadDictionaryFromDatabase(db);
        sqlite3_close(db);
    }

    return dictionary;
}
