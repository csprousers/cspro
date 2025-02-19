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

    int NextBackOffMillis()
    {
        // randomized_interval =
        //  retry_interval * (random value in range[1 - randomization_factor, 1 + randomization_factor])
        const float random = 2 * m_randomizationFactor * ( static_cast<float>(rand()) / static_cast<float>(RAND_MAX) ) + 1 - m_randomizationFactor;
        int result = int(m_delay * random);
        m_delay *= 2;
        return result;
    }

private:
    int m_delay;
    const float m_multiplier;
    const float m_randomizationFactor;
};
