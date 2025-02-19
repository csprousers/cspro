#pragma once

#include <zUtilO/zUtilO.h>


// a portable randomizer to ensure that random numbers generated on Windows/Android are the same

class CLASS_DECL_ZUTILO Randomizer
{
public:
    static void Seed(uint32_t seed);

    static double Next();

    // returns a seed value that can be used for calls to things like std::default_random_engine
    static uint32_t NextSeed();


    struct State;

    class StateSaver
    {
    public:
        StateSaver(bool reset_to_default_state);
        ~StateSaver();

    private:
        std::unique_ptr<State> m_state;
    };
};
