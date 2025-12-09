#pragma once

namespace Sqlite { class DB; }


namespace CSPro
{
    namespace ParadataViewer
    {
        ref class DatabaseQuery;

        public ref class Database sealed
        {
        public:
            Database(System::String^ file_path);

            ~Database() { this->!Database(); }
            !Database();

            static System::Object^ GetSqlResult(Sqlite::Statement& stmt, int column_number);

            void ExecuteNonQuery(System::String^ sql);

            int64_t ExecuteSingleQuery(System::String^ sql);

            System::Collections::Generic::List<array<System::Object^>^>^ ExecuteQuery(System::String^ sql);

            DatabaseQuery^ CreateQuery(System::String^ sql);

        private:
            Sqlite::DB* m_db;
        };
    }
}
