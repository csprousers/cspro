#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItemPrinter.h>
#include <zToolsO/SerializerHelper.h>
#include <zAppO/FieldStatus.h>

class BinaryCaseItem;
class BinaryContentReader;
class BinaryDataMetadata;
class Case;
class CaseAccess;


// --------------------------------------------------------------------------
// Case -> JSON
// --------------------------------------------------------------------------

class CaseJsonWriterSerializerHelper : public SerializerHelper::Helper
{
public:
    CaseJsonWriterSerializerHelper();

    bool GetVerbose() const           { return m_verbose; }
    void SetVerbose(bool flag = true) { m_verbose = flag; }

    bool GetWriteCasePositions() const           { return m_writeCasePositions; }
    void SetWriteCasePositions(bool flag = true) { m_writeCasePositions = flag; }

    bool GetWriteBlankValues() const           { return m_writeBlankValues; }
    void SetWriteBlankValues(bool flag = true) { m_writeBlankValues = flag; }

    bool GetWriteLabels() const { return m_writeLabels; }
    void SetWriteLabels(bool flag = true);

    const CaseItemPrinter* GetCaseItemPrinter() const { return m_caseItemPrinter.get(); }

    using BinaryDataWriter = std::function<void(JsonWriter& json_writer, const BinaryCaseItem& case_item, const CaseItemIndex& index)>;

    const BinaryDataWriter* GetBinaryDataWriter() const { return m_binaryDataWriter.get(); }
    void SetBinaryDataWriter(BinaryDataWriter function) { m_binaryDataWriter = std::make_unique<BinaryDataWriter>(std::move(function)); }
    void ClearBinaryDataWriter()                        { m_binaryDataWriter.reset(); }

    const FieldStatusRetriever* GetFieldStatusRetriever() const                  { return m_fieldStatusRetriever.get(); }
    void SetFieldStatusRetriever(std::shared_ptr<FieldStatusRetriever> function) { m_fieldStatusRetriever = std::move(function); }

    // methods that can be overridden
    virtual bool GetWriteCaseNote() const    { return false; }
    virtual bool GetWriteVectorClock() const { return false; }

private:
    bool m_verbose;
    bool m_writeCasePositions;
    bool m_writeBlankValues;
    bool m_writeLabels;
    std::unique_ptr<const CaseItemPrinter> m_caseItemPrinter;
    std::unique_ptr<BinaryDataWriter> m_binaryDataWriter;
    std::shared_ptr<FieldStatusRetriever> m_fieldStatusRetriever;
};


inline CaseJsonWriterSerializerHelper::CaseJsonWriterSerializerHelper()
    :   m_verbose(false),
        m_writeCasePositions(false),
        m_writeBlankValues(false),
        m_writeLabels(false)
{
}


inline void CaseJsonWriterSerializerHelper::SetWriteLabels(bool flag/* = true*/)
{
    m_writeLabels = flag;
    m_caseItemPrinter = m_writeLabels ? std::make_unique<CaseItemPrinter>(CaseItemPrinter::Format::Label) :
                                        nullptr;
}



// --------------------------------------------------------------------------
// JSON -> Case
// --------------------------------------------------------------------------

class ZCASEO_API CaseJsonParserHelper
{
public:
    CaseJsonParserHelper(std::shared_ptr<const CaseAccess> case_access)
        :   m_caseAccess(std::move(case_access))
    {
    }

    virtual ~CaseJsonParserHelper() { }

    const CaseAccess* GetCaseAccess() const { return m_caseAccess.get(); }

    // the base implemention returns null
    virtual std::unique_ptr<BinaryContentReader> CreateBinaryContentReader(std::optional<uint64_t> size);

    void ParseJson(Case& data_case, const JsonNode& json_node);

    static void ParseNumericCaseItem(const NumericCaseItem& numeric_case_item, CaseItemIndex& index, const JsonNode& case_item_node);
    static void ParseStringCaseItem(const StringCaseItem& string_case_item, CaseItemIndex& index, const JsonNode& case_item_node);
    void ParseBinaryCaseItem(const BinaryCaseItem& binary_case_item, CaseItemIndex& index, const JsonNode& case_item_node);

protected:
    std::shared_ptr<const CaseAccess> m_caseAccess;
};
