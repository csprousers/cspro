#pragma once

#include <zDictO/DDClass.h>
#pragma make_public(CDictItem)

#include <zDictCLR/ValueSet.h>

namespace CSPro
{
    namespace Dictionary
    {
        public enum class ItemType    { Item, Subitem };
        public enum class ContentType { Numeric = 0, Alpha = 1, Document = 2, Audio = 3, Image = 4 };

        ///<summary>
        ///Item (variable) from a CSPro data dictionary
        ///</summary>
        public ref class DictionaryItem sealed
        {
        public:
            DictionaryItem(CDictItem* pNativeItem);

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

            property int Start {
                int get();
                void set(int);
            }

            property int Length {
                int get();
                void set(int);
            }

            property int Occurrences {
                int get();
                void set(int);
            }

            property ItemType ItemType {
                CSPro::Dictionary::ItemType get();
                void set(CSPro::Dictionary::ItemType);
            }

            property ContentType ContentType {
                CSPro::Dictionary::ContentType get();
                void set(CSPro::Dictionary::ContentType);
            }

            property bool DecimalChar {
                bool get();
                void set(bool);
            }

            property int DecimalPlaces {
                int get();
                void set(int);
            }

            property bool ZeroFill {
                bool get();
                void set(bool);
            }

            property array<ValueSet^>^ ValueSets {
                array<ValueSet^>^ get();
            }

            CDictItem* GetNativePointer() { return m_pNativeItem; }

        private:
            CDictItem* m_pNativeItem;
        };
    }
}
