#pragma once

#include <zDataCLR/CaseSummary.h>

class Case;
class CaseToHtmlConverter;
class DataViewerCaseConstructionReporter;


namespace CSPro
{
    namespace Data
    {
        ref class Case;
        ref class DataRepository;
        ref class DataViewerSettings;

        public ref class DataViewerController sealed
        {
        public:
            DataViewerController(DataViewerSettings^ settings, DataRepository^ repository);

            ~DataViewerController() { this->!DataViewerController(); }
            !DataViewerController();

            Case^ ReadCase(CaseSummary case_summary);

            Case^ ReadCase(System::String^ case_key, System::String^ case_uuid);

            void Refresh();

            Case^ ReadRefreshedCase(Case^ data_case);

            void ChangeSetting(System::Windows::Forms::ToolStripMenuItem^ menu_item, bool called_via_accelerator);

            // methods used by the DataViewerHtmlProvider
            DataRepository^ GetDataRepository()           { return m_repository; }
            DataViewerSettings^ GetSettings()             { return m_settings; }
            CaseToHtmlConverter& GetCaseToHtmlConverter() { return *m_caseToHtmlConverter; }
            const std::map<double, std::vector<std::wstring>>& GetCaseConstructionErrorsMap() { return *m_caseConstructionErrorsMap; }

            // methods used for binary data retrieval
            const std::vector<std::byte>& GetBinaryData(Case^ data_case, System::String^ case_item_identifier);

            void SaveBinaryData(Case^ data_case, System::String^ case_item_identifier, System::String^ filename);

            System::String^ GetTemporaryBinaryDataFile(Case^ data_case, System::String^ case_item_identifier, System::String^ suggested_filename);

        private:
            Case^ ReadCase(double position_in_repository);

            DataViewerSettings^ m_settings;
            DataRepository^ m_repository;
            std::map<double, std::shared_ptr<::Case>>* m_caseMap;
            CaseToHtmlConverter* m_caseToHtmlConverter;
            std::shared_ptr<DataViewerCaseConstructionReporter>* m_caseConstructionReporter;
            std::map<double, std::vector<std::wstring>>* m_caseConstructionErrorsMap;
        };
    }
}
