#pragma once

struct sqlite3;
struct sqlite3_stmt;

namespace CSPro
{
    namespace ParadataViewer
    {
        ref class DatabaseQuery;

        public ref class Database sealed
        {
        public:
            Database(System::String^ filename);

            ~Database() { this->!Database(); }
            !Database();

            void ExecuteNonQuery(System::String^ sql);

            int64_t ExecuteSingleQuery(System::String^ sql);

            System::Collections::Generic::List<array<System::Object^>^>^ ExecuteQuery(System::String^ sql);

            DatabaseQuery^ CreateQuery(System::String^ sql);

        private:
            sqlite3* m_db;
            std::vector<sqlite3_stmt*>* m_paStmts;
        };
    }
}
