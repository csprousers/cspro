#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItemPrinter.h>
#include <zToolsO/SerializerHelper.h>
#include <zUtilO/BinaryDataReader.h>
#include <zAppO/FieldStatus.h>

class BinaryCaseItem;
class Case;
class CaseAccess;
template<typename CharType> class JsonNode;
class JsonWriter;


// --------------------------------------------------------------------------
// Case -> JSON
// --------------------------------------------------------------------------

class CaseJsonWriterSerializerHelper : public SerializerHelper::Helper
{
public:
    CaseJsonWriterSerializerHelper();

    bool GetVerbose() const           { return m_verbose; }
    void SetVerbose(bool flag = true) { m_verbose = flag; }

    bool GetWriteBlankValues() const           { return m_writeBlankValues; }
    void SetWriteBlankValues(bool flag = true) { m_writeBlankValues = flag; }

    bool GetWriteLabels() const { return m_writeLabels; }
    void SetWriteLabels(bool flag = true);

    const CaseItemPrinter* GetCaseItemPrinter() const { return m_caseItemPrinter.get(); }

    using BinaryDataWriter = std::function<void(JsonWriter& json_writer, const BinaryCaseItem& case_item, const CaseItemIndex& index)>;

    const BinaryDataWriter* GetBinaryDataWriter() const { return m_binaryDataWriter.get(); }
    void SetBinaryDataWriter(BinaryDataWriter function) { m_binaryDataWriter = std::make_unique<BinaryDataWriter>(std::move(function)); }

    const FieldStatusRetriever* GetFieldStatusRetriever() const                  { return m_fieldStatusRetriever.get(); }
    void SetFieldStatusRetriever(std::shared_ptr<FieldStatusRetriever> function) { m_fieldStatusRetriever = std::move(function); }

private:
    bool m_verbose;
    bool m_writeBlankValues;
    bool m_writeLabels;
    std::unique_ptr<const CaseItemPrinter> m_caseItemPrinter;
    std::unique_ptr<BinaryDataWriter> m_binaryDataWriter;
    std::shared_ptr<FieldStatusRetriever> m_fieldStatusRetriever;
};


inline CaseJsonWriterSerializerHelper::CaseJsonWriterSerializerHelper()
    :   m_verbose(false),
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
    CaseJsonParserHelper(const CaseAccess* case_access)
        :   m_caseAccess(case_access)
    {
    }

    virtual ~CaseJsonParserHelper() { }

    const CaseAccess* GetCaseAccess() const { return m_caseAccess; }

    virtual std::unique_ptr<BinaryDataReader> CreateBinaryDataReader(BinaryDataMetadata /*binary_data_metadata*/, const JsonNode<wchar_t>& /*json_node*/) { return nullptr; }

    void ParseJson(Case& data_case, const JsonNode<wchar_t>& json_node);

private:
    const CaseAccess* m_caseAccess;
};
