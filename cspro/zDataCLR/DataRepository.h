#pragma once

#include <zDataO/DataRepository.h>
#pragma make_public(DataRepository)

#include <zDataCLR/CaseSummary.h>
#include <zDataCLR/SyncHistoryEntry.h>

namespace CSPro
{
    namespace Data
    {
        ref class DataViewerSettings;

        /// <summary>
        /// CSPro data source (data file)
        /// </summary>
        public ref class DataRepository sealed
        {
        public:

            DataRepository();

            ~DataRepository()
            {
                this->!DataRepository();
            }

            !DataRepository()
            {
                delete m_caseAccess;
                delete m_pNativeRepo;
            }

            /// <summary>
            /// Open an existing data repository file read-only.
            /// </summary>
            void OpenForReading(CSPro::Dictionary::DataDictionary^ dictionary, CSPro::Util::ConnectionString^ connection_string);

            /// <summary>
            /// Open an existing data repository file allowing read and write.
            /// </summary>
            void OpenForReadingAndWriting(CSPro::Dictionary::DataDictionary^ dictionary, CSPro::Util::ConnectionString^ connection_string);

            /// <summary>
            /// Create a new data repository file allowing read and write.
            /// </summary>
            void CreateForReadingAndWriting(CSPro::Dictionary::DataDictionary^ dictionary, CSPro::Util::ConnectionString^ connection_string);

            /// <summary>
            /// Closes a repository.
            /// </summary>
            void Close();

            /// <summary>
            /// The repository's concise name.
            /// </summary>
            property System::String^ ConciseName { System::String^ get(); }

            /// <summary>
            /// Total number of non-deleted cases in the data file.
            /// </summary>
            property int NumberCases { int get(); }

            /// <summary>
            /// Data dictionary describing data layout in the data file.
            /// </summary>
            property CSPro::Dictionary::DataDictionary^ Dictionary { CSPro::Dictionary::DataDictionary^ get(); }

            /// <summary>
            /// True if this repository supports data synchronization.
            /// </summary>
            property bool Syncable { bool get(); }


            int GetNumberCases(DataViewerSettings^ data_viewer_settings);

            System::Collections::Generic::List<CSPro::Data::CaseSummary>^ GetCaseSummaries(DataViewerSettings^ data_viewer_settings,
                int start_index, int number_case_summaries);

            /// <summary>
            /// Get history of syncs for the repository
            /// </summary>
            array<CSPro::Data::SyncHistoryEntry^>^ GetSyncHistory();

            /// <summary>
            /// Returns a repository's embedded dictionary, if available, returning null if not.
            /// </summary>
            static CSPro::Dictionary::DataDictionary^ GetEmbeddedDictionary(CSPro::Util::ConnectionString^ connection_string);

            ::DataRepository* GetNativePointer();

        private:
            void CreateAndOpen(CSPro::Dictionary::DataDictionary^ dictionary, CSPro::Util::ConnectionString^ connection_string,
                DataRepositoryAccess eAccess, DataRepositoryOpenFlag eOpenFlag);

            std::shared_ptr<CaseAccess>* m_caseAccess;
            ::DataRepository* m_pNativeRepo;

            // Need to hold reference to the dictionary so that it does not get
            // garbage collected while we use the repo
            CSPro::Dictionary::DataDictionary^ m_dictionary;
        };
    }
}
