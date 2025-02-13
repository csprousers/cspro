#include "stdafx.h"
#include "SortableKeyDatabase.h"
#include <SQLite/SQLiteHelpers.h>


SortableKeyDatabase::SortableKeyDatabase(const SortType sort_type)
    :   m_db(nullptr),
        m_sortType(sort_type),
        m_stmtPut(nullptr),
        m_stmtExists(nullptr),
        m_stmtIterator(nullptr),
        m_putArgumentCounter(0)
{
}


SortableKeyDatabase::~SortableKeyDatabase()
{
    if( m_db != nullptr )
    {
        safe_sqlite3_finalize(m_stmtPut);
        safe_sqlite3_finalize(m_stmtExists);
        safe_sqlite3_finalize(m_stmtIterator);

        sqlite3_close(m_db);
    }
}


void SortableKeyDatabase::AddKeyType(const ContentType content_type, const bool ascending)
{
    ASSERT(IsNumeric(content_type) || IsString(content_type) || IsBinary(content_type));

    // binary data is represented as a number
    m_keyTypes.emplace_back(!IsString(content_type), ascending);
}


bool SortableKeyDatabase::Open()
{
    // open the temporary database
    if( sqlite3_open("", &m_db) == SQLITE_OK )
    {
        std::wstring create_sql_columns;
        std::wstring put_sql_columns;
        std::wstring put_sql_values;
        std::wstring iterator_sql_columns;
        int key_type_index = 0;

        for( const auto& [numeric, ascending] : m_keyTypes )
        {
            SO::AppendFormat(create_sql_columns, _T(", `Col%03d` %s NOT NULL"), key_type_index, numeric ? _T("REAL") : _T("TEXT"));
            SO::AppendFormat(put_sql_columns, _T(", `Col%03d`"), key_type_index);
            put_sql_values.append(_T(", ?"));
            SO::AppendFormat(iterator_sql_columns, _T("%s`Col%03d` %s"), ( key_type_index > 0 ) ? _T(", ") : _T(""), key_type_index, ascending ? _T("ASC") : _T("DESC"));

            ++key_type_index;
        }

        std::wstring create_sql;
        std::wstring put_sql;
        std::wstring iterator_sql;

        if( m_sortType == SortType::CaseSort || m_sortType == SortType::CaseOnly )
        {
            if( m_sortType == SortType::CaseSort )
            {
                create_sql = FormatTextCS2WS(_T("CREATE TABLE `CSSort` (`Key` TEXT NOT NULL, `Position` REAL PRIMARY KEY UNIQUE NOT NULL%s) WITHOUT ROWID;"), create_sql_columns.c_str());
            }

            else
            {
                create_sql = FormatTextCS2WS(_T("CREATE TABLE `CSSort` (`Key` TEXT PRIMARY KEY UNIQUE NOT NULL, `Position` REAL UNIQUE NOT NULL%s) WITHOUT ROWID;"), create_sql_columns.c_str());
            }

            put_sql = FormatTextCS2WS(_T("INSERT INTO `CSSort` (`Key`, `Position`%s) VALUES ( ?, ?%s);"), put_sql_columns.c_str(), put_sql_values.c_str());
            iterator_sql = FormatTextCS2WS(_T("SELECT `Position` FROM `CSSort` ORDER BY %s;"), iterator_sql_columns.c_str());
        }

        else
        {
            create_sql = FormatTextCS2WS(_T("CREATE TABLE `CSSort` (`RecordIndex` INTEGER NOT NULL, `IdRecord` BLOB NOT NULL, `Record` BLOB NOT NULL%s);"), create_sql_columns.c_str());
            put_sql = FormatTextCS2WS(_T("INSERT INTO `CSSort` (`RecordIndex`, `IdRecord`, `Record`%s) VALUES ( ?, ?, ?%s);"), put_sql_columns.c_str(), put_sql_values.c_str());
            iterator_sql = FormatTextCS2WS(_T("SELECT `RecordIndex`, `IdRecord`, `Record` FROM `CSSort` ORDER BY %s;"), iterator_sql_columns.c_str());
        }

        // create the table
        if( sqlite3_exec(m_db, ToUtf8(create_sql), nullptr, nullptr, nullptr) == SQLITE_OK )
        {
            // generate the prepared statements
            if( sqlite3_prepare_v2(m_db, ToUtf8(put_sql), -1, &m_stmtPut, nullptr) == SQLITE_OK  )
            {
                if( m_sortType == SortType::RecordSort ||
                    sqlite3_prepare_v2(m_db, "SELECT 1 FROM `CSSort` WHERE `Key` = ? LIMIT 1;", -1, &m_stmtExists, nullptr) == SQLITE_OK  )
                {
                    if( m_sortType == SortType::CaseOnly ||
                        sqlite3_prepare_v2(m_db, ToUtf8(iterator_sql), -1, &m_stmtIterator, nullptr) == SQLITE_OK  )
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


bool SortableKeyDatabase::CaseExists(const wstring_view key_sv)
{
    ASSERT(m_sortType != SortType::RecordSort);

    sqlite3_reset(m_stmtExists);
    sqlite3_bind_text(m_stmtExists, 1, ToUtf8(key_sv), -1, SQLITE_TRANSIENT);

    return ( sqlite3_step(m_stmtExists) == SQLITE_ROW );
}


void SortableKeyDatabase::InitCaseInfo(const double position_in_repository, const wstring_view key_sv/* = wstring_view()*/)
{
    ASSERT(m_sortType != SortType::RecordSort);

    m_putArgumentCounter = 0;
    sqlite3_reset(m_stmtPut);
    sqlite3_bind_text(m_stmtPut, ++m_putArgumentCounter, ToUtf8(key_sv), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(m_stmtPut, ++m_putArgumentCounter, position_in_repository);
}


void SortableKeyDatabase::InitRecordInfo(const size_t record_index,
                                         const void* id_record_buffer, const size_t id_buffer_size,
                                         const void* record_buffer, const size_t buffer_size)
{
    ASSERT(m_sortType == SortType::RecordSort);

    m_putArgumentCounter = 0;
    sqlite3_reset(m_stmtPut);
    sqlite3_bind_int(m_stmtPut, ++m_putArgumentCounter, static_cast<int>(record_index));
    sqlite3_bind_blob(m_stmtPut, ++m_putArgumentCounter, id_record_buffer, id_buffer_size, nullptr);
    sqlite3_bind_blob(m_stmtPut, ++m_putArgumentCounter, record_buffer, buffer_size, nullptr);
}


void SortableKeyDatabase::AddCaseKeyValue(const double value)
{
    sqlite3_bind_double(m_stmtPut, ++m_putArgumentCounter, value);
}


void SortableKeyDatabase::AddCaseKeyValue(const wstring_view value_sv)
{
    sqlite3_bind_text(m_stmtPut, ++m_putArgumentCounter, ToUtf8(value_sv), -1, SQLITE_TRANSIENT);
}


void SortableKeyDatabase::AddCaseKeyValue(const CaseItem& case_item, const CaseItemIndex& index)
{
    if( case_item.IsTypeNumeric() )
    {
        AddCaseKeyValue(assert_cast<const NumericCaseItem&>(case_item), index);
    }

    else if( case_item.IsTypeString() )
    {
        AddCaseKeyValue(assert_cast<const StringCaseItem&>(case_item), index);
    }

    else if( case_item.IsTypeBinary() )
    {
        AddCaseKeyValue(assert_cast<const BinaryCaseItem&>(case_item), index);
    }

    else
    {
        ASSERT(false);
    }
}


void SortableKeyDatabase::AddCaseKeyValue(const NumericCaseItem& numeric_case_item, const CaseItemIndex& index)
{
    AddCaseKeyValue(numeric_case_item.GetValueForComparison(index));
}


void SortableKeyDatabase::AddCaseKeyValue(const StringCaseItem& string_case_item, const CaseItemIndex& index)
{
    AddCaseKeyValue(string_case_item.GetValue(index));
}


void SortableKeyDatabase::AddCaseKeyValue(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
{
    const std::optional<uint64_t> size = binary_case_item.GetBinaryDataSize_noexcept(index);
    AddCaseKeyValue(size.has_value() ? static_cast<double>(*size) : -1);
}


bool SortableKeyDatabase::AddCase()
{
    return ( sqlite3_step(m_stmtPut) == SQLITE_DONE );
}


bool SortableKeyDatabase::NextPosition(double* position_in_repository)
{
    ASSERT(m_sortType == SortType::CaseSort);

    if( sqlite3_step(m_stmtIterator) == SQLITE_ROW )
    {
        *position_in_repository = sqlite3_column_double(m_stmtIterator, 0);
        return true;
    }

    return false;
}


bool SortableKeyDatabase::NextRecord(size_t* record_index, std::vector<std::byte>* id_binary_buffer, std::vector<std::byte>* record_binary_buffer)
{
    ASSERT(m_sortType == SortType::RecordSort);

    if( sqlite3_step(m_stmtIterator) == SQLITE_ROW )
    {
        *record_index = sqlite3_column_int(m_stmtIterator, 0);

        for( int i = 1; i < 3; ++i )
        {
            std::vector<std::byte>* buffer = ( i == 1 ) ? id_binary_buffer : record_binary_buffer;
            size_t buffer_size = sqlite3_column_bytes(m_stmtIterator, i);
            buffer->resize(buffer_size);
            memcpy(buffer->data(), sqlite3_column_blob(m_stmtIterator, i), buffer_size);
        }

        return true;
    }

    return false;
}
