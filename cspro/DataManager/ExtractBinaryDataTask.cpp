#include "StdAfx.h"
#include "ExtractBinaryDataTask.h"
#include "TaskRunner.h"
#include <zToolsO/Hash.h>
#include <zCaseO/BinaryCaseItem.h>


CREATE_JSON_KEY(invalidCharacterReplacement)
CREATE_JSON_KEY(rightTrimKeys)

CREATE_ENUM_JSON_SERIALIZER(ExtractBinaryDataSettings::FilenameFormat,
    { ExtractBinaryDataSettings::FilenameFormat::Filename,     "filename" },
    { ExtractBinaryDataSettings::FilenameFormat::KeyFilename,  "keyFilename" },
    { ExtractBinaryDataSettings::FilenameFormat::Signature,    "signature" },
    { ExtractBinaryDataSettings::FilenameFormat::KeySignature, "keySignature" })


// --------------------------------------------------------------------------
// ExtractBinaryDataSettings
// --------------------------------------------------------------------------

ExtractBinaryDataSettings ExtractBinaryDataSettings::CreateFromJson(const JsonNode& json_node)
{
    return ExtractBinaryDataSettings
    {
        json_node.Get<FilenameFormat>(JK::format),
        json_node.Get<bool>(JK::rightTrimKeys),
        json_node.Get<std::string>(JK::invalidCharacterReplacement),
        json_node.Get<std::string>(JK::directory)
    };
}


void ExtractBinaryDataSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::format, filename_format)
               .Write(JK::rightTrimKeys, right_trim_keys)
               .Write(JK::invalidCharacterReplacement, invalid_character_replacement)
               .Write(JK::directory, output_directory)
               .EndObject();
}



// --------------------------------------------------------------------------
// ExtractBinaryDataTask
// --------------------------------------------------------------------------

ExtractBinaryDataTask::ExtractBinaryDataTask(ExtractBinaryDataSettings settings)
    :   m_settings(std::move(settings)),
        m_casesWithBinaryData(0),
        m_extractedFiles(0),
        m_skippedFiles(0)
{
}


void ExtractBinaryDataTask::ValidateSettings(const ExtractBinaryDataSettings& settings)
{
    if( !settings.invalid_character_replacement.empty() )
    {
        if( SO::WideLength(settings.invalid_character_replacement) > 1 )
        {
            throw CSProException("The invalid character replacement, '%s', cannot be longer than a single character.",
                                 settings.invalid_character_replacement.c_str());
        }

        if(  Path::CreateValidFilename(settings.invalid_character_replacement) != settings.invalid_character_replacement )
        {
            throw CSProException("The invalid character replacement, '%s', is itself not a valid filename character.",
                                 settings.invalid_character_replacement.c_str());
        }
    }
}


std::string ExtractBinaryDataTask::CreateValidFilename(const ExtractBinaryDataSettings& settings, const std::string& case_key, std::string filename)
{
    if( settings.filename_format == ExtractBinaryDataSettings::FilenameFormat::KeyFilename ||
        settings.filename_format == ExtractBinaryDataSettings::FilenameFormat::KeySignature )
    {
        filename = SO::Concatenate(settings.right_trim_keys ? SO::TrimRight(case_key) : case_key,
                                   " - ",
                                   filename);

        // all left spaces must be replaced since a filename cannot start with a space
        const size_t left_spaces = case_key.length() - SO::TrimLeft(case_key).length();

        for( size_t i = 0; i < left_spaces; ++i )
            filename.replace(i * settings.invalid_character_replacement.length(), 1, settings.invalid_character_replacement);
    }

    return Path::MakeValidFilename(filename, &settings.invalid_character_replacement);
}


void ExtractBinaryDataTask::Initialize()
{
    const size_t number_binary_dictionary_items = m_caseAccess->GetCaseMetadata().GetTotalNumberBinaryCaseItems();

    m_taskRunner->SetTitle(FormatText("Extracting binary data from %d item%s...", static_cast<int>(number_binary_dictionary_items),
                                                                                  PluralizeWord(number_binary_dictionary_items)));

    ValidateSettings(m_settings);

    FileIO::CreateDirectories(m_settings.output_directory);
}


void ExtractBinaryDataTask::ProcessCase(Case& data_case)
{
    bool had_binary_data = false;

    data_case.ForeachDefinedBinaryCaseItem(
        [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
        {
            const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

            const std::string file_path = Path::Combine(m_settings.output_directory,
                                                        CreateValidFilename(m_settings, data_case.GetKey(), binary_case_item.GetSuggestedFilename(index)));

            // if the file already exists with the same contents, don't write it out again
            const auto& lookup = m_writtenSignatures.find(file_path);

            if( lookup != m_writtenSignatures.cend() )
            {
                ++m_skippedFiles;
            }

            else
            {
                m_writtenSignatures.try_emplace(file_path, binary_data_accessor.GetSignature());

                if( PortableFunctions::FileSize(file_path) == static_cast<int64_t>(binary_data_accessor.GetBinaryDataSize()) &&
                    Hash::Md5::CreateFromFile(file_path) == binary_data_accessor.GetSignature() )
                {
                    ++m_skippedFiles;
                }

                else
                {
                    FileIO::Write(file_path, binary_data_accessor.GetBinaryData().GetContent());
                    ++m_extractedFiles;
                }
            }

            had_binary_data = true;
        });

    if( had_binary_data )
        ++m_casesWithBinaryData;
}


void ExtractBinaryDataTask::Finalize(const Result result)
{
    if( result == Result::Complete )
    {
        m_taskRunner->LogText("Process summary:");
        m_taskRunner->LogText("    Cases processed: %d", static_cast<int>(GetCasesProcessed()));
        m_taskRunner->LogText("    Cases with binary data: %d", static_cast<int>(m_casesWithBinaryData));

        if( m_skippedFiles != 0 )
            m_taskRunner->LogText("    Files previously extracted: %d", static_cast<int>(m_skippedFiles));

        m_taskRunner->LogText();
        m_taskRunner->LogText("Successfully extracted %d file%s.", static_cast<int>(m_extractedFiles),
                                                                   PluralizeWord(m_extractedFiles));
    }
}
