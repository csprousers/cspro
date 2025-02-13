#pragma once

#include <zDataCLR/DataRepository.h>


namespace CSPro
{
    namespace Data
    {
        public enum class ExporterExportType
        {
            CommaDelimited,
            Excel,
            Json,
            R,
            SAS,
            SPSS,
            Stata
        };

        public ref class Exporter sealed
        {
        public:
            Exporter(CSPro::Data::DataRepository^ repository);

            ~Exporter() { this->!Exporter(); }
            !Exporter();

            void Refresh(System::String^ base_filename, System::Collections::Generic::ICollection<ExporterExportType>^ exporter_export_types, bool one_file_per_record);

            System::String^ GetOutputFilenames();

            System::String^ Export(System::ComponentModel::BackgroundWorker^ background_worker);

        private:
            void AddExportConnectionString(const std::wstring& base_filename, DataRepositoryType type, bool one_file_per_record);

        private:
            ::DataRepository* m_nativeRepository;
            std::vector<std::wstring>* m_recordNames;
            std::vector<::ConnectionString>* m_exportConnectionStrings;
        };
    }
}
