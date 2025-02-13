#include "StdAfx.h"
#include "ProcessSummary.h"


void ProcessSummary::WriteJson(JsonWriter& json_writer, const std::vector<std::wstring>* level_names/* = nullptr*/) const
{
    json_writer.BeginObject();

    // read percent
    json_writer.Write(JK::readPercent, m_percentSourceRead);

    // attributes
    json_writer.BeginObject(( m_attributesType == AttributesType::Records ) ? JK::records : JK::slices)
               .Write(JK::read, m_attributesRead)
               .Write(JK::ignored, m_attributesIgnored)
               .Write(JK::unknown, m_attributesUnknown)
               .Write(JK::erased, m_attributesErased)
               .EndObject();

    // messages
    json_writer.BeginObject(JK::messages)
               .Write(JK::total, m_totalMessages)
               .Write(JK::error, m_errorMessages)
               .Write(JK::warning, m_warningMessages)
               .Write(JK::user, m_userMessages)
               .EndObject();

    // levels
    size_t level_number = 0;

    json_writer.WriteObjects(JK::levels, m_levelSummaries,
        [&](const LevelSummary& level_summary)
        {
            if( level_names != nullptr )
            {
                ASSERT(level_number < level_names->size());
                json_writer.Write(JK::name, level_names->at(level_number));
            }
            
            json_writer.Write(JK::count, level_summary.input_cases)
                       .Write(JK::badStructures, level_summary.bad_structures)
                       .Write(JK::levelPosts, level_summary.level_posts);

            ++level_number;
        });

    json_writer.EndObject();
}
