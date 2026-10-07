#pragma once


// Compute sequence of delays for an exponential backoff retry policy.

class ExponentialBackOff
{
public:
    ExponentialBackOff(int initialDelayMillis = 500, float multiplier = 1.5f, float randomizationFactor = 0.5f)
        :   m_delay(initialDelayMillis),
            m_multiplier(multiplier),
            m_randomizationFactor(randomizationFactor)
    {
    }

    void Sleep()
    {
        // randomized_interval =
        //  retry_interval * (random value in range[1 - randomization_factor, 1 + randomization_factor])
        const float random = 2 * m_randomizationFactor * ( static_cast<float>(rand()) / static_cast<float>(RAND_MAX) ) + 1 - m_randomizationFactor;
        const int result = static_cast<int>(m_delay * random);
        m_delay *= 2;

#ifdef WIN32
        ::Sleep(result);
#else
        usleep(result * 1000);
#endif
    }

private:
    int m_delay;
    const float m_multiplier;
    const float m_randomizationFactor;
};
