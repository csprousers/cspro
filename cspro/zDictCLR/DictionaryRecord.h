#pragma once

#include <zDictO/DDClass.h>
#pragma make_public(CDictRecord)

#include <zDictCLR/DictionaryItem.h>

namespace CSPro {

    namespace Dictionary {

        ///<summary>
        ///Record (group of variables) from a data dictionary
        ///</summary>
        public ref class DictionaryRecord sealed
        {
        public:
            DictionaryRecord(CDictRecord* pNativeRecord);

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

            property array<DictionaryItem^>^ Items {
                array<DictionaryItem^>^ get();
            }

            DictionaryItem^ AddItem();

            property System::String^ RecordType {
                System::String^ get();
                void set(System::String^);
            }

            property bool Required {
                bool get();
                void set(bool);
            }

            property int MaxRecs {
                int get();
                void set(int);
            }

            property int Length {
                int get();
                void set(int);
            }

            CDictRecord* GetNativePointer() { return m_pNativeRecord; }

        private:
            CDictRecord* m_pNativeRecord;
        };
    }
}
