#pragma once

class LogicArray;


namespace CSPro
{
    namespace Engine
    {
        public ref class SaveArrayValues sealed
        {
        public:
            System::String^ Name;
            System::Collections::Generic::List<int>^ Dimensions;
            bool Numeric;
            int PaddingStringLength;
            int Runs;
            int Cases;

            array<System::String^>^ Values;
            array<int>^ Gets;
            array<int>^ Puts;
        };

        public ref class SaveArrayFile sealed
        {
        public:
            SaveArrayFile();

            ~SaveArrayFile() { this->!SaveArrayFile(); }
            !SaveArrayFile();

            void Load(System::String^ filename) { LoadOrSave(filename, true); }
            void Save(System::String^ filename) { LoadOrSave(filename, false); }

            System::Collections::Generic::List<SaveArrayValues^>^ GetSaveArrayValues();
            void SetSaveArrayValues(System::Collections::Generic::List<SaveArrayValues^>^ list_save_array_values);

        private:
            void LoadOrSave(System::String^ filename, bool loading);

        private:
            std::vector<LogicArray*>* m_logicArrays;
        };
    }
}
