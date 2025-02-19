#include "StdAfx.h"
#include "Randomizer.h"


namespace
{
    // code modified from http://www.mit.edu/afs/athena/activity/c/cgs/src/math/drand48/drand48.c
    const uint32_t N = 16;
    const uint32_t MASK = ( 1 << ( N - 1 ) ) + ( 1 << ( N - 1 ) ) - 1;
    const uint32_t X0 = 0x330E;
    const uint32_t X1 = 0xABCD;
    const uint32_t X2 = 0x1234;
    const uint32_t A0 = 0xE66D;
    const uint32_t A1 = 0xDEEC;
    const uint32_t A2 = 0x5;
    const uint32_t C = 0xB;
    const double two16m = 1.0 / ( (uint64_t)1 << N );
}


struct Randomizer::State
{
    uint32_t x[3] = { X0, X1, X2 };
    uint32_t a[3] = { A0, A1, A2 };
    uint32_t c = C;
};


namespace
{
    Randomizer::State CurrentState;

    template<typename T1>
    inline uint32_t LOW(const T1 x)
    {
        return static_cast<uint32_t>(x & MASK);
    }

    inline uint32_t HIGH(const uint32_t x)
    {
        return LOW(x >> N);
    }

    inline void MUL(const uint32_t x, const uint32_t y, uint32_t z[])
    {
        const uint32_t l = static_cast<uint32_t>(static_cast<int64_t>(x) * static_cast<int64_t>(y));
        z[0] = LOW(l);
        z[1] = HIGH(l);
    }

    template<typename T1, typename T2>
    inline bool CARRY(const T1 x, const T2 y)
    {
        return ( ( static_cast<int64_t>(x) + static_cast<int64_t>(y) ) > MASK );
    }

    inline void ADDEQU(uint32_t& x, const int64_t y, uint32_t& z)
    {
        z = CARRY(x, y);
        x = LOW(x + y);
    }

    inline void SET3(uint32_t x[], const uint32_t x0, const uint32_t x1, const uint32_t x2)
    {
        x[0] = x0;
        x[1] = x1;
        x[2] = x2;
    }

    inline void SEED(const uint32_t x0, const uint32_t x1, const uint32_t x2)
    {
        SET3(CurrentState.x, x0, x1, x2);
        SET3(CurrentState.a, A0, A1, A2);
        CurrentState.c = C;
    }

    void DoNext()
    {
        uint32_t p[2];
        uint32_t q[2];
        uint32_t r[2];
        uint32_t carry0;
        uint32_t carry1;
        auto& x = CurrentState.x;
        auto& a = CurrentState.a;
        auto& c = CurrentState.c;

        MUL(a[0], x[0], p);
        ADDEQU(p[0], c, carry0);
        ADDEQU(p[1], carry0, carry1);
        MUL(a[0], x[1], q);
        ADDEQU(p[1], q[0], carry0);
        MUL(a[1], x[0], r);
        x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] + a[0] * x[2] + a[1] * x[1] + a[2] * x[0]);
        x[1] = LOW(p[1] + r[0]);
        x[0] = LOW(p[0]);
    }
}


void Randomizer::Seed(const uint32_t seed)
{
    SEED(X0, LOW(seed), HIGH(seed));
}


double Randomizer::Next()
{
    DoNext();
    return ( two16m * ( two16m * ( two16m * CurrentState.x[0] + CurrentState.x[1] ) + CurrentState.x[2] ) );
}


uint32_t Randomizer::NextSeed()
{
    DoNext();
    return CurrentState.x[2];
}


Randomizer::StateSaver::StateSaver(const bool reset_to_default_state)
    :   m_state(std::make_unique<State>(CurrentState))
{
    if( reset_to_default_state )
        CurrentState = State();
}


Randomizer::StateSaver::~StateSaver()
{
    CurrentState = *m_state;
}
