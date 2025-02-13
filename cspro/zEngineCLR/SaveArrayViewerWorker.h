#pragma once


namespace CSPro
{
    namespace Engine
    {
        class SaveArrayViewerWorkerImpl;

        public ref class SaveArrayViewerWorker sealed
        {
        public:
            SaveArrayViewerWorker(System::String^ save_array_filename, array<System::String^>^ save_array_names);

            ~SaveArrayViewerWorker() { this->!SaveArrayViewerWorker(); }
            !SaveArrayViewerWorker();

            property System::Collections::Hashtable^ ValueSets { System::Collections::Hashtable^ get(); }

            void GetLogicDetails(System::String^ save_array_name, System::Collections::ArrayList^ dimensions, System::Collections::ArrayList^ proc_references);

        private:
            SaveArrayViewerWorkerImpl* m_impl;
        };
    }
}
