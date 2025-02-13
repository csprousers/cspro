#include "Stdafx.h"
#include "DataViewerController.h"
#include "Case.h"
#include "DataRepository.h"
#include "DataViewerCaseConstructionReporter.h"
#include "DataViewerSettings.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>
#include <zCaseO/BinaryCaseItem.h>


CSPro::Data::DataViewerController::DataViewerController(DataViewerSettings^ settings, DataRepository^ repository)
    :   m_settings(settings),
        m_repository(repository),
        m_caseMap(new std::map<double, std::shared_ptr<::Case>>),
        m_caseToHtmlConverter(new CaseToHtmlConverter),
        m_caseConstructionReporter(new std::shared_ptr<DataViewerCaseConstructionReporter>),
        m_caseConstructionErrorsMap(new std::map<double, std::vector<std::wstring>>)
{
    m_settings->ApplyCaseToHtmlConverterSettings(*m_caseToHtmlConverter);
    m_caseToHtmlConverter->SetCreateBinaryDataUrls(true);

    *m_caseConstructionReporter = std::make_shared<DataViewerCaseConstructionReporter>();
}


CSPro::Data::DataViewerController::!DataViewerController()
{
    delete m_caseMap;
    delete m_caseToHtmlConverter;
    delete m_caseConstructionReporter;
    delete m_caseConstructionErrorsMap;
}


CSPro::Data::Case^ CSPro::Data::DataViewerController::ReadCase(CaseSummary case_summary)
{
    return ReadCase(case_summary.PositionInRepository);
}


CSPro::Data::Case^ CSPro::Data::DataViewerController::ReadCase(System::String^ case_key, System::String^ case_uuid)
{
    ::DataRepository& repository = *m_repository->GetNativePointer();

    // search first by UUID and then by key
    for( int pass = 0; pass < 2; ++pass )
    {
        try
        {
            CString uuid = ( pass == 0 ) ? case_uuid : CString();

            if( pass == 0 && uuid.IsEmpty() )
                continue;

            CString key = ( pass == 1 ) ? case_key : CString();
            double position_in_repository;

            repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);

            return ReadCase(position_in_repository);
        }

        catch( const DataRepositoryException::Error& exception )
        {
            // only throw the exception after the key search
            if( pass == 1 )
                throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
        }
    }

    // this is here to eliminate a compiler warning
    throw gcnew System::Exception();
}


CSPro::Data::Case^ CSPro::Data::DataViewerController::ReadCase(double position_in_repository)
{
    try
    {
        // see if the case has already been read
        auto case_map_lookup = m_caseMap->find(position_in_repository);

        if( case_map_lookup != m_caseMap->end() )
            return gcnew Case(case_map_lookup->second);

        // otherwise potentially clear unused case entries in the case map lookup
        constexpr size_t SizeToBeginPruning = 100;

        if( m_caseMap->size() >= SizeToBeginPruning )
        {
            for( auto itr = m_caseMap->cbegin(); itr != m_caseMap->cend();  )
            {
                if( itr->second.use_count() == 1 )
                {
                    m_caseConstructionErrorsMap->erase(itr->first);
                    itr = m_caseMap->erase(itr);
                }

                else
                {
                    ++itr;
                }
            }
        }

        // and then read the case
        ::DataRepository& repository = *m_repository->GetNativePointer();

        std::shared_ptr<::Case> data_case = repository.GetCaseAccess()->CreateCase();

        (*m_caseConstructionReporter)->Reset();
        data_case->SetCaseConstructionReporter(*m_caseConstructionReporter);

        repository.ReadCase(*data_case, position_in_repository);

        (*m_caseMap)[position_in_repository] = data_case;
        (*m_caseConstructionErrorsMap)[position_in_repository] = (*m_caseConstructionReporter)->GetErrors();

        return gcnew Case(data_case);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}


void CSPro::Data::DataViewerController::Refresh()
{
    // clear the cached case map
    m_caseMap->clear();
    m_caseConstructionErrorsMap->clear();
}


CSPro::Data::Case^ CSPro::Data::DataViewerController::ReadRefreshedCase(Case^ data_case)
{
    const ::Case& native_data_case = data_case->GetNativeReference();
    ::DataRepository& repository = *m_repository->GetNativePointer();

    CString key;
    CString uuid = native_data_case.GetUuid();
    double position_in_repository;

    for( int pass = 0; pass < 2; ++pass )
    {
        // first try to read the old case using the UUID
        if( pass == 0 )
        {
            if( uuid.IsEmpty() )
                continue;
        }

        // otherwise try to read the old case using the key
        else
        {
            key = native_data_case.GetKey();
            ASSERT(!key.IsEmpty());
        }

        try
        {
            repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);
            return ReadCase(position_in_repository);
        }

        catch( const DataRepositoryException::CaseNotFound& ) { }
    }

    return nullptr;
}


void CSPro::Data::DataViewerController::ChangeSetting(System::Windows::Forms::ToolStripMenuItem^ menu_item, bool called_via_accelerator)
{
    if( menu_item != nullptr )
        m_settings->ChangeSetting(menu_item, called_via_accelerator);

    m_settings->ApplyCaseToHtmlConverterSettings(*m_caseToHtmlConverter);
}


const std::vector<std::byte>& CSPro::Data::DataViewerController::GetBinaryData(Case^ data_case, System::String^ case_item_identifier)
{
    const ::Case& native_data_case = data_case->GetNativeReference();
    const CaseItem* case_item;
    std::unique_ptr<CaseItemIndex> index;
    std::tie(case_item, index) = CaseItemIndex::FromSerializableText(native_data_case, ToWS(case_item_identifier));

    if( case_item != nullptr && case_item->IsTypeBinary() )
    {
        const BinaryData* binary_data = assert_cast<const BinaryCaseItem&>(*case_item).GetBinaryData_noexcept(*index);

        if( binary_data != nullptr )
            return binary_data->GetContent();
    }

    throw CSProException("Error accessing binary data");
}


void CSPro::Data::DataViewerController::SaveBinaryData(Case^ data_case, System::String^ case_item_identifier, System::String^ filename)
{
    try
    {
        const std::vector<std::byte>& binary_data = GetBinaryData(data_case, case_item_identifier);

        FileIO::Write(ToWS(filename), binary_data);
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}


System::String^ CSPro::Data::DataViewerController::GetTemporaryBinaryDataFile(Case^ data_case, System::String^ case_item_identifier, System::String^ suggested_filename)
{
    std::wstring temporary_filename = GetUniqueTempFilename(ToWS(suggested_filename));
    
    System::String^ temporary_filename_string = gcnew System::String(temporary_filename.c_str());

    SaveBinaryData(data_case, case_item_identifier, temporary_filename_string);

    // make the file read-only
    SetFileAttributes(temporary_filename.c_str(), FILE_ATTRIBUTE_READONLY);

    TemporaryFile::RegisterFileForDeletion(temporary_filename);

    return temporary_filename_string;
}
