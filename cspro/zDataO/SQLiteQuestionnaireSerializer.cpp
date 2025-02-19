#include "stdafx.h"
#include "SQLiteQuestionnaireSerializer.h"
#include "SQLiteBinaryContentReader.h"
#include "SQLiteDictionarySchemaGenerator.h"
#include "SQLiteErrorWithMessage.h"
#include <sstream>


SQLiteQuestionnaireSerializer::SQLiteQuestionnaireSerializer(UniqueId repository_id, sqlite3* const db)
    :   m_repositoryId(std::move(repository_id)),
        m_db(db),
        m_delete_statement(nullptr)
{
}


SQLiteQuestionnaireSerializer::~SQLiteQuestionnaireSerializer()
{
    ClearPreparedStatements();
}


void SQLiteQuestionnaireSerializer::ReadQuestionnaire(Case& data_case) const
{
    if (data_case.GetCaseConstructionReporter() != nullptr)
        data_case.GetCaseConstructionReporter()->IncrementCaseLevelCount(0);

    CaseLevel& level = data_case.GetRootCaseLevel();

    SQLiteStatement select_statement(m_read_statements->m_id_record);
    select_statement.Bind(1, data_case.GetUuid());
    int query_result = select_statement.Step();
    if (query_result != SQLITE_ROW)
        throw SQLiteErrorWithMessage(m_db);

    int64_t level_pk = select_statement.GetColumn<int64_t>(0);

    const int start_column = 1;
    CaseItemIndex index = level.GetIdCaseRecord().GetCaseItemIndex();
    ReadRecordItems(level.GetIdCaseRecord(), index, select_statement, start_column);

    ASSERT(select_statement.Step() == SQLITE_DONE); // level zero should only have one node

    ReadLevelRecords(level, level_pk, m_read_statements->m_records);

    if (level.GetCaseLevelMetadata().GetChildCaseLevelMetadata() != nullptr)
        ReadChildLevel(level, level_pk, *m_read_statements->m_child_level);
}


void SQLiteQuestionnaireSerializer::WriteQuestionnaire(const Case& data_case, int64_t revision) const
{
    DeleteCase(data_case.GetUuid());
    WriteLevel(data_case.GetRootCaseLevel(), UTF8_TODO::GetWide(data_case.GetUuid()), {}, *m_write_statements, revision);
}


void SQLiteQuestionnaireSerializer::SetCaseAccess(std::shared_ptr<const CaseAccess> case_access, const bool read_only)
{
    m_caseAccess = std::move(case_access);
    ASSERT(m_caseAccess != nullptr);

    ClearPreparedStatements();
    CreateReadStatements();

    if( !read_only )
    {
        CreateWriteStatements();
        CreateDeleteStatement();
    }

    if( m_caseAccess->GetCaseMetadata().UsesBinaryData() )
        m_binaryItemSerializer = std::make_unique<SQLiteBinaryItemSerializer>(m_repositoryId, m_db);
}


int64_t SQLiteQuestionnaireSerializer::WriteLevel(const CaseLevel& level, const std::wstring& parent_level_id, std::optional<int> level_occ, const PreparedStatements& prepared_statements, int64_t revision) const
{
    const int64_t level_pk = WriteRecord(level.GetIdCaseRecord(), prepared_statements.m_id_record, parent_level_id, revision, level_occ);
    const std::wstring level_id = std::to_wstring(level_pk);

    for (size_t record_number = 0; record_number < level.GetNumberCaseRecords(); ++record_number) {
        const CaseRecord& record = level.GetCaseRecord(record_number);
        WriteRecord(record, prepared_statements.m_records.at(record_number), level_id, revision);
    }

    for (size_t level_index = 0; level_index < level.GetNumberChildCaseLevels(); ++level_index)
    {
        const CaseLevel& child_level = level.GetChildCaseLevel(level_index);
        WriteLevel(child_level, level_id, level_index + 1, *prepared_statements.m_child_level, revision);
    }

    return level_pk;
}


int64_t SQLiteQuestionnaireSerializer::WriteRecord(const CaseRecord& record, sqlite3_stmt* prepared_statement, const std::wstring& level_id, int64_t revision, std::optional<int> level_occ) const
{
    SQLiteStatement insert_statement(prepared_statement);
    insert_statement.Bind(1, level_id);

    for (CaseItemIndex index = record.GetCaseItemIndex(); index.GetRecordOccurrence() < record.GetNumberOccurrences(); index.IncrementRecordOccurrence())
    {
        int column_number = 2;
        if (record.GetCaseRecordMetadata().GetDictRecord().GetMaxRecs() > 1) {
            insert_statement.Bind(column_number, index.GetRecordOccurrence() + 1);
            ++column_number;
        }
        else if (level_occ) {
            // For id-item records on child level nodes add the level occurrence number
            insert_statement.Bind(column_number, *level_occ);
            ++column_number;
        }

        for (const CaseItem* item : record.GetCaseItems()) {
            for (index.SetItemSubitemOccurrence(*item, 0); index.GetItemSubitemOccurrence(*item) < item->GetTotalNumberItemSubitemOccurrences(); index.IncrementItemSubitemOccurrence(*item)) {
                BindCaseItem(insert_statement, column_number, *item, index, revision);
                ++column_number;
                if (IsBinary(item->GetDataType()))
                    ++column_number;
            }
        }

        if (insert_statement.Step() != SQLITE_DONE) {
            throw SQLiteErrorWithMessage(m_db);
        }

        if (index.GetRecordOccurrence() < record.GetNumberOccurrences())
            insert_statement.Reset();
    }

    return sqlite3_last_insert_rowid(m_db);
}


void SQLiteQuestionnaireSerializer::BindCaseItem(SQLiteStatement& statement, const int param_number, const CaseItem& case_item, const CaseItemIndex& index, const int64_t revision) const
{
    if( IsNumeric(case_item.GetDataType()) )
    {
        const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(case_item);
        const double value = numeric_case_item.GetValueForOutput(index);

        if( value == NOTAPPL )
        {
            statement.BindNull(param_number);
        }

        else
        {
            statement.Bind(param_number, value);
        }
    }

    else if( IsString(case_item.GetDataType()) )
    {
        const StringCaseItem& string_case_item = assert_cast<const StringCaseItem&>(case_item);
        statement.Bind(param_number, string_case_item.GetValue(index));
    }

    else if( IsBinary(case_item.GetDataType()) )
    {
        const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(case_item);
        BindBinaryCaseItem(statement, param_number, param_number + 1, binary_case_item, index, revision);
    }

    else
    {
        ASSERT(false);
    }
}


void SQLiteQuestionnaireSerializer::BindBinaryCaseItem(SQLiteStatement& statement, const int signature_param_number, const int metadata_param_number,
                                                       const BinaryCaseItem& binary_case_item, const CaseItemIndex& index, const int64_t revision) const
{
    ASSERT(m_binaryItemSerializer != nullptr);

    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

    // if the binary item is not defined, bind nulls
    if( !binary_data_accessor.IsDefined() )
    {
        statement.BindNull(signature_param_number);
        statement.BindNull(metadata_param_number);
    }

    // otherwise insert the content and bind the signature and metadata
    else
    {
        const std::string signature = m_binaryItemSerializer->InsertContent(binary_data_accessor, index.GetCase().GetUuid(), revision);
        ASSERT(!signature.empty());

        statement.Bind(signature_param_number, signature);
        statement.Bind(metadata_param_number, Json::ToJson(binary_data_accessor.GetBinaryDataMetadata()));
    }
}


void SQLiteQuestionnaireSerializer::ReadLevelRecords(CaseLevel& level, int64_t level_pk, const std::vector<sqlite3_stmt*>& read_statements) const
{
    for (size_t record_number = 0; record_number < level.GetNumberCaseRecords(); ++record_number) {
        CaseRecord& record = level.GetCaseRecord(record_number);
        ReadRecord(record, level_pk, read_statements.at(record_number));
    }
}


void SQLiteQuestionnaireSerializer::ReadChildLevel(CaseLevel& parent_level, int64_t parent_level_pk, const PreparedStatements& prepared_statements) const
{
    SQLiteStatement select_statement(prepared_statements.m_id_record);
    std::string parent_level_id = std::to_string(parent_level_pk);
    select_statement.Bind(1, parent_level_id);
    int query_result;
    while ((query_result = select_statement.Step()) == SQLITE_ROW) {

        int64_t level_pk = select_statement.GetColumn<int64_t>(0);

        CaseLevel& level = parent_level.AddChildCaseLevel();

        if (level.GetCase().GetCaseConstructionReporter() != nullptr)
            level.GetCase().GetCaseConstructionReporter()->IncrementCaseLevelCount(level.GetCaseLevelMetadata().GetDictLevel().GetLevelNumber());

        const int start_column = 1;
        CaseItemIndex index = level.GetIdCaseRecord().GetCaseItemIndex();
        ReadRecordItems(level.GetIdCaseRecord(), index, select_statement, start_column);

        ReadLevelRecords(level, level_pk, prepared_statements.m_records);

        if (level.GetCaseLevelMetadata().GetChildCaseLevelMetadata() != nullptr)
            ReadChildLevel(level, level_pk, *prepared_statements.m_child_level);
    }

    if (query_result != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);
}


namespace
{
    inline bool IncrementRecordOccurrences(CaseRecord& record, Case& data_case, CaseConstructionReporter* error_reporter)
    {
        if( error_reporter != nullptr )
            error_reporter->IncrementRecordCount();

        size_t num_occs = record.GetNumberOccurrences() + 1;

        if( num_occs <= record.GetCaseRecordMetadata().GetDictRecord().GetMaxRecs() )
        {
            record.SetNumberOccurrences(num_occs);
            return true;
        }

        else if( error_reporter != nullptr )
        {
            error_reporter->TooManyRecordOccurrences(data_case,
                                                     record.GetCaseRecordMetadata().GetDictRecord().GetName(),
                                                     record.GetCaseRecordMetadata().GetDictRecord().GetMaxRecs());
        }

        return false;
    }
}


void SQLiteQuestionnaireSerializer::ReadRecord(CaseRecord& record, int64_t level_pk, sqlite3_stmt* prepared_statement) const
{
    SQLiteStatement select_statement(prepared_statement);
    select_statement.Bind(1, level_pk);

    Case& data_case = record.GetCaseLevel().GetCase();
    CaseConstructionReporter* error_reporter = data_case.GetCaseConstructionReporter();
    ASSERT(record.GetNumberOccurrences() == 0);

    if (record.GetNumberCaseItems() == 0) {
        // No items so just get record count for totocc
        if (select_statement.Step() != SQLITE_ROW)
            throw SQLiteErrorWithMessage(m_db);

        int num_occs = select_statement.GetColumn<int>(0);

        while( num_occs-- > 0 ) {
            IncrementRecordOccurrences(record, data_case, error_reporter);
        }
    } else {

        size_t record_occurrence = 0;
        int query_result;

        while ((query_result = select_statement.Step()) == SQLITE_ROW) {

            if (IncrementRecordOccurrences(record, data_case, error_reporter)) {
                CaseItemIndex index = record.GetCaseItemIndex(record_occurrence++);

                const int column_number = 0;
                ReadRecordItems(record, index, select_statement, column_number);
            }
        }

        if (query_result != SQLITE_DONE)
            throw SQLiteErrorWithMessage(m_db);
    }
}


void SQLiteQuestionnaireSerializer::ReadRecordItems(CaseRecord& case_record, CaseItemIndex& index, SQLiteStatement& select_statement, const int start_column) const
{
    int column_number = start_column;

    for( const CaseItem* const case_item : case_record.GetCaseItems() )
    {
        for( size_t occurrence = 0; occurrence < case_item->GetTotalNumberItemSubitemOccurrences(); ++occurrence )
        {
            index.SetItemSubitemOccurrence(*case_item, occurrence);
            SetCaseItemValueStatement(*case_item, index, select_statement, column_number);

            column_number += IsBinary(case_item->GetDataType()) ? 2 : 1;
        }
    }
}


void SQLiteQuestionnaireSerializer::SetCaseItemValueStatement(const CaseItem& case_item, CaseItemIndex& index, SQLiteStatement& statement, const int column_number) const
{
    const int column_type = statement.GetColumnType(column_number);

    if( column_type == SQLITE_NULL )
        return;

    if( IsNumeric(case_item.GetDataType()) )
    {
        const NumericCaseItem& numeric_case_item = assert_cast<const NumericCaseItem&>(case_item);

        numeric_case_item.SetValueFromInput(index, ( column_type == SQLITE_FLOAT || column_type == SQLITE_INTEGER ) ? statement.GetColumn<double>(column_number) :
                                                                                                                      StringToNumber(statement.GetColumn<std::wstring>(column_number)));
    }

    else if( IsString(case_item.GetDataType()) )
    {
        const StringCaseItem& string_case_item = assert_cast<const StringCaseItem&>(case_item);

        string_case_item.SetValue(index, statement.GetColumn<std::string>(column_number));
    }

    else if( IsBinary(case_item.GetDataType()) )
    {
        const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(case_item);

        ASSERT(column_type == SQLITE_TEXT);
        SetBinaryCaseItemFromStatement(binary_case_item, index, statement, column_number, column_number + 1);
    }

    else
    {
        ASSERT(false);
    }
}


void SQLiteQuestionnaireSerializer::SetBinaryCaseItemFromStatement(const BinaryCaseItem& binary_case_item, CaseItemIndex& index, SQLiteStatement& statement,
                                                                   const int signature_column_number, const int metadata_column_number) const
{
    ASSERT(m_binaryItemSerializer != nullptr);

    std::string signature = statement.GetColumn<std::string>(signature_column_number);
    ASSERT(BinaryDataAccessor::IsValidSignature(signature));

    const std::string metadata_json = statement.GetColumn<std::string>(metadata_column_number);
    std::optional<BinaryDataMetadata> binary_data_metadata;

    try
    {
        binary_data_metadata = Json::Parse(metadata_json).Get<BinaryDataMetadata>();
    }

    catch( const JsonParseException& )
    {
        // ignore JSON errors
        binary_data_metadata.emplace();
    }

    // use a binary content reader to lazy load content
    binary_case_item.SetValue(index, std::move(*binary_data_metadata), std::move(signature),
                              std::make_unique<SQLiteBinaryContentReader>(m_repositoryId, m_binaryItemSerializer.get()));
}


void SQLiteQuestionnaireSerializer::ClearPreparedStatements()
{
    sqlite3_finalize(m_delete_statement);
}


void SQLiteQuestionnaireSerializer::CreateWriteStatements()
{
    const std::vector<CaseLevelMetadata>& case_levels_metadata = m_caseAccess->GetCaseMetadata().GetCaseLevelsMetadata();
    std::string parent_level_table_name = "case";
    m_write_statements = CreateWriteStatementsForLevel(case_levels_metadata.front(), parent_level_table_name);
}


void SQLiteQuestionnaireSerializer::CreateReadStatements()
{
    const std::vector<CaseLevelMetadata>& case_levels_metadata = m_caseAccess->GetCaseMetadata().GetCaseLevelsMetadata();
    std::string parent_level_table_name = "case";
    m_read_statements = CreateReadStatementsForLevel(case_levels_metadata.front(), parent_level_table_name);
}


void SQLiteQuestionnaireSerializer::CreateDeleteStatement()
{
    constexpr const char* sql = "DELETE FROM `level-1` WHERE `case-id` = ?";
    if (sqlite3_prepare_v2(m_db, sql, -1, &m_delete_statement, NULL) != SQLITE_OK)
        throw SQLiteErrorWithMessage(m_db);
}


std::unique_ptr<SQLiteQuestionnaireSerializer::PreparedStatements>
SQLiteQuestionnaireSerializer::CreateWriteStatementsForLevel(const CaseLevelMetadata& level_metadata, const std::string& parent_level_table_name)
{
    auto statements_for_level = std::make_unique<PreparedStatements>();

    std::string level_name = "level-" + std::to_string(level_metadata.GetDictLevel().GetLevelNumber() + 1);
    statements_for_level->m_id_record = CreateWriteStatementsForRecord(level_metadata.GetIdCaseRecordMetadata(), level_name, parent_level_table_name);

    for (const CaseRecordMetadata& record_metadata : level_metadata.GetCaseRecordsMetadata()) {
        const std::string record_table_name = SO::ToLower(record_metadata.GetDictRecord().GetName());
        statements_for_level->m_records.emplace_back(CreateWriteStatementsForRecord(record_metadata, record_table_name, level_name));
    }

    const CaseLevelMetadata* child_level = level_metadata.GetChildCaseLevelMetadata();
    if (child_level)
        statements_for_level->m_child_level = CreateWriteStatementsForLevel(*child_level, level_name);

    return statements_for_level;
}


sqlite3_stmt* SQLiteQuestionnaireSerializer::CreateWriteStatementsForRecord(const CaseRecordMetadata& record_metadata,
    const std::string& record_table_name, const std::string& parent_level_table_name)
{
    size_t num_columns = 1;
    std::ostringstream ss;
    ss << "INSERT INTO `" << record_table_name << "` (`" << parent_level_table_name << "-id`";
    bool isChildLevelIdRecord = &record_metadata == &record_metadata.GetCaseLevelMetadata().GetIdCaseRecordMetadata() &&
                                record_metadata.GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() > 0;
    if (record_metadata.GetDictRecord().GetMaxRecs() > 1 || isChildLevelIdRecord) {
        ss << ",occ";
        ++num_columns;
    }

    for (const CaseItem* item : record_metadata.GetCaseItems()) {
        const CDictItem& dict_item = item->GetDictItem();
        if (dict_item.GetItemSubitemOccurs() > 1) {
            if (IsBinary(dict_item))
            {
                //  need to label columns <item-name>-signature and <item-name>-metadata for each occrrence.
                for (size_t occ = 1; occ <= dict_item.GetItemSubitemOccurs(); ++occ)
                {
                    ss << ",`" << SO::ToLower(dict_item.GetName()) << SQLiteDictionarySchemaGenerator::BinaryColumnPostfixSignature << "(" << occ << ")`";
                    ss << ",`" << SO::ToLower(dict_item.GetName()) << SQLiteDictionarySchemaGenerator::BinaryColumnPostfixMetadata << "(" << occ << ")`";
                    num_columns += 2;
                }
            }
            else
            {
                for (size_t occ = 1; occ <= dict_item.GetItemSubitemOccurs(); ++occ) {
                    ss << ",`" << SO::ToLower(dict_item.GetName()) << "(" << occ << ")`";
                    ++num_columns;
                }
            }
        }
        else
        {
            //  need to label columns <item-name>-signature and <item-name>-metadata.
            if (IsBinary(dict_item))
            {
                ss << ",`" << SO::ToLower(dict_item.GetName()) << SQLiteDictionarySchemaGenerator::BinaryColumnPostfixSignature << "`";
                ss << ",`" << SO::ToLower(dict_item.GetName()) << SQLiteDictionarySchemaGenerator::BinaryColumnPostfixMetadata << "`";
                num_columns += 2;
            }
            else
            {
                ss << ",`" << SO::ToLower(dict_item.GetName()) << "`";
                ++num_columns;
            }
        }
    }
    ss << ") VALUES(?";
    for (size_t i = 0; i < num_columns - 1; ++i)
        ss << ",?";
    ss << ")";

    std::string sql = ss.str();
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
        throw SQLiteErrorWithMessage(m_db);
    return stmt;
}


std::unique_ptr<SQLiteQuestionnaireSerializer::PreparedStatements>
SQLiteQuestionnaireSerializer::CreateReadStatementsForLevel(const CaseLevelMetadata& level_metadata, const std::string& parent_level_table_name)
{
    auto statements_for_level = std::make_unique<PreparedStatements>();

    std::string level_name = "level-" + std::to_string(level_metadata.GetDictLevel().GetLevelNumber() + 1);
    statements_for_level->m_id_record = CreateReadStatementsForRecord(level_metadata.GetIdCaseRecordMetadata(), level_name, parent_level_table_name, true);

    for (const CaseRecordMetadata& record_metadata : level_metadata.GetCaseRecordsMetadata()) {
        const std::string record_table_name = SO::ToLower(record_metadata.GetDictRecord().GetName());
        statements_for_level->m_records.emplace_back(CreateReadStatementsForRecord(record_metadata, record_table_name, level_name, false));
    }

    const CaseLevelMetadata* child_level = level_metadata.GetChildCaseLevelMetadata();
    if (child_level)
        statements_for_level->m_child_level = CreateReadStatementsForLevel(*child_level, level_name);

    return statements_for_level;
}


sqlite3_stmt* SQLiteQuestionnaireSerializer::CreateReadStatementsForRecord(const CaseRecordMetadata& record_metadata,
    const std::string& record_table_name, const std::string& parent_level_table_name, bool include_id)
{
    std::ostringstream ss;

    if (!include_id && record_metadata.GetCaseItems().empty()) {
        // Even though there are no variables to load we still need to get num occs so
        // create a count query to do that

        ss << "SELECT COUNT(*) FROM `" << record_table_name <<
            "` WHERE `" << parent_level_table_name << "-id` = ?";
    }
    else {
        std::vector<std::string> column_names;
        if (include_id)
            column_names.emplace_back(record_table_name + "-id");
        for (const CaseItem* item : record_metadata.GetCaseItems()) {
            const CDictItem& dict_item = item->GetDictItem();
            if (dict_item.GetItemSubitemOccurs() > 1) {
                if (IsBinary(dict_item))
                {
                    //  need to label columns <item-name>-signature and <item-name>-metadata for each occrrence.
                    for (size_t item_occurrence = 1; item_occurrence <= dict_item.GetItemSubitemOccurs(); ++item_occurrence)
                    {
                        column_names.emplace_back(SO::ToLower(dict_item.GetName()) + SQLiteDictionarySchemaGenerator::BinaryColumnPostfixSignature + '(' + std::to_string(item_occurrence) + ')');
                        column_names.emplace_back(SO::ToLower(dict_item.GetName()) + SQLiteDictionarySchemaGenerator::BinaryColumnPostfixMetadata + '(' + std::to_string(item_occurrence) + ')');
                    }
                }
                else
                {
                    for (size_t item_occurrence = 1; item_occurrence <= dict_item.GetItemSubitemOccurs(); ++item_occurrence)
                        column_names.emplace_back(SO::ToLower(dict_item.GetName()) + '(' + std::to_string(item_occurrence) + ')');
                }
            }
            else {
                if (IsBinary(dict_item))
                {
                    //  need to label columns <item-name>-signature and <item-name>-metadata.
                    column_names.emplace_back(SO::ToLower(dict_item.GetName()) + SQLiteDictionarySchemaGenerator::BinaryColumnPostfixSignature);
                    column_names.emplace_back(SO::ToLower(dict_item.GetName()) + SQLiteDictionarySchemaGenerator::BinaryColumnPostfixMetadata);
                }
                else
                {
                    column_names.emplace_back(SO::ToLower(dict_item.GetName()));
                }
            }
        }

        ss << "SELECT ";

        for (size_t i = 0; i < column_names.size(); ++i) {
            if (i > 0)
                ss << ',';
            ss << '`' << column_names[i] << '`';
        }
        ss << " FROM `" << record_table_name <<
            "` WHERE `" << parent_level_table_name << "-id` = ?";

        if (record_metadata.GetDictRecord().GetMaxRecs() > 1)
            ss << " ORDER BY occ";
    }
    std::string sql = ss.str();
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
        throw SQLiteErrorWithMessage(m_db);

    return stmt;
}


void SQLiteQuestionnaireSerializer::DeleteCase(const std::string& case_id) const
{
    if (SQLiteStatement(m_delete_statement).Bind(1, case_id).Step() != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);
}


SQLiteQuestionnaireSerializer::PreparedStatements::~PreparedStatements()
{
    sqlite3_finalize(m_id_record);
    for (sqlite3_stmt* statement : m_records)
        sqlite3_finalize(statement);
}
