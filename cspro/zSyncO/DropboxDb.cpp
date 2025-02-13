#include "stdafx.h"
#include "DropboxDb.h"
#include "SyncException.h"
#include <assert.h>
#include <easyloggingwrapper.h>
#include <SQLite/SQLite.h>
#include <zUtilO/Interapp.h>

#define ToUtf8(X)   ( UTF8Convert::WideToUTF8(X).c_str() )

namespace {
    int GetRecordCallback(void* in, int argc, char** argv, char**)
    {
        assert(argc == 3); // ALW: Select 3 fields
        DropboxDb::Record* out = static_cast<DropboxDb::Record*>(in);
        out->setLocalFilePath(argv[0]);
        out->setChecksum(argv[1]);
        out->setEtag(argv[2]);
        return 0;
    }
}

DropboxDb::DropboxDb()
: m_tableName(_T("download"))
, m_db(nullptr)
{
    m_dbName = PortableFunctions::PathAppendToPath(GetAppDataPath(),_T("Dropbox.db"));
}

DropboxDb::~DropboxDb()
{
    sqlite3_close(m_db);
}

void DropboxDb::Init()
{
    try {
        Open();
        CreateTable();
    }
    catch (const SQLiteError&) {
        // ALW - Try deleting the database and creating a new one.
        Close();
        Delete();
        Open();
        CreateTable();
    }
}

void DropboxDb::Open()
{
    CLOG(INFO, "sync") << "Open/create " << UTF8Convert::WideToUTF8(m_dbName) + " in read/write mode.";

    const int rc = sqlite3_open(ToUtf8(m_dbName), &m_db);
    if (SQLITE_OK != rc) {
        CLOG(ERROR, "sync") << sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        throw SQLiteError(rc);
    }
}

void DropboxDb::CreateTable()
{
    CLOG(INFO, "sync") << "Create " << UTF8Convert::WideToUTF8(m_tableName) << " table.";

    const CString sql =
        "CREATE TABLE IF NOT EXISTS " + m_tableName + " ("
        "local_file_path TEXT PRIMARY KEY NOT NULL, "
        "checksum TEXT NOT NULL, "
        "etag TEXT NOT NULL)";
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, ToUtf8(sql), NULL, NULL, &errMsg);
    if (SQLITE_OK != rc) {
        CLOG(ERROR, "sync") << errMsg;
        sqlite3_free(errMsg);
        throw SQLiteError(rc);
    }
}

void DropboxDb::SelectRecord(CString localFilePath, Record &record) const
{
    CLOG(INFO, "sync") << "Select record from " << UTF8Convert::WideToUTF8(m_tableName)
        << " where local_file_path = \"" << UTF8Convert::WideToUTF8(localFilePath) << "\".";

    const TCHAR* formatter = _T("SELECT local_file_path, checksum, etag FROM %s WHERE local_file_path = '%s' LIMIT 1");
    CString sql;
    sql.Format(formatter, (LPCTSTR)m_tableName, (LPCTSTR)localFilePath);
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, ToUtf8(sql), GetRecordCallback, &record, &errMsg);
    if (SQLITE_OK != rc) {
        CLOG(ERROR, "sync") << errMsg;
        sqlite3_free(errMsg);
        throw SQLiteError(rc);
    }
}

void DropboxDb::InsertRecord(CString localFilePath, CString checksum, CString etag)
{
    CLOG(INFO, "sync") << "Insert into " << UTF8Convert::WideToUTF8(m_tableName) << " the values "
        << "(\"" << UTF8Convert::WideToUTF8(localFilePath) << "\", \""
        << UTF8Convert::WideToUTF8(checksum) << "\", \"" << UTF8Convert::WideToUTF8(etag) << "\").";

    const TCHAR* formatter = _T("INSERT INTO %s (local_file_path, checksum, etag) VALUES ('%s', '%s', '%s')");
    CString sql;
    sql.Format(formatter, (LPCTSTR)m_tableName, (LPCTSTR)localFilePath, (LPCTSTR)checksum, (LPCTSTR)etag);
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, ToUtf8(sql), NULL, NULL, &errMsg);
    if (SQLITE_OK != rc) {
        CLOG(ERROR, "sync") << errMsg;
        sqlite3_free(errMsg);
        throw SQLiteError(rc);
    }
}

void DropboxDb::DeleteRecord(CString localFilePath)
{
    CLOG(INFO, "sync") << "Delete record from " << UTF8Convert::WideToUTF8(m_tableName)
        << " where local_file_path = \"" << UTF8Convert::WideToUTF8(localFilePath) << "\".";

    const TCHAR* formatter = _T("DELETE FROM %s WHERE local_file_path = '%s'");
    CString sql;
    sql.Format(formatter, (LPCTSTR)m_tableName, (LPCTSTR)localFilePath);
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, ToUtf8(sql), NULL, NULL, &errMsg);
    if (SQLITE_OK != rc) {
        CLOG(ERROR, "sync") << errMsg;
        sqlite3_free(errMsg);
        throw SQLiteError(rc);
    }
}

void DropboxDb::UpdateField(CString updateColumnName, CString updateValue, CString whereColumnName, CString whereValue)
{
    CLOG(INFO, "sync") << "Update " << UTF8Convert::WideToUTF8(m_tableName)
        << " set " << UTF8Convert::WideToUTF8(updateColumnName) << " = \"" << UTF8Convert::WideToUTF8(updateValue)
        << "\" where " << UTF8Convert::WideToUTF8(whereColumnName) << " = \"" << UTF8Convert::WideToUTF8(whereValue) << "\".";

    const TCHAR* formatter = _T("UPDATE %s SET %s = '%s' WHERE %s = '%s'");
    CString sql;
    sql.Format(formatter, (LPCTSTR)m_tableName, (LPCTSTR)updateColumnName, (LPCTSTR)updateValue, (LPCTSTR)whereColumnName, (LPCTSTR)whereValue);
    char* errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, ToUtf8(sql), NULL, NULL, &errMsg);
    if (SQLITE_OK != rc) {
        CLOG(ERROR, "sync") << errMsg;
        sqlite3_free(errMsg);
        throw SQLiteError(rc);
    }
}

void DropboxDb::Close()
{
    CLOG(INFO, "sync") << "Close database " << UTF8Convert::WideToUTF8(m_dbName) << ".";
    const int rc = sqlite3_close(m_db);
    if (SQLITE_OK != rc) {
        CLOG(ERROR, "sync") << sqlite3_errmsg(m_db);
        throw SQLiteError(rc);
    }
}

void DropboxDb::Delete()
{
    CLOG(INFO, "sync") << "Delete database file " << UTF8Convert::WideToUTF8(m_dbName) << ".";
    const bool success = PortableFunctions::FileDelete(m_dbName);
    if (!success) {
        CLOG(ERROR, "sync") << "Could not delete database file " << UTF8Convert::WideToUTF8(m_dbName) << ".";
        throw SQLiteError(1);
    }
}

CString DropboxDb::Record::getLocalFilePath() const
{
    return m_localFilePath;
}

CString DropboxDb::Record::getChecksum() const
{
    return m_checksum;
}

CString DropboxDb::Record::getEtag() const
{
    return m_etag;
}

void DropboxDb::Record::setLocalFilePath(CString value)
{
    m_localFilePath = value;
}

void DropboxDb::Record::setChecksum(CString value)
{
    m_checksum = value;
}

void DropboxDb::Record::setEtag(CString value)
{
    m_etag = value;
}

bool DropboxDb::Record::isEmpty() const
{
    return m_localFilePath.IsEmpty()
        && m_checksum.IsEmpty()
        && m_etag.IsEmpty();
}
