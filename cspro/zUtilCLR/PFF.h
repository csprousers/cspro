#pragma once

#include <zUtilCLR/ConnectionString.h>

class PFF;


namespace CSPro
{
    namespace Util
    {
        public enum class AppType
        {
            ENTRY_TYPE = 0,
            BATCH_TYPE,
            TAB_TYPE,
            FREQ_TYPE,
            SORT_TYPE,
            EXPORT_TYPE,
            REFORMAT_TYPE,
            COMPARE_TYPE,
            CONCAT_TYPE,
            PACK_TYPE,
            INDEX_TYPE,
            SYNC_TYPE,
            PARADATA_CONCAT_TYPE,
            EXCEL2CSPRO_TYPE,
            DEPLOY_TYPE,
            VIEW_TYPE,
            INVALID_TYPE
        };

        public enum class DeployToOverride { None, CSWeb, Dropbox, FTP, LocalFile, LocalFolder };

        public enum class SyncDirection { PUT = 1, GET = 2, BOTH = 3 };

        public enum class SyncServerType { CSWeb = 0, Dropbox = 1, FTP = 2, LocalDropbox = 3, LocalFiles = 4 };


        ///<summary>
        ///CSPro Program Information File (PFF file)
        ///</summary>
        public ref class PFF sealed
        {
        public:
            PFF();
            PFF(System::String^ filename);

            ~PFF() { this->!PFF(); }
            !PFF();

            static property System::String^ Extension
            {
                System::String^ get();
            }

            bool Load();
            bool Save(System::String^ filename);

            void ExecuteOnExitPff();

            property System::String^ PffFilename
            {
                System::String^ get();
                void set(System::String^ value);
            }

            property CSPro::Util::AppType AppType
            {
                CSPro::Util::AppType get();
                void set(CSPro::Util::AppType type);
            }

            property System::String^ AppFName
            {
                System::String^ get();
                void set(System::String^);
            }

            property System::String^ ExcelFilename
            {
                System::String^ get();
            }

            property System::String^ InputDictFName
            {
                System::String^ get();
            }

            property CSPro::Util::ConnectionString^ SingleOutputDataConnectionString
            {
                CSPro::Util::ConnectionString^ get();
            }

            property array<System::String^>^ ExternalDictionaryNames
            {
                array<System::String^>^ get();
            }

            CSPro::Util::ConnectionString^ GetExternalDataConnectionString(System::String^ dictionary_name);

            property System::String^ SyncUrl
            {
                System::String^ get();
                void set(System::String^ value);
            }

            property CSPro::Util::SyncDirection SyncDirection
            {
                CSPro::Util::SyncDirection get();
                void set(CSPro::Util::SyncDirection value);
            }

            property CSPro::Util::SyncServerType SyncServerType
            {
                CSPro::Util::SyncServerType get();
                void set(CSPro::Util::SyncServerType value);
            }

            property bool Silent
            {
                bool get();
            }

            property System::Collections::Generic::List<System::String^>^ CustomParameterMappings
            {
                System::Collections::Generic::List<System::String^>^ get();
            };

            property CSPro::Util::DeployToOverride DeployToOverride
            {
                CSPro::Util::DeployToOverride get();
                void set(CSPro::Util::DeployToOverride value);
            }

        private:
            ::PFF* m_pff;
        };
    }
}
