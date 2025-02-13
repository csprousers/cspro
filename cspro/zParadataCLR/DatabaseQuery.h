#pragma once

struct sqlite3_stmt;

namespace CSPro
{
    namespace ParadataViewer
    {
        ref class Database;

        public ref class DatabaseQuery sealed
        {
        internal:
            DatabaseQuery(sqlite3_stmt* stmt);

        public:
            property int ColumnCount { int get(); }

            property array<System::String^>^ ColumnNames { array<System::String^>^ get(); }

            System::Collections::Generic::List<array<System::Object^>^>^ GetResults(int iMaxNumberResults);

            property System::Nullable<bool> AdditionalResultsAvailable { System::Nullable<bool> get(); }

        private:
            sqlite3_stmt* m_stmt;
            int m_iNumberColumns;
            bool m_bGetResultsExecutedAtLeastOnce;
            bool m_bNextRowAlreadyStepped;
        };
    }
}
