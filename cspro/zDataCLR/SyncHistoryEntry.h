#pragma once

class SyncHistoryEntry;


namespace CSPro
{
    namespace Data
    {
        ///<summary>
        ///Information about a past synchronization
        ///</summary>
        public ref class SyncHistoryEntry sealed
        {
        public:
            SyncHistoryEntry(const ::SyncHistoryEntry& sync_history_entry);

            ///<summary>
            ///Id of remote device (server) synced with
            ///</summary>
            property System::String^ DeviceId {
                System::String^ get();
            }

            ///<summary>
            ///Name of remote device (server) synced with
            ///</summary>
            property System::String^ DeviceName {
                System::String^ get();
            }

            ///<summary>
            ///Direction of synchronization
            ///</summary>
            property CSPro::Util::SyncDirection Direction {
                CSPro::Util::SyncDirection get();
            }

            ///<summary>
            ///Universe used to limit cases synced
            ///</summary>
            property System::String^ Universe {
                System::String^ get();
            }

            ///<summary>
            ///Date/time of sync
            ///</summary>
            property System::DateTime Time {
                System::DateTime get();
            }


        private:
            System::String^ m_deviceId;
            System::String^ m_deviceName;
            CSPro::Util::SyncDirection m_direction;
            System::String^ m_universe;
            System::DateTime m_time;
        };
    }
}
