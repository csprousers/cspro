#include "StdAfx.h"
#include "PifInfoPopulator.h"
#include <zUtilO/PathHelpers.h>
#include <zAppO/Application.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zDictO/DDClass.h>
#include <zFormO/FormFile.h>
#include <zEngineO/AllSymbols.h>
#include <engine/Engarea.h>


namespace
{
    CString GetDictionaryFilename(const EngineDictionary& engine_dictionary)
    {
        CString dictionary_filename = engine_dictionary.GetDictionary().GetFullFileName();
        
        // when run from a .pen file, the dictionary filename will be blank, in which case the dictionary name can be used
        return ( dictionary_filename.IsEmpty() ) ? WS2CS(engine_dictionary.GetName()) :
                                                   dictionary_filename;
    }

    CString GetDictionaryFilename(const DICT* pDicT)
    {
        CString dictionary_filename = pDicT->GetDataDict()->GetFullFileName();
        
        // when run from a .pen file, the dictionary filename will be blank, in which case the dictionary name can be used
        return ( dictionary_filename.IsEmpty() ) ? WS2CS(pDicT->GetName()) :
                                                   dictionary_filename;
    }
}


PifInfoPopulator::PifInfoPopulator(const EngineData& engine_data, PFF& pff)
    :   m_engineData(&engine_data),
        m_pff(pff),
        m_application(*pff.GetApplication())
{
}


std::vector<std::shared_ptr<PIFINFO>> PifInfoPopulator::GetPifInfo()
{
    if( m_engineData->dictionaries_pre80.empty() ) // UseNewDriver
    {
        // dictionaries that are part of flows (input [and output for batch], and external form data)
        ASSERT(!m_engineData->engine_dictionaries.empty());
        size_t dictionary_index = 0;

        for( const Flow* flow : m_engineData->flows )
        {
            const EngineDictionary& engine_dictionary = flow->GetEngineDictionary();
            ASSERT(m_engineData->engine_dictionaries[dictionary_index] == &engine_dictionary);

            if( flow->GetSubType() == SymbolSubType::Primary )
            {
                AddDictionary(engine_dictionary);

                // output data (for batch applications)    
                if( m_application.GetEngineAppType() == EngineAppType::Batch )
                {
                    // with multiple output data possible, it would be nice to add PIF_MULTIPLE_FILES as a flag, but the MFC dialog
                    // only supports that option for selecting files that exist, so users wanting to specify multiple output files
                    // will have to manually enter the filenames
                    auto output_info = AddInfo(FILE_NONE, OUTPFILE, PIF_USE_REPOSITORY_TYPE | PIF_DISALLOW_CSPRO_EXTENSIONS | PIF_ALLOW_BLANK);
                    output_info->dictionary_filename = GetDictionaryFilename(engine_dictionary);
                    output_info->SetConnectionStrings(m_pff.GetOutputDataConnectionStringsSerializable());
                }
            }

            else
            {
                ASSERT(flow->GetSubType() == SymbolSubType::Secondary);
                AddDictionary(engine_dictionary, WS2CS(flow->GetName()));
            }

            ++dictionary_index;
        }


        // external data (that is not working storage)
        for( ; dictionary_index < m_engineData->engine_dictionaries.size(); ++dictionary_index )
        {
            const EngineDictionary& engine_dictionary = *m_engineData->engine_dictionaries[dictionary_index];

            if( engine_dictionary.IsDictionaryObject() && engine_dictionary.GetSubType() != SymbolSubType::Work )
                AddDictionary(engine_dictionary);
        }
    }


    else // UseOldDriver
    {
        ASSERT(!m_engineData->dictionaries_pre80.empty());
        size_t dictionary_index = 0;

        AddDictionary(m_engineData->dictionaries_pre80[dictionary_index]);

        // output data (for batch applications)    
        if( m_application.GetEngineAppType() == EngineAppType::Batch )
        {
            // with multiple output data possible, it would be nice to add PIF_MULTIPLE_FILES as a flag, but the MFC dialog
            // only supports that option for selecting files that exist, so users wanting to specify multiple output files
            // will have to manually enter the filenames
            auto output_info = AddInfo(FILE_NONE, OUTPFILE, PIF_USE_REPOSITORY_TYPE | PIF_DISALLOW_CSPRO_EXTENSIONS | PIF_ALLOW_BLANK);
            output_info->dictionary_filename = GetDictionaryFilename(m_engineData->dictionaries_pre80[dictionary_index]);
            output_info->SetConnectionStrings(m_pff.GetOutputDataConnectionStringsSerializable());
        }

        ++dictionary_index;


        // external form file data (for entry applications)
        if( m_application.GetEngineAppType() == EngineAppType::Entry )
        {
            --dictionary_index;

            for( const FLOW* flow : m_engineData->flows_pre80 )
            {
                const auto& form_file = flow->GetFormFile();

                if( form_file != nullptr )
                {
                    ASSERT(m_engineData->dictionaries_pre80[dictionary_index]->GetDataDict() == form_file->GetDictionary());

                    if( flow->GetSubType() == SymbolSubType::Secondary )
                    {
                        AddDictionary(m_engineData->dictionaries_pre80[dictionary_index], form_file->GetName());
                    }

                    else
                    {
                        ASSERT(dictionary_index == 0);
                    }

                    ++dictionary_index;
                }
            }
        }


        // external data (that is not working storage)
        for( ; dictionary_index < m_engineData->dictionaries_pre80.size(); ++dictionary_index )
        {
            if( m_engineData->dictionaries_pre80[dictionary_index]->GetSubType() != SymbolSubType::Work )
                AddDictionary(m_engineData->dictionaries_pre80[dictionary_index]);
        }
    }


    // write file
    if( m_application.GetHasWriteStatements() )
    {
        AddInfo(FILE_NONE, WRITEFILE, PIF_ALLOW_BLANK)->sFileName = m_pff.GetWriteFName();
    }

    else
    {
        m_pff.SetWriteFName(_T(""));
    }


    // listing file (which will only be shown in entry if the filename has been specified)
    if( m_application.GetEngineAppType() != EngineAppType::Entry || !m_pff.GetListingFName().IsEmpty() )
    {
        AddInfo(FILE_NONE, LISTFILE, 0)->sFileName = GetFilenameOrDefaultFilename(m_pff.GetListingFName(),
            [] { return CString(_T('.')) + WinSettings::Read<CString>(WinSettings::Type::ListingFilenameExtension, FileExtensions::Listing); });
    }


    // frequencies
    if( m_application.GetHasSaveableFrequencyStatements() )
    {
        AddInfo(FILE_NONE, FREQFILE, PIF_ALLOW_BLANK)->sFileName = m_pff.GetFrequenciesFilename();
    }

    else
    {
        m_pff.SetFrequenciesFilename(_T(""));
    }


    // impute frequencies
    if( m_application.GetHasImputeStatements() )
    {
        AddInfo(FILE_NONE, IMPUTEFILE, PIF_ALLOW_BLANK)->sFileName = GetFilenameOrDefaultFilename(m_pff.GetImputeFrequenciesFilename(), [&]
            {
                CString default_frequency_extension = WinSettings::Read<CString>(WinSettings::Type::FrequencyFilenameExtension, FileExtensions::Listing);
                return ( CString(_T(".impute_freq.")) + default_frequency_extension );
            });
    }

    else
    {
        m_pff.SetImputeFrequenciesFilename(_T(""));
    }


    // impute stat data file
    if( m_application.GetHasImputeStatStatements() )
    {
        auto stat_data_info = AddInfo(FILE_NONE, IMPUTESTATFILE, PIF_USE_REPOSITORY_TYPE | PIF_DISALLOW_CSPRO_EXTENSIONS | PIF_ALLOW_BLANK);

        // the stat dictionary does not exist yet, but to allow the dialog to treat this as a
        // connection string, we will make up a fake dictionary filename
        stat_data_info->dictionary_filename = GetDictionaryFilename(m_engineData->dictionaries_pre80.front()) + IMPUTESTATFILE;

        stat_data_info->SetConnectionString(m_pff.GetImputeStatConnectionString().IsDefined() ?
            m_pff.GetImputeStatConnectionString() : 
            ConnectionString(GetFilenameOrDefaultFilename(CString(), [] { return CString(_T(".impute_stat.")) + FileExtensions::Data::CSProDB; })));
    }

    else
    {
        m_pff.SetImputeStatConnectionString(_T(""));
    }


    // save array file
    if( m_application.GetHasSaveArrays() )
    {
        AddInfo(FILE_NONE, SAVEARRAYFILE, PIF_ALLOW_BLANK)->sFileName =
            GetFilenameOrDefaultFilename(m_pff.GetSaveArrayFilename(), [] { return FileExtensions::WithDot::SaveArray; });
    }

    else
    {
        m_pff.SetSaveArrayFilename(_T(""));
    }


    // paradata log
    if( m_application.GetApplicationProperties().GetParadataProperties().GetCollectionType() != ParadataProperties::CollectionType::No )
    {
        // suggest a default paradata filename only if the PFF doesn't exist
        AddInfo(FILE_NONE, PARADATAFILE, PIF_ALLOW_BLANK)->sFileName =
            PortableFunctions::FileExists(m_pff.GetPifFileName()) ? m_pff.GetParadataFilename() :
                GetFilenameOrDefaultFilename(m_pff.GetParadataFilename(), [] { return FileExtensions::WithDot::Paradata; });
    }

    else
    {
        m_pff.SetParadataFilename(_T(""));
    }
        

    // external files (in sorted order)
    std::vector<std::wstring> file_handler_names;

    for( const LogicFile* logic_file : m_engineData->files_global_visibility )
    {
        if( logic_file->IsUsed() )
            file_handler_names.emplace_back(logic_file->GetName());
    }

    std::sort(file_handler_names.begin(), file_handler_names.end());

    for( const std::wstring& file_symbol_name : file_handler_names )
        AddInfo(PIFUSRFILE, WS2CS(file_symbol_name), PIF_ALLOW_BLANK)->sFileName = m_pff.LookUpUsrDatFile(WS2CS(file_symbol_name));

    return m_pifInfo;
};


std::shared_ptr<PIFINFO> PifInfoPopulator::AddInfo(const FILETYPE& type, const CString& name, const UINT& options)
{
    auto info = m_pifInfo.emplace_back(std::make_shared<PIFINFO>());
    info->eType = type;
    info->sUName = name;
    info->sDisplay = name;
    info->uOptions = options;
    return info;
}


void PifInfoPopulator::AddDictionary(const EngineDictionary& engine_dictionary, const TCHAR* external_form_name/* = nullptr*/)
{
    auto dictionary_info = AddInfo(PIFDICT, WS2CS(engine_dictionary.GetName()), PIF_USE_REPOSITORY_TYPE | PIF_DISALLOW_CSPRO_EXTENSIONS);
    dictionary_info->dictionary_filename = GetDictionaryFilename(engine_dictionary);
        
    if( engine_dictionary.GetSubType() == SymbolSubType::Input )
    {
        dictionary_info->sDisplay = _T("Input Data");
        dictionary_info->uOptions |= PIF_REPOSITORY_MUST_BE_READABLE;

        if( m_application.GetEngineAppType() == EngineAppType::Batch )
            dictionary_info->uOptions |= PIF_MULTIPLE_FILES | PIF_FILE_MUST_EXIST | PIF_READ_ONLY;

        dictionary_info->SetConnectionStrings(m_pff.GetInputDataConnectionStringsSerializable());
    }

    else
    {
        if( external_form_name != nullptr )
        {
            dictionary_info->sDisplay.Format(_T("External Form Data (%s)"), external_form_name);
            dictionary_info->uOptions |= PIF_REPOSITORY_MUST_BE_READABLE;
        }

        else
        {
            dictionary_info->sDisplay.Format(_T("External Data (%s)"), engine_dictionary.GetName().c_str());
        }

        dictionary_info->SetConnectionString(m_pff.GetExternalDataConnectionString(WS2CS(engine_dictionary.GetName())));
    }
}

void PifInfoPopulator::AddDictionary(const CSymbolDict* pDicT, const TCHAR* external_form_name/* = nullptr*/)
{
    auto dictionary_info = AddInfo(PIFDICT, WS2CS(pDicT->GetName()), PIF_USE_REPOSITORY_TYPE | PIF_DISALLOW_CSPRO_EXTENSIONS);
    dictionary_info->dictionary_filename = GetDictionaryFilename(pDicT);
        
    if( pDicT->GetSubType() == SymbolSubType::Input )
    {
        dictionary_info->sDisplay = _T("Input Data");
        dictionary_info->uOptions |= PIF_REPOSITORY_MUST_BE_READABLE;

        if( m_application.GetEngineAppType() == EngineAppType::Batch )
            dictionary_info->uOptions |= PIF_MULTIPLE_FILES | PIF_FILE_MUST_EXIST | PIF_READ_ONLY;

        dictionary_info->SetConnectionStrings(m_pff.GetInputDataConnectionStringsSerializable());
    }

    else
    {
        if( external_form_name != nullptr )
        {
            dictionary_info->sDisplay.Format(_T("External Form Data (%s)"), external_form_name);
            dictionary_info->uOptions |= PIF_REPOSITORY_MUST_BE_READABLE;
        }

        else
        {
            dictionary_info->sDisplay.Format(_T("External Data (%s)"), pDicT->GetName().c_str());
        }

        dictionary_info->SetConnectionString(m_pff.GetExternalDataConnectionString(WS2CS(pDicT->GetName())));
    }
}


template<typename ExtensionCallback>
CString PifInfoPopulator::GetFilenameOrDefaultFilename(const CString& filename, ExtensionCallback extension_callback) const
{
    if( !filename.IsEmpty() )
        return filename;

    const CString& application_filename = m_application.GetApplicationFilename();

    return PathHelpers::GetFilenameInDirectory(
        PortableFunctions::PathGetFilenameWithoutExtension<CString>(application_filename) + extension_callback(),
        application_filename);
}
