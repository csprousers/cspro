#pragma once

#include <zUtilO/zUtilO.h>
#include <zMessageO/MessageType.h>
#include <zDictO/DDClass.h>

class JsonWriter;


class ProcessSummary
{
private:
    struct LevelSummary
    {
        size_t input_cases;
        size_t bad_structures;
        size_t level_posts;
    };

public:
    enum class AttributesType { Records, Slices };

    size_t GetPercentSourceRead() const       { return m_percentSourceRead; }

    template<typename T>
    void SetPercentSourceRead(T&& percent) { m_percentSourceRead = static_cast<size_t>(std::forward<T>(percent)); }

    size_t GetNumberLevels() const { return m_levelSummaries.size(); }
    void SetNumberLevels(size_t number_levels);

    size_t GetCaseLevelsRead(size_t level_number) const         { return m_levelSummaries[LS_Check(level_number)].input_cases; }
    size_t GetBadCaseLevelStructures(size_t level_number) const { return m_levelSummaries[LS_Check(level_number)].bad_structures; }
    size_t GetLevelPostProcsExecuted(size_t level_number) const { return m_levelSummaries[LS_Check(level_number)].level_posts; }

    void IncrementCaseLevelsRead(size_t level_number)         { ++m_levelSummaries[LS_Check(level_number)].input_cases; }
    void IncrementBadCaseLevelStructures(size_t level_number) { ++m_levelSummaries[LS_Check(level_number)].bad_structures; }
    void IncrementPostProcsExecuted(size_t level_number)      { ++m_levelSummaries[LS_Check(level_number)].level_posts; }

    AttributesType GetAttributesType() const    { return m_attributesType; }
    void SetAttributesType(AttributesType type) { m_attributesType = type; }
                                                                
    size_t GetAttributesRead() const    { return m_attributesRead; }
    size_t GetAttributesUnknown() const { return m_attributesUnknown; }
    size_t GetAttributesErased() const  { return m_attributesErased; }
    size_t GetAttributesIgnored() const { return m_attributesIgnored; }
                                                                
    void IncrementAttributesRead()                  { ++m_attributesRead; }
    void IncrementAttributesRead(size_t attributes) { m_attributesRead += attributes; }
    void IncrementAttributesUnknown()               { ++m_attributesUnknown; ++m_attributesIgnored; }
    void IncrementAttributesErased()                { ++m_attributesErased;  ++m_attributesIgnored; }
                                                                
    size_t GetErrorMessages() const   { return m_errorMessages; }
    size_t GetWarningMessages() const { return m_warningMessages; }
    size_t GetUserMessages() const    { return m_userMessages; }
    size_t GetTotalMessages() const   { return m_totalMessages; }

    void IncrementMessageCounter(MessageType message_type);

    // for tabulation applications
    const std::map<int, size_t>& GetSystemMessagesCountMap() const { return m_systemMessageCounts; }
    std::map<int, size_t>& GetSystemMessagesCountMap()             { return m_systemMessageCounts; }

    // serialization
    CLASS_DECL_ZUTILO void WriteJson(JsonWriter& json_writer, const std::vector<std::wstring>* level_names = nullptr) const;

private:
    size_t LS_Check(size_t level_number) const;

private:
    size_t m_percentSourceRead = 0;

    std::vector<LevelSummary> m_levelSummaries;

    AttributesType m_attributesType = AttributesType::Records;
    size_t m_attributesRead = 0;
    size_t m_attributesUnknown = 0;
    size_t m_attributesErased = 0;
    size_t m_attributesIgnored = 0;

    size_t m_errorMessages = 0;
    size_t m_warningMessages = 0;
    size_t m_userMessages = 0;
    size_t m_totalMessages = 0;

    std::map<int, size_t> m_systemMessageCounts;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline size_t ProcessSummary::LS_Check(const size_t level_number) const
{
    ASSERT(level_number < m_levelSummaries.size());
    return level_number;
}


inline void ProcessSummary::SetNumberLevels(const size_t number_levels)
{
    ASSERT(number_levels <= MaxNumberLevels);

    m_levelSummaries.clear();
    m_levelSummaries.resize(number_levels);
}


inline void ProcessSummary::IncrementMessageCounter(const MessageType message_type)
{
    ++m_totalMessages;

    if( message_type == MessageType::Error )
    {
        ++m_errorMessages;
    }

    else if( message_type == MessageType::Warning )
    {
        ++m_warningMessages;
    }

    else if( message_type == MessageType::User )
    {
        ++m_userMessages;
    }
}
