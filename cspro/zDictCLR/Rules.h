#pragma once

namespace CSPro
{
    namespace Dictionary
    {
        public ref class Rules
        {
        private:
            Rules() {}

        public:
            static int MaxLengthNumeric = MAX_NUMERIC_ITEM_LEN;
            static int MaxLengthDecimal = MAX_DECIMALS;
            static int MaxLengthAlpha = MAX_ALPHA_ITEM_LEN;
        };
    }
}
