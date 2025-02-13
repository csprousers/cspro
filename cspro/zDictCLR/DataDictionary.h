#pragma once

#include <zDictO/DDClass.h>
#pragma make_public(CDataDict)

#include <zDictCLR/DictionaryLevel.h>

namespace CSPro {

    namespace Dictionary {

        ///<summary>
        ///CSPro data dictionary
        ///</summary>
        public ref class DataDictionary sealed
        {
        public:
            static property System::String^ Extension { System::String^ get(); }

            DataDictionary();
            DataDictionary(System::String^ filename);
            DataDictionary(CDataDict* pNativeDict, bool owns_dictionary);
            DataDictionary(CDataDict* pNativeDict);

            ~DataDictionary() { this->!DataDictionary(); }
            !DataDictionary();

            void Save(System::String^ filename);

            property System::String^ Name {
                System::String^ get();
                void set(System::String^);
            }

            property System::String^ Label {
                System::String^ get();
                void set(System::String^);
            }

            property bool AllowDataViewerModifications { bool get(); }
            property bool AllowExport { bool get(); }

            property System::String^ Note {
                System::String^ get();
                void set(System::String^);
            }

            property int RecordTypeStart {
                int get();
                void set(int);
            }

            property int RecordTypeLength {
                int get();
                void set(int);
            }

            property bool ZeroFill {
                bool get();
                void set(bool);
            }

            property array<System::Tuple<System::String^, System::String^>^>^ Languages {
                array<System::Tuple<System::String^, System::String^>^>^ get();
            };

            property array<DictionaryLevel^>^ Levels {
                array<DictionaryLevel^>^ get();
            }

            CDataDict* GetNativePointer();

        private:
            CDataDict* m_pNativeDict;
            bool m_bOwnsDictionary;
        };
    }
}
