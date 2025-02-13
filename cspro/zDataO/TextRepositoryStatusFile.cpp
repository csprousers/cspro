#include "stdafx.h"
#include "TextRepositoryStatusFile.h"
#include "CaseIterator.h"
#include "TextRepository.h"
#include <zUtilO/Specfile.h>
#include <zCaseO/CaseConstructionHelpers.h>


namespace TextRepositoryStatusFileCommands
{
    constexpr const TCHAR* FileHeader = _T("[KeyInfo]");

    constexpr const TCHAR* VerifiedHeader = _T("[Verified]");
    constexpr const TCHAR* VerifiedKey    = _T("VerifiedKey");

    constexpr const TCHAR* OldStyleVerifiedHeader  = _T("[LastVerified]");
    constexpr const TCHAR* OldStyleLastVerifiedKey = _T("NodeKey");

    constexpr const TCHAR* PartialSaveHeader         = _T("[PartialNodes]");
    constexpr const TCHAR* PartialSaveKey            = _T("Pos");
    constexpr const TCHAR* PartialSaveTokenDelimiter = _T(".");
    constexpr const TCHAR* PartialSaveModeAdd        = _T("ADD");
    constexpr const TCHAR* PartialSaveModeModify     = _T("MOD");
    constexpr const TCHAR* PartialSaveModeVerify     = _T("VER");

    constexpr const TCHAR* CaseLabelHeader   = _T("[CaseLabel]");
    constexpr const TCHAR* CaseLabelKeyLabel = _T("KeyLabel");
};

namespace FileCommands = TextRepositoryStatusFileCommands;


TextRepositoryStatusFile::TextRepositoryStatusFile(TextRepository& repository, DataRepositoryOpenFlag open_flag)
    :   m_filename(GetStatusFilename(repository.GetConnectionString())),
        m_dictionaryName(repository.GetCaseAccess()->GetDataDict().GetName()),
        m_useTransactionManager(repository.m_useTransactionManager),
        m_hasTransactionsToWrite(false)
{
    if( PortableFunctions::FileIsRegular(m_filename) )
    {
        ASSERT(open_flag != DataRepositoryOpenFlag::CreateNew);
        Load(repository);
    }
}


TextRepositoryStatusFile::~TextRepositoryStatusFile()
{
    ASSERT(!m_hasTransactionsToWrite);
}


std::wstring TextRepositoryStatusFile::GetStatusFilename(const ConnectionString& connection_string)
{
    return connection_string.GetFilename() + FileExtensions::Data::WithDot::TextStatus;
}


void TextRepositoryStatusFile::Load(TextRepository& repository)
{
    CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(InvalidLineException, "")

    CString command;
    CString argument;

    const int key_length = repository.m_keyMetadata->key_length;
    const bool load_statuses = repository.GetCaseAccess()->GetUsesStatuses();
    const bool load_case_labels = repository.GetCaseAccess()->GetUsesCaseLabels();

    try
    {
        CSpecFile sts_file;

        if( !sts_file.Open(m_filename.c_str(), CFile::modeRead) )
            throw DataRepositoryException::IOError(_T("There was an error opening the status file."));

        enum class ProcessingSection { None, Verified, OldStyleVerified, PartialSaves, CaseLabels };
        ProcessingSection processing_section = ProcessingSection::None;

        while( sts_file.GetLine(command, argument, false) == SF_OK )
        {
            command.Trim();

            if( command.IsEmpty() )
                continue;

            // turn ␤ -> \n
            ASSERT(!SO::ContainsNewlineCharacter(command));
            NewlineSubstitutor::MakeUnicodeNLToNewline(argument);

            if( SO::EqualsNoCase(command, FileCommands::FileHeader) )
            {
                if( !sts_file.IsVersionOK(CSPRO_VERSION) )
                {
                    // we will ignore version errors
                    // throw DataRepositoryException::IOError(_T("The status file is from a newer version of CSPro and cannot be read."));
                }
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
                    if( load_statuses && repository.m_requiresIndex )
                    {
                        CaseKey case_key;
                        std::vector<CString> keys;
                        bool key_found = false;

                        auto case_key_iterator = repository.CreateCaseKeyIterator(CaseIterationMethod::SequentialOrder, CaseIterationOrder::Ascending);

                        while( !key_found && case_key_iterator->NextCaseKey(case_key) )
                        {
                            keys.emplace_back(case_key.GetKey());
                            key_found = ( case_key.GetKey().Compare(argument) == 0 );
                        }

                        if( key_found )
                        {
                            for( const CString& key : keys )
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
                        CString parameters[4];
                        size_t occurrences[3] = { 0, 0, 0 };

                        int processing_element = 0;
                        int current_token_pos = 0;

                        CString token = argument.Tokenize(FileCommands::PartialSaveTokenDelimiter, current_token_pos);

                        while( !token.IsEmpty() && processing_element < 7 )
                        {
                            if( processing_element < 4 )
                            {
                                parameters[processing_element] = token;
                            }

                            else
                            {
                                occurrences[processing_element - 4] = std::max(_ttoi(token) - 1, 0);
                            }

                            ++processing_element;
                            token = argument.Tokenize(FileCommands::PartialSaveTokenDelimiter, current_token_pos);
                        }

                        // check that the processed line is valid, which means that...

                        // there must be 3 (if no field information), 4 (if no occurrences), or 7 elements
                        if( processing_element != 3 && processing_element != 4 && processing_element != 7 )
                            throw InvalidLineException();

                        // the partial save mode must be valid
                        PartialSaveMode partial_save_mode;

                        if( parameters[0].CompareNoCase(FileCommands::PartialSaveModeAdd) == 0 )
                        {
                            partial_save_mode = PartialSaveMode::Add;
                        }

                        else if( parameters[0].CompareNoCase(FileCommands::PartialSaveModeModify) == 0 )
                        {
                            partial_save_mode = PartialSaveMode::Modify;
                        }

                        else if( parameters[0].CompareNoCase(FileCommands::PartialSaveModeVerify) == 0 )
                        {
                            partial_save_mode = PartialSaveMode::Verify;
                        }

                        else
                        {
                            throw InvalidLineException();
                        }

                        // the key must be equal to or bigger than the first-level dictionary key
                        CString key = parameters[1];
                        CString level_key;

                        if( key.GetLength() < key_length )
                            throw InvalidLineException();

                        if( key.GetLength() > key_length )
                        {
                            level_key = key.Mid(key_length);
                            key.Truncate(key_length);
                        }

                        // the dictionary name must match the repository's dictionary name
                        if( parameters[2].CompareNoCase(m_dictionaryName) != 0 )
                            throw InvalidLineException();

                        std::shared_ptr<CaseItemReference> partial_save_case_item_reference;

                        if( processing_element > 3 )
                        {
                            partial_save_case_item_reference = CaseConstructionHelpers::CreateCaseItemReference(*repository.GetCaseAccess(),
                                level_key, parameters[3], occurrences);
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
                        if( argument.GetLength() < key_length )
                            throw InvalidLineException();

                        CString key = argument.Left(key_length);
                        GetOrCreateStatus(key).case_label = argument.Mid(key_length);
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
        throw DataRepositoryException::IOError(FormatText(_T("The status file had an invalid line: %s=%s"), command.GetString(), argument.GetString()));
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch(...)
    {
        throw DataRepositoryException::IOError(_T("There was an error reading the status file."));
    }
}


void TextRepositoryStatusFile::CommitTransactions()
{
    ASSERT(m_useTransactionManager);

    if( m_hasTransactionsToWrite )
        Save(true);
}


void TextRepositoryStatusFile::Save(bool force_write_to_disk/* = false*/)
{
    if( m_useTransactionManager && !force_write_to_disk )
    {
        m_hasTransactionsToWrite = true;
        return;
    }

    try
    {
        CSpecFile sts_file;

        if( !sts_file.Open(m_filename.c_str(), CFile::modeWrite) )
            throw DataRepositoryException::IOError(_T("There was an error creating the status file."));

        sts_file.PutLine(FileCommands::FileHeader);
        sts_file.PutLine(CMD_VERSION, CSPRO_VERSION);

        if( m_statuses != nullptr )
        {
            // write every non-default value
            for( int pass = 0; pass < 3; ++pass )
            {
                bool header_written = false;

                auto write_header = [&](const TCHAR* header)
                {
                    if( !header_written )
                    {
                        sts_file.PutLine(_T(""));
                        sts_file.PutLine(header);
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
                            sts_file.PutLine(FileCommands::VerifiedKey, NewlineSubstitutor::NewlineToUnicodeNL(key));
                        }
                    }

                    else if( pass == 1 )
                    {
                        if( status.partial_save_mode != PartialSaveMode::None )
                        {
                            write_header(FileCommands::PartialSaveHeader);

                            const TCHAR* const mode = ( status.partial_save_mode == PartialSaveMode::Add )    ? FileCommands::PartialSaveModeAdd :
                                                      ( status.partial_save_mode == PartialSaveMode::Modify ) ? FileCommands::PartialSaveModeModify :
                                                                                                                FileCommands::PartialSaveModeVerify;

                            CString line = FormatText(_T("%s=%s.%s%s.%s."), FileCommands::PartialSaveKey,
                                                      mode, NewlineSubstitutor::NewlineToUnicodeNL(key).GetString(),
                                                      ( status.partial_save_case_item_reference != nullptr ) ? NewlineSubstitutor::NewlineToUnicodeNL(status.partial_save_case_item_reference->GetLevelKey()).GetString() : _T(""),
                                                      m_dictionaryName.GetString());

                            if( status.partial_save_case_item_reference != nullptr )
                            {
                                line.AppendFormat(_T("%s."), status.partial_save_case_item_reference->GetName().GetString());

                                if( status.partial_save_case_item_reference->HasOccurrences() )
                                {
                                    const std::vector<size_t>& one_based_occurrences = status.partial_save_case_item_reference->GetOneBasedOccurrences();
                                    line.AppendFormat(_T("%d.%d.%d"), static_cast<int>(one_based_occurrences[0]),
                                                                      static_cast<int>(one_based_occurrences[1]),
                                                                      static_cast<int>(one_based_occurrences[2]));
                                }
                            }

                            sts_file.PutLine(line);
                        }
                    }

                    else if( pass == 2 )
                    {
                        if( !status.case_label.IsEmpty() )
                        {
                            write_header(FileCommands::CaseLabelHeader);
                            sts_file.PutLine(FileCommands::CaseLabelKeyLabel, NewlineSubstitutor::NewlineToUnicodeNL(key + status.case_label));
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
        throw DataRepositoryException::IOError(_T("There was an error writing to the status file."));
    }

    m_hasTransactionsToWrite = false;
}


void TextRepositoryStatusFile::WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter)
{
    const CString& key = data_case.GetKey();
    bool modified = false;

    const bool has_default_attributes = !data_case.GetVerified() &&
                                        !data_case.IsPartial() &&
                                        data_case.GetCaseLabel().IsEmpty();

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
            m_statuses = std::make_unique<std::map<CString, Status>>();

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


bool TextRepositoryStatusFile::RemoveEntry(const CString& key)
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


void TextRepositoryStatusFile::DeleteCase(const CString& key)
{
    if( m_statuses != nullptr && RemoveEntry(key) )
        Save();
}
