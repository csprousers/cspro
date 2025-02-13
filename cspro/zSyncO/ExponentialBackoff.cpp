#include "stdafx.h"
#include "ExponentialBackoff.h"

ExponentialBackOff::ExponentialBackOff(int initialDelayMillis, float multiplier, float randomizationFactor)
    : m_delay(initialDelayMillis),
    m_multiplier(multiplier),
    m_randomizationFactor(randomizationFactor)
{
}

int ExponentialBackOff::NextBackOffMillis()
{
    // randomized_interval =
    //  retry_interval * (random value in range[1 - randomization_factor, 1 + randomization_factor])
    const float random = (2 * m_randomizationFactor * ((float) rand() / RAND_MAX) + 1 - m_randomizationFactor);
    int result = int(m_delay * random);
    m_delay *= 2;
    return result;
}

