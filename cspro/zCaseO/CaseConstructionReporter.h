#pragma once

#include <zUtilO/DataTypes.h>
#include <zUtilO/ProcessSummary.h>

class Case;


// base class for an object that can receive warning messages occurring during data source or case construction

class CaseConstructionReporter
{
public:
    CaseConstructionReporter(std::shared_ptr<ProcessSummary> process_summary = nullptr);
    virtual ~CaseConstructionReporter() { }

    template<typename... Args>
    void IssueMessage(MessageType message_type, int message_number, Args const&... args);

    virtual void UnsupportedContentType(const std::string& dictionary_name, const std::string& content_type_names);

    virtual void BadRecordType(const Case& data_case, const std::string& record_type, const std::string& case_line);

    virtual void TooManyRecordOccurrences(const Case& data_case, const std::string& record_name, size_t maximum_occurrences);

    virtual void BlankRecordAdded(const std::string& key, const std::string& record_name);

    virtual void DuplicateUuid(const Case& data_case);

    virtual void BinaryDataIOError(const Case& data_case, bool read_error, const std::string& error);

    virtual void ContentTypeConversionError(const Case& data_case, const std::string& item_name,
                                            ContentType input_content_type, ContentType output_content_type);


    size_t GetCaseLevelCount(size_t level_number) const;
    void IncrementCaseLevelCount(size_t level_number);

    size_t GetRecordCount() const;
    void IncrementRecordCount();

    size_t GetBadRecordCount() const;
    void IncrementBadRecordCount();

    size_t GetErasedRecordCount() const;
    void IncrementErasedRecordCount();

protected:
    virtual void OnIssueMessage(MessageType message_type, int message_number, va_list parg);

private:
    void IssueMessageWorker(MessageType message_type, int message_number, ...);

private:
    const std::shared_ptr<ProcessSummary> m_processSummary;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CaseConstructionReporter::CaseConstructionReporter(std::shared_ptr<ProcessSummary> process_summary/* = nullptr*/)
    :   m_processSummary(std::move(process_summary))
{
}


template<typename... Args>
void CaseConstructionReporter::IssueMessage(const MessageType message_type, const int message_number, Args const&... args)
{
#ifdef _DEBUG
    ValidateFormatTextArgumentTypes<char>(args...);
#endif

    IssueMessageWorker(message_type, message_number, args...);
}


inline void CaseConstructionReporter::IssueMessageWorker(const MessageType message_type, const int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);
    OnIssueMessage(message_type, message_number, parg);
    va_end(parg);
}


inline void CaseConstructionReporter::OnIssueMessage(MessageType /*message_type*/, int /*message_number*/, va_list /*parg*/)
{
}


inline void CaseConstructionReporter::UnsupportedContentType(const std::string& /*dictionary_name*/, const std::string& /*content_type_names*/)
{
}


inline void CaseConstructionReporter::BadRecordType(const Case& /*data_case*/, const std::string& /*record_type*/, const std::string& /*case_line*/)
{
}


inline void CaseConstructionReporter::TooManyRecordOccurrences(const Case& /*data_case*/, const std::string& /*record_name*/, size_t /*maximum_occurrences*/)
{
}


inline void CaseConstructionReporter::BlankRecordAdded(const std::string& /*key*/, const std::string& /*record_name*/)
{
}


inline void CaseConstructionReporter::DuplicateUuid(const Case& /*data_case*/)
{
}


inline void CaseConstructionReporter::BinaryDataIOError(const Case& /*data_case*/, bool /*read_error*/, const std::string& /*error*/)
{
}


inline void CaseConstructionReporter::ContentTypeConversionError(const Case& /*data_case*/, const std::string& /*item_name*/,
                                                                 ContentType /*input_content_type*/, ContentType /*output_content_type*/)
{
}


inline size_t CaseConstructionReporter::GetCaseLevelCount(size_t level_number) const
{
    ASSERT(m_processSummary != nullptr);
    return m_processSummary->GetCaseLevelsRead(level_number);
}


inline void CaseConstructionReporter::IncrementCaseLevelCount(size_t level_number)
{
    if( m_processSummary != nullptr )
        m_processSummary->IncrementCaseLevelsRead(level_number);
}


inline size_t CaseConstructionReporter::GetRecordCount() const
{
    ASSERT(m_processSummary != nullptr);
    return m_processSummary->GetAttributesRead();
}


inline void CaseConstructionReporter::IncrementRecordCount()
{
    if( m_processSummary != nullptr )
        m_processSummary->IncrementAttributesRead();
}


inline size_t CaseConstructionReporter::GetBadRecordCount() const
{
    ASSERT(m_processSummary != nullptr);
    return m_processSummary->GetAttributesUnknown();
}


inline void CaseConstructionReporter::IncrementBadRecordCount()
{
    if( m_processSummary != nullptr )
        m_processSummary->IncrementAttributesUnknown();
}


inline size_t CaseConstructionReporter::GetErasedRecordCount() const
{
    ASSERT(m_processSummary != nullptr);
    return m_processSummary->GetAttributesErased();
}

inline void CaseConstructionReporter::IncrementErasedRecordCount()
{
    if( m_processSummary != nullptr )
        m_processSummary->IncrementAttributesErased();
}
