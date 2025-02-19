#pragma once

class ConnectionString;
#pragma make_public(ConnectionString)


namespace CSPro
{
    namespace Util
    {
        public enum class DataRepositoryType
        {
            Null,
            Text,
            SQLite,
            EncryptedSQLite,
            Memory,
            Json,
            CSWeb,
            CommaDelimited,
            SemicolonDelimited,
            TabDelimited,
            Excel,
            CSProExport,
            R,
            SAS,
            SPSS,
            Stata
        };

        public enum class DataFileDlgType
        {
            OpenExisting, OpenOrCreate, CreateNew
        };

        public ref class ConnectionString sealed
        {
        public:
            ConnectionString(::ConnectionString connection_string);
            ConnectionString(System::String^ connection_string_text);

            static ConnectionString^ CreateNullableConnectionString(::ConnectionString connection_string);

            ~ConnectionString() { this->!ConnectionString(); }
            !ConnectionString();

            property System::String^ FilePath { System::String^ get(); }

            property DataRepositoryType Type { DataRepositoryType get(); }

            property bool HasResource { bool get(); }
            property bool HasFilePath { bool get(); }

            property bool TypeContainsEmbeddedDictionary { bool get(); }

            System::String^ ToString() override;

            System::String^ ToDisplayString(bool use_filename_only);
            System::String^ ToDisplayString();

            System::String^ ToRelativeString(System::String^ directory_name);

            void AdjustRelativePath(System::String^ directory_name);

            const ::ConnectionString& GetNativeConnectionString();

            // some methods used by the PFF Editor
            static System::String^ GetDataRepositoryTypeDisplayText(DataRepositoryType type);

            System::String^ ToStringWithModifiedType(DataRepositoryType new_type);

            // a way to use the DataFileDlg from .NET
            static ConnectionString^ ShowDataFileDlg(System::IntPtr^ hWndOwner, DataFileDlgType type, bool add_only_readable_types,
                                                     ConnectionString^ connection_string);

        private:
            ::ConnectionString *m_nativeConnectionString;
        };
    }
}
