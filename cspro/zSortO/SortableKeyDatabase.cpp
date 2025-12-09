#include "stdafx.h"
#include "SortableKeyDatabase.h"


SortableKeyDatabase::SortableKeyDatabase(const SortType sort_type)
    :   m_sortType(sort_type),
        m_putArgumentCounter(0)
{
}


void SortableKeyDatabase::AddKeyType(const ContentType content_type, const bool ascending)
{
    ASSERT(IsNumeric(content_type) || IsString(content_type) || IsBinary(content_type));

    // binary data is represented as a number
    m_keyTypes.emplace_back(!IsString(content_type), ascending);
}


void SortableKeyDatabase::Open()
{
    ASSERT(!m_db.IsOpen());

    try
    {
        // open the temporary database
        m_db.Open("", Sqlite::OpenFlags::ReadWrite | Sqlite::OpenFlags::Create);

        std::string create_sql_columns;
        std::string put_sql_columns;
        std::string put_sql_values;
        std::string iterator_sql_columns;
        int key_type_index = 0;

        for( const auto& [numeric, ascending] : m_keyTypes )
        {
            create_sql_columns.append(FormatText( ", `Col%03d` %s NOT NULL", key_type_index, numeric ? "REAL" : "TEXT"));
            put_sql_columns.append(FormatText(", `Col%03d`", key_type_index));
            put_sql_values.append(", ?");
            iterator_sql_columns.append(FormatText("%s`Col%03d` %s", ( key_type_index > 0 ) ? ", " : "", key_type_index, ascending ? "ASC" : "DESC"));

            ++key_type_index;
        }

        std::string create_sql;
        std::string put_sql;
        std::string iterator_sql;

        if( m_sortType == SortType::CaseSort || m_sortType == SortType::CaseOnly )
        {
            if( m_sortType == SortType::CaseSort )
            {
                create_sql = FormatText("CREATE TABLE `CSSort` (`Key` TEXT NOT NULL, `Position` REAL PRIMARY KEY UNIQUE NOT NULL%s) WITHOUT ROWID;", create_sql_columns.c_str());
            }

            else
            {
                create_sql = FormatText("CREATE TABLE `CSSort` (`Key` TEXT PRIMARY KEY UNIQUE NOT NULL, `Position` REAL UNIQUE NOT NULL%s) WITHOUT ROWID;", create_sql_columns.c_str());
            }

            put_sql = FormatText("INSERT INTO `CSSort` (`Key`, `Position`%s) VALUES ( ?, ?%s);", put_sql_columns.c_str(), put_sql_values.c_str());
            iterator_sql = FormatText("SELECT `Position` FROM `CSSort` ORDER BY %s;", iterator_sql_columns.c_str());
        }

        else
        {
            create_sql = FormatText("CREATE TABLE `CSSort` (`RecordIndex` INTEGER NOT NULL, `IdRecord` BLOB NOT NULL, `Record` BLOB NOT NULL%s);", create_sql_columns.c_str());
            put_sql = FormatText("INSERT INTO `CSSort` (`RecordIndex`, `IdRecord`, `Record`%s) VALUES ( ?, ?, ?%s);", put_sql_columns.c_str(), put_sql_values.c_str());
            iterator_sql = FormatText("SELECT `RecordIndex`, `IdRecord`, `Record` FROM `CSSort` ORDER BY %s;", iterator_sql_columns.c_str());
        }

        // create the table
        m_db.Execute(create_sql);

        // generate the prepared statements
        m_stmtPut = m_db.PrepareStatement(put_sql);

        if( m_sortType != SortType::RecordSort )
            m_stmtExists = m_db.PrepareStatement("SELECT 1 FROM `CSSort` WHERE `Key` = ? LIMIT 1;");

        if( m_sortType != SortType::CaseOnly )
            m_stmtIterator = m_db.PrepareStatement(iterator_sql);
    }

    catch( const CSProException& exception )
    {
        throw CSProException("There was a problem opening the sortable key database: %s", exception.what());
    }
}


bool SortableKeyDatabase::CaseExists(const std::string& key)
{
    ASSERT(m_sortType != SortType::RecordSort);

    m_stmtExists.Reset()
                .Bind(1, key);

    return ( m_stmtExists.Step() == Sqlite::Result::Row );
}


void SortableKeyDatabase::InitCaseInfo(const double position_in_repository, const std::string& key/* = SO::Empty_string*/)
{
    ASSERT(m_sortType != SortType::RecordSort);

    m_putArgumentCounter = 0;

    m_stmtPut.Reset()
             .Bind(++m_putArgumentCounter, key)
             .Bind(++m_putArgumentCounter, position_in_repository);
}


void SortableKeyDatabase::InitRecordInfo(const size_t record_index, const std::vector<std::byte>& id_record_buffer, const std::vector<std::byte>& record_buffer)
{
    ASSERT(m_sortType == SortType::RecordSort);

    m_putArgumentCounter = 0;

    m_stmtPut.Reset()
             .Bind(++m_putArgumentCounter, record_index)
             .BindBlob(++m_putArgumentCounter, id_record_buffer)
             .BindBlob(++m_putArgumentCounter, record_buffer);
}


void SortableKeyDatabase::AddCaseKeyValue(const double value)
{
    m_stmtPut.Bind(++m_putArgumentCounter, value);
}


void SortableKeyDatabase::AddCaseKeyValue(const std::string& value)
{
    m_stmtPut.Bind(++m_putArgumentCounter, value);
}


void SortableKeyDatabase::AddCaseKeyValue(const CaseItem& case_item, const CaseItemIndex& index)
{
    if( IsNumeric(case_item.GetDataType()) )
    {
        AddCaseKeyValue(assert_cast<const NumericCaseItem&>(case_item), index);
    }

    else if( IsString(case_item.GetDataType()) )
    {
        AddCaseKeyValue(assert_cast<const StringCaseItem&>(case_item), index);
    }

    else if( IsBinary(case_item.GetDataType()) )
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
    return ( m_stmtPut.Step() == Sqlite::Result::Done );
}


bool SortableKeyDatabase::NextPosition(double& position_in_repository)
{
    ASSERT(m_sortType == SortType::CaseSort);

    if( m_stmtIterator.Step() != Sqlite::Result::Row )
        return false;

    position_in_repository = m_stmtIterator.GetColumn<double>(0);

    return true;
}


bool SortableKeyDatabase::NextRecord(size_t& record_index, std::vector<std::byte>& id_binary_buffer, std::vector<std::byte>& record_binary_buffer)
{
    ASSERT(m_sortType == SortType::RecordSort);

    if( m_stmtIterator.Step() != Sqlite::Result::Row )
        return false;

    record_index = m_stmtIterator.GetColumn<size_t>(0);
    id_binary_buffer = m_stmtIterator.GetColumn<std::vector<std::byte>>(1);
    record_binary_buffer = m_stmtIterator.GetColumn<std::vector<std::byte>>(2);

    return true;
}
