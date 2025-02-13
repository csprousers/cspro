#pragma once

class DictValuePair;


namespace CSPro
{
    namespace Dictionary
    {
        ///<summary>
        ///Pair of from/to values in a value set.
        ///</summary>
        public ref class ValuePair sealed
        {
        public:
            ValuePair(const DictValuePair& dict_value_pair);

            property System::String^ From { System::String^ get() { return m_from; } }

            property System::String^ To { System::String^ get() { return m_to; } }

        private:
            System::String^ m_from;
            System::String^ m_to;
        };
    }
}
