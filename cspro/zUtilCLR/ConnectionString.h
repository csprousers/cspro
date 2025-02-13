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

        public ref class ConnectionString sealed
        {
        public:
            ConnectionString(::ConnectionString connection_string);
            ConnectionString(System::String^ connection_string_text);

            static ConnectionString^ CreateNullableConnectionString(::ConnectionString connection_string);

            ~ConnectionString() { this->!ConnectionString(); }
            !ConnectionString();

            property System::String^ Filename
            {
                System::String^ get();
            }

            bool FilenameMatches(System::String^ filename);

            property DataRepositoryType Type
            {
                DataRepositoryType get();
            }

            property bool TypeContainsEmbeddedDictionary
            {
                bool get();
            }

            System::String^ ToString() override;

            System::String^ ToRelativeString(System::String^ directory_name);

            void AdjustRelativePath(System::String^ directory_name);

            const ::ConnectionString& GetNativeConnectionString();

            // some methods used by the PFF Editor
            static System::String^ GetDataRepositoryTypeDisplayText(DataRepositoryType type);

            System::String^ ToStringWithModifiedType(DataRepositoryType new_type);

        private:
            ::ConnectionString *m_nativeConnectionString;
        };
    }
}
