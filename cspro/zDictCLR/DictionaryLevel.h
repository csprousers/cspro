#pragma once

#include <zDictO/DDClass.h>
#pragma make_public(DictLevel)
#include <zDictCLR/DictionaryRecord.h>

namespace CSPro {

    namespace Dictionary {

        ///<summary>
        ///Level (group of records) from a CSPro data dictionary
        ///</summary>
        public ref class DictionaryLevel sealed
        {
        internal:
            DictLevel* GetNativePointer();

        public:
            DictionaryLevel(DictLevel* pNativeLevel);

            property System::String^ Name {
                System::String^ get();
                void set(System::String^);
            }

            property System::String^ Label {
                System::String^ get();
                void set(System::String^);
            }

            property System::String^ Note {
                System::String^ get();
                void set(System::String^);
            }

            property DictionaryRecord^ IdItems {
                DictionaryRecord^ get();
            }

            property array<DictionaryRecord^>^ Records {
                array<DictionaryRecord^>^ get();
            }

            DictionaryRecord^ AddRecord();

        private:
            DictLevel* m_pNativeLevel;
        };

    }
}
