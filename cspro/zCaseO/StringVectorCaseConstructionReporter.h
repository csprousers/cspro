#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/StringBasedCaseConstructionReporter.h>
#include <zMessageO/SystemMessageIssuer.h>


class ZCASEO_API StringVectorCaseConstructionReporter : public StringBasedCaseConstructionReporter, public SystemMessageIssuer
{
public:
    const std::vector<std::string>* GetErrors() const { return m_errors.get(); }

protected:
    void BinaryDataIOError(const Case& data_case, bool read_error, const std::string& error) override;

    void WriteString(const std::string& key, std::string message) override;

    void OnIssue(MessageType message_type, int message_number, const std::string& message_text) override;
    void OnIssue(const Logic::ParserMessage& parser_message) override;
    void OnAbort(const std::string& message_text) override;

private:
    template<typename T>
    void AddError(T&& error);

private:
    std::unique_ptr<std::vector<std::string>> m_errors;
};
