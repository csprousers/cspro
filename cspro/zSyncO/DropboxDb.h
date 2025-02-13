#pragma once

struct sqlite3;

class DropboxDb
{
public:

    class Record;

    DropboxDb();
    ~DropboxDb();

    void Init();
    void SelectRecord(CString localFilePath, Record& record) const;
    void InsertRecord(CString localFilePath, CString checksum, CString etag);
    void DeleteRecord(CString localFilePath);
    void UpdateField(CString updateColumnName, CString updateValue, CString whereColumnName, CString whereValue);

    class Record
    {
    public:

        CString getLocalFilePath() const;
        CString getChecksum() const;
        CString getEtag() const;
        void setLocalFilePath(CString value);
        void setChecksum(CString value);
        void setEtag(CString value);

        bool isEmpty() const;

    private:

        CString m_localFilePath;
        CString m_checksum;
        CString m_etag;
    };

private:

    DropboxDb(const DropboxDb&);
    DropboxDb& operator=(const DropboxDb&);
    void Open();
    void CreateTable();
    void Close();
    void Delete();

    std::wstring m_dbName;
    const CString m_tableName;
    sqlite3* m_db;
};
