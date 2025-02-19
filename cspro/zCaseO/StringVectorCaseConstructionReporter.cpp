#include "stdafx.h"
#include "StringVectorCaseConstructionReporter.h"


template<typename T>
void StringVectorCaseConstructionReporter::AddError(T&& error)
{
    if( m_errors == nullptr )
        m_errors = std::make_unique<std::vector<std::string>>();

    m_errors->emplace_back(std::forward<T>(error));
}


void StringVectorCaseConstructionReporter::BinaryDataIOError(const Case& data_case, bool read_error, const std::string& error)
{
    StringBasedCaseConstructionReporter::BinaryDataIOError(data_case, read_error, error);

    // because these errors can occur multiple times due to the lazy loading of binary data,
    // not just when constructing the case, we will make sure that the error appears only once
    ASSERT(m_errors != nullptr && !m_errors->empty());
    const auto& added_message_pos = m_errors->cend() - 1;

    if( std::find(m_errors->cbegin(), added_message_pos, *added_message_pos) != added_message_pos )
        m_errors->pop_back();
}


void StringVectorCaseConstructionReporter::WriteString(const std::string& /*key*/, std::string message)
{
    AddError(std::move(message));
}


void StringVectorCaseConstructionReporter::OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::string& message_text)
{
    AddError(message_text);
}


void StringVectorCaseConstructionReporter::OnIssue(const Logic::ParserMessage& parser_message)
{
    ASSERT(false);
    AddError(parser_message.message_text);
}


void StringVectorCaseConstructionReporter::OnAbort(const std::string& message_text)
{
    ASSERT(false);
    AddError(message_text);
}
