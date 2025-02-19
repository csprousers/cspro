#pragma once

#include <zListingO/Lister.h>
#include <zToolsO/DelimitedTextCreator.h>

class CaseItem;
namespace FileIO { class TextFile; }
namespace Listing { class CsvLister; }


class Listing::CsvLister : public Lister
{
public:
    CsvLister(std::shared_ptr<ProcessSummary> process_summary, const std::string& file_path, bool append, std::shared_ptr<const CaseAccess> case_access);
    ~CsvLister();

protected:
    void WriteMessages(const Messages& messages) override;

    void ProcessCaseSource(const Case* data_case) override;

private:
    std::unique_ptr<FileIO::TextFile> m_textFile;

    DelimitedTextCreator m_csvTextCreator;
    DelimitedTextCreator m_csvTextCreatorForKeys;

    std::shared_ptr<const CaseAccess> m_caseAccess;

    bool m_useLevelKey;
    std::vector<const CaseItem*> m_idCaseItems;
    std::vector<char> m_numericCaseItemConversionBuffer;
};
