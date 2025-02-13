#pragma once

class Case;

/// <summary>
/// Interface for reading/writing questionnaire portion case in SQLiteRepository.
/// Derived classes can write out old school blobs or write to columns/tables
/// </summary>
struct ISQLiteQuestionnaireSerializer {

    /// <summary>
    /// Read a questionnaire from the database and fill in the case
    /// </summary>
    /// <param name="data_case">case to read questionnaire data into</param>
    /// <returns>SQLite result code</returns>
    virtual void ReadQuestionnaire(Case& data_case) const = 0;

    /// <summary>
    /// Write the questionnaire portion of case_data to the database
    /// </summary>
    /// <param name="data_case">case to write questionnaire data from</param>
    virtual void WriteQuestionnaire(const Case& data_case, int64_t revision) const = 0;

    /// <summary>
    /// Write the questionnaire portion of case_data to the database
    /// </summary>
    /// <param name="case_access">describes items used</param>
    /// <param name="read_only">if true, data is never written</param>
    virtual void SetCaseAccess(std::shared_ptr<const CaseAccess> case_access, bool read_only) = 0;

    virtual ~ISQLiteQuestionnaireSerializer() {};
    ISQLiteQuestionnaireSerializer() = default;
    ISQLiteQuestionnaireSerializer(const ISQLiteQuestionnaireSerializer&) = delete;
    ISQLiteQuestionnaireSerializer& operator=(const ISQLiteQuestionnaireSerializer&) = delete;
};
