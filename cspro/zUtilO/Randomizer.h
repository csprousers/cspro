#pragma once

#include <zUtilO/zUtilO.h>

// a portable randomizer to ensure that random numbers generated on Windows/Android are the same

namespace Randomizer
{
    CLASS_DECL_ZUTILO void Seed(uint32_t seed);

    CLASS_DECL_ZUTILO double Next();

    // returns a seed value that can be used for calls to things like std::default_random_engine
    CLASS_DECL_ZUTILO uint32_t NextSeed();


    struct State;

    class CLASS_DECL_ZUTILO StateSaver
    {
    public:
        StateSaver(bool reset_to_default_state);
        ~StateSaver();

    private:
        std::unique_ptr<State> m_state;
    };
};
