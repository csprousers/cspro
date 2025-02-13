#pragma once


namespace CSPro
{
    namespace Sync
    {
        public ref class DropboxKeys sealed
        {
        public:
            static property System::String^ client_id
            {
                System::String^ get();
            }

            static property System::String^ client_secret
            {
                System::String^ get();
            }
        };
    }
}
