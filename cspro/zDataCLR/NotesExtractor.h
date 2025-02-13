#pragma once

#include <zDataCLR/DataRepository.h>

namespace CSPro
{
    namespace Data
    {
        public ref class NotesExtractor sealed
        {
        public:
            /// <summary>
            /// Save a data repository's notes to a new data file. Returns a string detailing the number of notes extracted.
            /// </summary>
            System::String^ Extract(System::ComponentModel::BackgroundWorker^ background_worker,
                CSPro::Data::DataRepository^ input_repository, CSPro::Util::ConnectionString^ notes_connection_string);
        };
    }
}
