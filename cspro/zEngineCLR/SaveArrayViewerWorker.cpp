#include "Stdafx.h"
#include "SaveArrayViewerWorker.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zUtilO/Interapp.h>
#include <zAppO/PFF.h>
#include <zDictO/DDClass.h>
#include <zDictO/DictionaryIterator.h>
#include <zLogicO/KeywordTable.h>
#include <zLogicO/ReservedWords.h>
#include <zLogicO/SourceBuffer.h>


namespace
{
    struct SaveArrayLogicDetails
    {
        std::wstring name;
        std::vector<std::wstring> dimensions;
        std::set<std::wstring> uses;
    };
}


namespace CSPro::Engine
{
    class SaveArrayViewerWorkerImpl
    {
    public:
        SaveArrayViewerWorkerImpl(const std::wstring& save_array_filename, const std::vector<std::wstring>& save_array_names);

        std::vector<const DictValueSet*> GetValueSets() const;

        SaveArrayLogicDetails* GetLogicDetails(wstring_view save_array_name)
        {
            auto save_array_logic_details = std::find_if(m_saveArrayLogicDetails.begin(), m_saveArrayLogicDetails.end(),
                                                         [&](const SaveArrayLogicDetails& sald) { return SO::EqualsNoCase(save_array_name, sald.name); });

            return ( save_array_logic_details != m_saveArrayLogicDetails.end() ) ? &*save_array_logic_details :
                                                                                   nullptr;
        }

    private:
        void ProcessApplication(const std::wstring& application_filename);
        void ProcessLogic(const std::wstring& logic_filename, const LogicSettings& logic_settings);

    private:
        std::vector<SaveArrayLogicDetails> m_saveArrayLogicDetails;
        std::unique_ptr<CDataDict> m_dictionary;
    };
}



// --------------------------------------------------------------------------
// SaveArrayViewerWorker
// --------------------------------------------------------------------------

CSPro::Engine::SaveArrayViewerWorker::SaveArrayViewerWorker(System::String^ save_array_filename, array<System::String^>^ save_array_names)
    :   m_impl(nullptr)
{
    try
    {
        m_impl = new SaveArrayViewerWorkerImpl(ToWS(save_array_filename), ToVectorWS(save_array_names));
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}


CSPro::Engine::SaveArrayViewerWorker::!SaveArrayViewerWorker()
{
    delete m_impl;
}


System::Collections::Hashtable^ CSPro::Engine::SaveArrayViewerWorker::ValueSets::get()
{
    auto valuesets_hashtable = gcnew System::Collections::Hashtable();

    for( const DictValueSet* dict_value_set : m_impl->GetValueSets() )
    {
        valuesets_hashtable->Add(gcnew System::String(dict_value_set->GetName()),
                                 gcnew CSPro::Dictionary::ValueSet(*dict_value_set));
    }

    return valuesets_hashtable;    
}


void CSPro::Engine::SaveArrayViewerWorker::GetLogicDetails(System::String^ save_array_name, System::Collections::ArrayList^ dimensions,
                                                           System::Collections::ArrayList^ proc_references)
{
    ASSERT(dimensions->Count == 0 && proc_references->Count == 0);

    SaveArrayLogicDetails* save_array_logic_details = m_impl->GetLogicDetails(ToWS(save_array_name));

    if( save_array_logic_details == nullptr )
        return;

    for( const std::wstring& dimension : save_array_logic_details->dimensions )
        dimensions->Add(gcnew System::String(dimension.c_str()));

    for( const std::wstring& use : save_array_logic_details->uses )
        proc_references->Add(gcnew System::String(use.c_str()));
}



// --------------------------------------------------------------------------
// SaveArrayViewerWorkerImpl
// --------------------------------------------------------------------------

CSPro::Engine::SaveArrayViewerWorkerImpl::SaveArrayViewerWorkerImpl(const std::wstring& save_array_filename, const std::vector<std::wstring>& save_array_names)
{
    for( const std::wstring& save_array_name : save_array_names )
        m_saveArrayLogicDetails.emplace_back(SaveArrayLogicDetails{ save_array_name });

    // try to find the application that uses this save array file
    std::vector<std::wstring> application_candidates;

    // 1) an application with the .sva extension removed
    std::wstring application_without_sva_filename = PortableFunctions::PathRemoveFileExtension(save_array_filename);

    if( PortableFunctions::FileIsRegular(application_without_sva_filename) )
    {
        application_candidates.emplace_back(std::move(application_without_sva_filename));
    }

    // 2) look for a reference to this file in any of the PFFs
    else
    {
        for( const std::wstring& pff_filename : DirectoryLister().SetNameFilter(FileExtensions::Wildcard::Pff)
                                                                 .GetPaths(PortableFunctions::PathGetDirectory(save_array_filename)) )
        {
            try
            {
                PFF pff;
                pff.SetPifFileName(WS2CS(pff_filename));
                pff.LoadPifFile();

                if( SO::EqualsNoCase(pff.GetSaveArrayFilename(), save_array_filename) &&
                    PortableFunctions::FileIsRegular(pff.GetAppFName()) )
                {
                    application_candidates.emplace_back(CS2WS(pff.GetAppFName()));
                }
            }

            catch(...)
            {
            }
        }
    }

    if( application_candidates.empty() )
        throw CSProException("No application candidates");

    // if more than one application exists, use the one that was most recently modified
    if( application_candidates.size() > 1 )
    {
        std::sort(application_candidates.begin(), application_candidates.end(),
                  [](const std::wstring& ac1, const std::wstring& ac2)
                  { return ( PortableFunctions::FileModifiedTime(ac1) < PortableFunctions::FileModifiedTime(ac2) ); });
    }

    ProcessApplication(application_candidates.back());
}


void CSPro::Engine::SaveArrayViewerWorkerImpl::ProcessApplication(const std::wstring& application_filename)
{
    Application application;
    application.Open(application_filename, true, false);

    // read the dictionary
    const std::wstring& dictionary_filename = application.GetFirstDictionaryFilenameOfType(DictionaryType::Input);

    if( dictionary_filename.empty() )
        throw CSProException("No input dictionary");

    m_dictionary = std::make_unique<CDataDict>();
    m_dictionary->Open(dictionary_filename, true);

    // process the logic
    const CodeFile* logic_main_code_file = application.GetLogicMainCodeFile();

    if( logic_main_code_file == nullptr )
        throw CSProException("No logic");

    ProcessLogic(logic_main_code_file->GetFilename(), application.GetLogicSettings());
}


void CSPro::Engine::SaveArrayViewerWorkerImpl::ProcessLogic(const std::wstring& logic_filename, const LogicSettings& logic_settings)
{
    const std::wstring ProcTokenText = _T("PROC");
    const std::wstring GlobalTokenText = _T("GLOBAL");
    const std::wstring FunctionStartTokenText = SO::ToLower(Logic::KeywordTable::GetKeywordName(TokenCode::TOKKWFUNCTION));
    const std::wstring FunctionEndTokenText = Logic::KeywordTable::GetKeywordName(TokenCode::TOKEND);

    // parse the logic for the array declaration and the PROCs and functions where the array is used
    Logic::SourceBuffer source_buffer(FileIO::ReadText(logic_filename));
    const std::vector<Logic::BasicToken>& basic_tokens = source_buffer.Tokenize(logic_settings);

    std::optional<std::wstring> current_proc_or_function;
    bool in_function = false;

    for( auto basic_token_itr = basic_tokens.cbegin(); basic_token_itr < basic_tokens.cend(); ++basic_token_itr ) 
    {
        auto current_token_is_text = [&]() { return ( basic_token_itr->type == Logic::BasicToken::Type::Text ); };
        auto advance_token = [&]() { return ( ++basic_token_itr < basic_tokens.cend() ); };

        if( !current_token_is_text() )
            continue;

        // if starting a procedure or function, read the name
        if( SO::EqualsOneOfNoCase(basic_token_itr->GetTextSV(), ProcTokenText, FunctionStartTokenText) )
        {
            current_proc_or_function.reset();
            in_function = SO::EqualsNoCase(basic_token_itr->GetTextSV(), FunctionStartTokenText);

            while( advance_token() )
            {
                // only keep track of references in functions and non-GLOBAL procedures
                if( current_token_is_text() )
                {
                    if( SO::EqualsNoCase(basic_token_itr->GetTextSV(), GlobalTokenText) )
                    {
                        break;
                    }

                    else if( !Logic::ReservedWords::IsReservedWord(basic_token_itr->GetTextSV()) )
                    {
                        current_proc_or_function = SO::Concatenate(in_function ? FunctionStartTokenText : ProcTokenText, _T(" "), basic_token_itr->GetTextSV());
                        break;
                    }
                }
            }
        }

        // if ending a function, reset the current function details
        else if( in_function && SO::EqualsNoCase(basic_token_itr->GetTextSV(), FunctionEndTokenText) )
        {
            current_proc_or_function.reset();
            in_function = false;
        }

        // for other text, see if it matches an array name
        else
        {
            SaveArrayLogicDetails* save_array_logic_details = GetLogicDetails(basic_token_itr->GetTextSV());

            if( save_array_logic_details == nullptr )
                continue;

            // if in a procedure or function, this is a use
            if( current_proc_or_function.has_value() )
            {
                save_array_logic_details->uses.insert(*current_proc_or_function);
            }

            // otherwise parse the declaration
            else if( advance_token() && basic_token_itr->GetTextSV() == _T("(") )
            {
                std::wstring parentheses_contents;
                size_t open_parentheses = 1;

                while( advance_token() )
                {
                    if( basic_token_itr->GetTextSV() == _T("(") )
                    {
                        ++open_parentheses;
                    }

                    else if( basic_token_itr->GetTextSV() == _T(")") )
                    {
                        if( --open_parentheses == 0 )
                        {
                            save_array_logic_details->dimensions = SO::SplitString(parentheses_contents, ',');
                            break;
                        }
                    }

                    parentheses_contents.append(basic_token_itr->GetTextSV());
                }
            }
        }
    }
}


std::vector<const DictValueSet*> CSPro::Engine::SaveArrayViewerWorkerImpl::GetValueSets() const
{
    std::vector<const DictValueSet*> dict_value_sets;

    DictionaryIterator::ForeachDictBaseUpTo<DictValueSet>(*m_dictionary,
        [&](const DictBase& dict_base)
        {
            if( dict_base.GetElementType() == DictElementType::ValueSet )
                dict_value_sets.emplace_back(assert_cast<const DictValueSet*>(&dict_base));
        });

    return dict_value_sets;
}
