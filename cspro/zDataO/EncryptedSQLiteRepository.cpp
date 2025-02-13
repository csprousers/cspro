#include "stdafx.h"
#include "EncryptedSQLiteRepository.h"
#include "EncryptedSQLiteRepositoryPasswordManager.h"
#include <SQLite/Encryption.h>
#include <zToolsO/Hash.h>
#include <zUtilO/Interapp.h>


const TCHAR* EncryptedSQLiteRepository::GetFileExtension() const
{
    return FileExtensions::Data::EncryptedCSProDB;
}


int EncryptedSQLiteRepository::OpenSQLiteDatabaseFile(const CDataDict* data_dictionary, const ConnectionString& connection_string, sqlite3** ppDb, const int flags)
{
    if( !SqliteEncryption::IsEnabled() )
        throw DataRepositoryException::IOError(SqliteEncryption::NoSeeExceptionMessage);

    const std::wstring& filename = connection_string.GetFilename();
    int open_result = 0;

    const EncryptedSQLiteRepositoryPasswordManager::OpenByPasswordHashCallback file_open_by_password_hash_callback =
        [&](const std::byte* password_hash)
        {
            // to get the SQLite key, prefix the password hash with the encryption type
            std::vector<char> key(EncryptionType_sv.length() + PasswordHashSize);
            memcpy(key.data(), EncryptionType_sv.data(), EncryptionType_sv.length());
            memcpy(key.data() + EncryptionType_sv.length(), password_hash, PasswordHashSize);

            const bool file_exists = PortableFunctions::FileExists(filename);

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
        [&](const std::wstring& password, const EncryptedSQLiteRepositoryPasswordManager::SuccessfulOpenCallback* successful_open_callback)
        {
            // check that the password is long enough
            if( password.length() < PasswordMinimumLength )
            {
                const std::wstring& formatter = MGF::GetMessageText(94301, _T("Passwords must be at least %d characters"));
                throw DataRepositoryException::EncryptionError(FormatText(formatter.c_str(), static_cast<int>(PasswordMinimumLength)));
            }

            const std::string utf8_password = UTF8Convert::WideToUTF8(password);

            // generate the password hash with the fixed salt; it would be ideal to have a randomly
            // generated salt, but because there is no place to store this, we will use a fixed salt
            // even though this does not add cryptographic value
            const std::vector<std::byte> password_hash = Hash::Hash(reinterpret_cast<const std::byte*>(utf8_password.c_str()), utf8_password.length(),
                                                                    reinterpret_cast<const std::byte*>(FixedSalt), _countof(FixedSalt),
                                                                    PasswordHashSize, PasswordHashIterations);

            const bool success = file_open_by_password_hash_callback(password_hash.data());

            if( success && successful_open_callback != nullptr )
            {
                const EncryptedSQLiteRepositoryPasswordManager::GetEmbeddedDictionaryCallback get_embedded_dictionary =
                    [&]() -> std::unique_ptr<CDataDict>
                    {
                        try
                        {
                            return std::unique_ptr<CDataDict>(ReadDictFromDatabase(*ppDb));
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
    const std::wstring* connection_string_password = connection_string.GetProperty(CSProperty::password);

    if( connection_string_password != nullptr )
    {
        if( !file_open_by_password_callback(*connection_string_password, nullptr) )
        {
            const std::wstring& formatter = MGF::GetMessageText(94302, _T("The connection string contained an invalid password for the file %s"));
            throw DataRepositoryException::EncryptionError(FormatText(formatter.c_str(), PortableFunctions::PathGetFilename(filename)));
        }
    }

    else
    {
        EncryptedSQLiteRepositoryPasswordManager password_manager(data_dictionary, filename, file_open_by_password_callback, file_open_by_password_hash_callback);
        password_manager.GetPassword();
    }

    return open_result;
}


int EncryptedSQLiteRepository::EncryptedSQLiteRepository::OpenSQLiteDatabase(const ConnectionString& connection_string, sqlite3** ppDb, const int flags)
{
    return OpenSQLiteDatabaseFile(&m_caseAccess->GetDataDict(), connection_string, ppDb, flags);
}


std::unique_ptr<CDataDict> EncryptedSQLiteRepository::GetEmbeddedDictionary(const ConnectionString& connection_string)
{
    std::unique_ptr<CDataDict> dictionary;
    sqlite3* db = nullptr;

    if( OpenSQLiteDatabaseFile(nullptr, connection_string, &db, SQLITE_OPEN_READONLY) == SQLITE_OK )
    {
        dictionary = ReadDictFromDatabase(db);
        sqlite3_close(db);
    }

    return dictionary;
}
