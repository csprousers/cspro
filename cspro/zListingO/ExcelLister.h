#pragma once

#include <zListingO/Lister.h>

class CaseItem;
class ExcelWriter;
namespace Listing { class ExcelLister; }


class Listing::ExcelLister : public Lister
{
public:
    ExcelLister(std::shared_ptr<ProcessSummary> process_summary, const std::string& file_path, std::shared_ptr<const CaseAccess> case_access);
    ~ExcelLister();

protected:
    void WriteMessages(const Messages& messages) override;

    void ProcessCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary) override;

    void ProcessCaseSource(const Case* data_case) override;

private:
    std::unique_ptr<ExcelWriter> m_excelWriter;
    uint32_t m_row;

    std::shared_ptr<const CaseAccess> m_caseAccess;

    bool m_useLevelKey;
    std::vector<const CaseItem*> m_idCaseItems;

    using KeyValueType = std::variant<std::monostate, double, SharableString>; // std::monostate = blank
    std::vector<KeyValueType> m_keyValues;

    std::optional<std::string> m_inputDataUri;
    std::optional<std::tuple<std::string, std::string>> m_caseKeyUuid;
};
