#include "stdafx.h"
#include "TextRepositoryStatusFile.h"
#include "CaseIterator.h"
#include "TextRepository.h"
#include <zToolsO/IniFile.h>
#include <zCaseO/CaseConstructionHelpers.h>


namespace FileCommands
{
    constexpr const char* FileHeader = "[KeyInfo]";

    constexpr const char* VerifiedHeader = "[Verified]";
    constexpr const char* VerifiedKey    = "VerifiedKey";

    constexpr const char* OldStyleVerifiedHeader  = "[LastVerified]";
    constexpr const char* OldStyleLastVerifiedKey = "NodeKey";

    constexpr const char* PartialSaveHeader         = "[PartialNodes]";
    constexpr const char* PartialSaveKey            = "Pos";
    constexpr const char  PartialSaveTokenDelimiter = '.';
    constexpr const char* PartialSaveModeAdd        = "ADD";
    constexpr const char* PartialSaveModeModify     = "MOD";
    constexpr const char* PartialSaveModeVerify     = "VER";

    constexpr const char* CaseLabelHeader   = "[CaseLabel]";
    constexpr const char* CaseLabelKeyLabel = "KeyLabel";
};



TextRepositoryStatusFile::TextRepositoryStatusFile(TextRepository& repository, const DataRepositoryOpenFlag open_flag)
    :   m_repository(repository),
        m_filePath(GetStatusFilePath(m_repository.GetConnectionString())),
        m_dictionaryName(m_repository.GetCaseAccess().GetDataDict().GetName()),
        m_hasTransactionsToWrite(false)
{
    if( PortableFunctions::FileIsRegular(m_filePath) )
    {
        ASSERT(open_flag != DataRepositoryOpenFlag::CreateNew);
        Load();
    }
}


TextRepositoryStatusFile::~TextRepositoryStatusFile()
{
    ASSERT(!m_hasTransactionsToWrite);
}


std::string TextRepositoryStatusFile::GetStatusFilePath(const ConnectionString& connection_string)
{
    return PortableFunctions::PathAppendFileExtension(connection_string.GetFilePath(), FileExtensions::Data::TextStatus);
}


void TextRepositoryStatusFile::Load()
{
    CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(InvalidLineException, "");

    std::string command;
    std::string argument;

    try
    {
        IniFileReader sts_file;
        sts_file.SetProperties(m_repository.GetConnectionString());

        try
        {
            sts_file.Open(m_filePath);
        }

        catch( const CSProException& exception )
        {
            throw DataRepositoryException::IOError("There was an error opening the status file: %s", exception.what());
        }

        const size_t key_wide_length = m_repository.m_keyMetadata->key_length;
        const bool load_statuses = m_repository.GetCaseAccess().GetUsesStatuses();
        const bool load_case_labels = m_repository.GetCaseAccess().GetUsesCaseLabels();

        enum class ProcessingSection { None, Header, Verified, OldStyleVerified, PartialSaves, CaseLabels };
        ProcessingSection processing_section = ProcessingSection::None;

        while( sts_file.ReadLine(command, argument, false) )
        {
            SO::MakeTrim(command);

            if( command.empty() )
                continue;

            // turn ␤ -> \n
            ASSERT(!SO::ContainsNewlineCharacter(command));
            NewlineSubstitutor::MakeUnicodeNLToNewline(argument);

            if( SO::EqualsNoCase(command, FileCommands::FileHeader) )
            {
                processing_section = ProcessingSection::Header;
            }

            else if( SO::EqualsNoCase(command, FileCommands::VerifiedHeader) )
            {
                processing_section = ProcessingSection::Verified;
            }

            else if( SO::EqualsNoCase(command, FileCommands::OldStyleVerifiedHeader) )
            {
                processing_section = ProcessingSection::OldStyleVerified;
            }

            else if( SO::EqualsNoCase(command, FileCommands::PartialSaveHeader) )
            {
                processing_section = ProcessingSection::PartialSaves;
            }

            else if( SO::EqualsNoCase(command, FileCommands::CaseLabelHeader) )
            {
                processing_section = ProcessingSection::CaseLabels;
            }


            else if( processing_section == ProcessingSection::Header )
            {
                if( SO::EqualsNoCase(command, IniFileBase::VersionKey) )
                {
                    // we will read but not process the version
                }

                else
                {
                    throw InvalidLineException();
                }
            }


            else if( processing_section == ProcessingSection::Verified )
            {
                if( SO::EqualsNoCase(command, FileCommands::VerifiedKey) )
                {
                    if( load_statuses )
                        GetOrCreateStatus(argument).verified = true;
                }

                else
                {
                    throw InvalidLineException();
                }
            }


            else if( processing_section == ProcessingSection::OldStyleVerified )
            {
                if( SO::EqualsNoCase(command, FileCommands::OldStyleLastVerifiedKey) )
                {
                    // we need to look at the cases in the repository and mark all as verified up to and including this key
                    if( load_statuses && m_repository.m_requiresIndex )
                    {
                        CaseKey case_key;
                        std::vector<std::string> keys;
                        bool key_found = false;

                        const std::unique_ptr<CaseIterator> case_key_iterator = m_repository.CreateCaseKeyIterator(CaseIterationMethod::SequentialOrder,
                                                                                                                   CaseIterationOrder::Ascending);

                        while( !key_found && case_key_iterator->NextCaseKey(case_key) )
                        {
                            keys.emplace_back(case_key.GetKey());
                            key_found = ( case_key.GetKey() == argument );
                        }

                        if( key_found )
                        {
                            for( const std::string& key : keys )
                                GetOrCreateStatus(key).verified = true;
                        }
                    }
                }

                else
                {
                    throw InvalidLineException();
                }
            }


            else if( processing_section == ProcessingSection::PartialSaves )
            {
                if( SO::EqualsNoCase(command, FileCommands::PartialSaveKey) )
                {
                    if( load_statuses )
                    {
                        std::string parameters[4];
                        size_t occurrences[3] = { 0, 0, 0 };
                        size_t processing_element = 0;

                        SO::ForeachSection<std::string>(argument, FileCommands::PartialSaveTokenDelimiter,
                            [&](std::string element)
                            {
                                if( processing_element < 4 )
                                {
                                    parameters[processing_element] = std::move(element);
                                }

                                else if( processing_element < 7 )
                                {
                                    occurrences[processing_element - 4] = std::max(atoi(element.c_str()) - 1, 0);
                                }

                                ++processing_element;
                            });

                        // check that the processed line is valid, which means that...

                        // there must be 3 (if no field information), 4 (if no occurrences), or 7 elements
                        if( processing_element != 3 && processing_element != 4 && processing_element != 7 )
                            throw InvalidLineException();

                        // the partial save mode must be valid
                        const PartialSaveMode partial_save_mode =
                            SO::EqualsNoCase(parameters[0], FileCommands::PartialSaveModeAdd)    ? PartialSaveMode::Add :
                            SO::EqualsNoCase(parameters[0], FileCommands::PartialSaveModeModify) ? PartialSaveMode::Modify :
                            SO::EqualsNoCase(parameters[0], FileCommands::PartialSaveModeVerify) ? PartialSaveMode::Verify :
                                                                                                   throw InvalidLineException();

                        // the key must be equal to or bigger than the first-level dictionary key
                        std::string key = parameters[1];
                        std::string level_key;

                        const size_t this_key_wide_length = SO::WideLength(key);

                        if( this_key_wide_length < key_wide_length )
                            throw InvalidLineException();

                        if( this_key_wide_length > key_wide_length )
                        {
                            const size_t level_key_offset = SO::WideGetOffset(key, key_wide_length);
                            level_key = key.substr(level_key_offset);
                            key.resize(level_key_offset);
                        }

                        // the dictionary name must match the repository's dictionary name
                        if( !SO::EqualsNoCase(parameters[2], m_dictionaryName) )
                            throw InvalidLineException();

                        std::shared_ptr<CaseItemReference> partial_save_case_item_reference;

                        if( processing_element > 3 )
                        {
                            partial_save_case_item_reference = CaseConstructionHelpers::CreateCaseItemReference(m_repository.GetCaseAccess(),
                                                                                                                std::move(level_key),
                                                                                                                parameters[3],
                                                                                                                occurrences);
                        }

                        Status& status = GetOrCreateStatus(key);
                        status.partial_save_mode = partial_save_mode;
                        status.partial_save_case_item_reference = partial_save_case_item_reference;
                    }
                }

                else
                {
                    throw InvalidLineException();
                }
            }


            else if( processing_section == ProcessingSection::CaseLabels )
            {
                if( SO::EqualsNoCase(command, FileCommands::CaseLabelKeyLabel) )
                {
                    if( load_case_labels )
                    {
                        const size_t case_label_offset = SO::WideGetOffset(argument, key_wide_length);

                        if( case_label_offset == std::string_view::npos )
                            throw InvalidLineException();

                        const std::string key = argument.substr(0, case_label_offset);
                        GetOrCreateStatus(key).case_label = argument.substr(case_label_offset);
                    }
                }

                else
                {
                    throw InvalidLineException();
                }
            }


            else
            {
                throw InvalidLineException();
            }
        }

        sts_file.Close();
    }

    catch( const InvalidLineException& )
    {
        throw DataRepositoryException::IOError("The status file had an invalid line: %s=%s",
                                               command.c_str(), argument.c_str());
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch(...)
    {
        throw DataRepositoryException::IOError("There was an error reading the status file.");
    }
}


void TextRepositoryStatusFile::CommitTransactions()
{
    ASSERT(m_repository.m_useTransactionManager);

    if( m_hasTransactionsToWrite )
        Save(true);
}


void TextRepositoryStatusFile::Save(const bool force_write_to_disk/* = false*/)
{
    if( m_repository.m_useTransactionManager && !force_write_to_disk )
    {
        m_hasTransactionsToWrite = true;
        return;
    }

    try
    {
        static_assert(FileIO::TextFile::DefaultWriteNewlineAsCRLF == true);
        IniFileWriter sts_file;
        sts_file.SetProperties(m_repository.GetConnectionString());

        try
        {
            sts_file.Open(m_filePath);
        }

        catch( const CSProException& exception )
        {
            throw DataRepositoryException::IOError("There was an error creating the status file: %s", exception.what());
        }

        sts_file.WriteLine(FileCommands::FileHeader);
        sts_file.WriteVersion();

        if( m_statuses != nullptr )
        {
            // write every non-default value
            for( int pass = 0; pass < 3; ++pass )
            {
                bool header_written = false;

                auto write_header = [&](const char* const header)
                {
                    if( !header_written )
                    {
                        sts_file.WriteLine();
                        sts_file.WriteLine(header);
                        header_written = true;
                    }
                };

                for( const auto& [key, status] : *m_statuses )
                {
                    if( pass == 0 )
                    {
                        if( status.verified )
                        {
                            write_header(FileCommands::VerifiedHeader);
                            sts_file.WriteLine(FileCommands::VerifiedKey, NewlineSubstitutor::NewlineToUnicodeNL(key));
                        }
                    }

                    else if( pass == 1 )
                    {
                        if( status.partial_save_mode != PartialSaveMode::None )
                        {
                            write_header(FileCommands::PartialSaveHeader);

                            const char* const mode = ( status.partial_save_mode == PartialSaveMode::Add )    ? FileCommands::PartialSaveModeAdd :
                                                     ( status.partial_save_mode == PartialSaveMode::Modify ) ? FileCommands::PartialSaveModeModify :
                                                                                                               FileCommands::PartialSaveModeVerify;

                            std::string value = FormatText("%s.%s%s.%s.",
                                                           mode, NewlineSubstitutor::NewlineToUnicodeNL(key).c_str(),
                                                           ( status.partial_save_case_item_reference != nullptr ) ? NewlineSubstitutor::NewlineToUnicodeNL(status.partial_save_case_item_reference->GetLevelKey()).c_str() : "",
                                                           m_dictionaryName.c_str());

                            if( status.partial_save_case_item_reference != nullptr )
                            {
                                value.append(status.partial_save_case_item_reference->GetName())
                                     .push_back('.');

                                if( status.partial_save_case_item_reference->HasOccurrences() )
                                {
                                    const std::vector<size_t>& one_based_occurrences = status.partial_save_case_item_reference->GetOneBasedOccurrences();
                                    value.append(FormatText("%d.%d.%d", static_cast<int>(one_based_occurrences[0]),
                                                                        static_cast<int>(one_based_occurrences[1]),
                                                                        static_cast<int>(one_based_occurrences[2])));
                                }
                            }

                            sts_file.WriteLine(FileCommands::PartialSaveKey, value);
                        }
                    }

                    else if( pass == 2 )
                    {
                        if( !status.case_label.empty() )
                        {
                            write_header(FileCommands::CaseLabelHeader);
                            sts_file.WriteLine(FileCommands::CaseLabelKeyLabel, NewlineSubstitutor::NewlineToUnicodeNL(key + status.case_label));
                        }
                    }
                }
            }
        }

        sts_file.Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch(...)
    {
        throw DataRepositoryException::IOError("There was an error writing to the status file.");
    }

    m_hasTransactionsToWrite = false;
}


void TextRepositoryStatusFile::WriteCase(Case& data_case, const WriteCaseParameter* const write_case_parameter)
{
    const std::string& key = data_case.GetKey();
    bool modified = false;

    const bool has_default_attributes = ( !data_case.GetVerified() &&
                                          !data_case.IsPartial() &&
                                          data_case.GetCaseLabel().empty() );

    if( m_statuses != nullptr )
    {
        // see if the key has changed
        if( write_case_parameter != nullptr && write_case_parameter->IsModifyParameter() )
        {
            // remove the previous key since the key has changed
            if( write_case_parameter->GetKey() != key )
            {
                modified = RemoveEntry(write_case_parameter->GetKey());
            }

            // if none of the attributes are different from the expected values, remove any existing entry
            else if( has_default_attributes )
            {
                modified = RemoveEntry(key);
            }
        }
    }

    if( !has_default_attributes )
    {
        if( m_statuses == nullptr )
            m_statuses = std::make_unique<std::map<std::string, Status>>();

        (*m_statuses)[key] = Status
        {
            data_case.GetCaseLabel(),
            data_case.GetVerified(),
            data_case.GetPartialSaveMode(),
            data_case.GetSharedPartialSaveCaseItemReference()
        };

        modified = true;
    }

    if( modified )
        Save();
}


bool TextRepositoryStatusFile::RemoveEntry(const std::string& key)
{
    ASSERT(m_statuses != nullptr);

    if( m_statuses->erase(key) > 0 )
    {
        // when there are no entries, delete the statuses
        if( m_statuses->empty() )
            m_statuses.reset();

        return true;
    }

    return false;
}


void TextRepositoryStatusFile::DeleteCase(const std::string& key)
{
    if( m_statuses != nullptr && RemoveEntry(key) )
        Save();
}
