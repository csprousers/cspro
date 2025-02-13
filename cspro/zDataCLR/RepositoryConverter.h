#pragma once

#include <zDataCLR/DataRepository.h>


namespace CSPro
{
    namespace Data
    {
        public ref class RepositoryConverter sealed
        {
        public:
            /// <summary>
            /// Save a data repository to a new file (and likely a new repository type). Returns a string
            /// detailing the number of cases converted and any errors encountering during the conversion.
            /// </summary>
            System::String^ Convert(System::ComponentModel::BackgroundWorker^ background_worker,
                CSPro::Data::DataRepository^ input_repository, CSPro::Util::ConnectionString^ output_connection_string);
        };
    }
}
