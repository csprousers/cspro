#pragma once

///<summary>
/// Compute sequence of delays for an exponential backoff retry policy.
///</summary>
class ExponentialBackOff
{
public:

    ExponentialBackOff(int initialDelayMillis = 500, float multiplier = 1.5f, float randomizationFactor = 0.5f);

    int NextBackOffMillis();

private:

    int m_delay;
    const float m_multiplier;
    const float m_randomizationFactor;
};