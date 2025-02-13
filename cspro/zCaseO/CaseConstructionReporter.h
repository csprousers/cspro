#pragma once

#include <zUtilO/DataTypes.h>
#include <zUtilO/ProcessSummary.h>

class Case;


// base class for an object that can receive warning messages occurring during data source or case construction

class CaseConstructionReporter
{
public:
    CaseConstructionReporter(std::shared_ptr<ProcessSummary> process_summary = nullptr)
        :   m_processSummary(process_summary)
    {
    }

    virtual ~CaseConstructionReporter() { }

    virtual void IssueMessage(MessageType /*message_type*/, int /*message_number*/, ...) { }

    virtual void UnsupportedContentType(const CString& /*dictionary_name*/, const std::wstring& /*content_type_names*/) { }

    virtual void BadRecordType(const Case& /*data_case*/, const CString& /*record_type*/, const CString& /*case_line*/) { }

    virtual void TooManyRecordOccurrences(const Case& /*data_case*/, const CString& /*record_name*/, size_t /*maximum_occurrences*/) { }

    virtual void BlankRecordAdded(const CString& /*key*/, const CString& /*record_name*/) { }

    virtual void DuplicateUuid(const Case& /*data_case*/) { }

    virtual void BinaryDataIOError(const Case& /*data_case*/, bool /*read_error*/, const std::wstring& /*error*/) { }

    virtual void ContentTypeConversionError(const Case& /*data_case*/, const CString& /*item_name*/,
                                            ContentType /*input_content_type*/, ContentType /*output_content_type*/) { }


    size_t GetCaseLevelCount(size_t level_number) const
    {
        ASSERT(m_processSummary != nullptr);
        return m_processSummary->GetCaseLevelsRead(level_number);
    }

    void IncrementCaseLevelCount(size_t level_number)
    {
        if( m_processSummary != nullptr )
            m_processSummary->IncrementCaseLevelsRead(level_number);
    }


    size_t GetRecordCount() const
    {
        ASSERT(m_processSummary != nullptr);
        return m_processSummary->GetAttributesRead();
    }

    void IncrementRecordCount()
    {
        if( m_processSummary != nullptr )
            m_processSummary->IncrementAttributesRead();
    }


    size_t GetBadRecordCount() const
    {
        ASSERT(m_processSummary != nullptr);
        return m_processSummary->GetAttributesUnknown();
    }

    void IncrementBadRecordCount()
    {
        if( m_processSummary != nullptr )
            m_processSummary->IncrementAttributesUnknown();
    }


    size_t GetErasedRecordCount() const
    {
        ASSERT(m_processSummary != nullptr);
        return m_processSummary->GetAttributesErased();
    }

    void IncrementErasedRecordCount()
    {
        if( m_processSummary != nullptr )
            m_processSummary->IncrementAttributesErased();
    }

private:
    std::shared_ptr<ProcessSummary> m_processSummary;
};
