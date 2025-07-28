#include "stdafx.h"
#include "VectorClock.h"
#include <zJson/Json.h>


int VectorClock::getVersion(const DeviceId& device_id) const
{
    const auto& lookup = m_vector.find(device_id);

    return ( lookup != m_vector.cend() ) ? lookup->second :
                                           0;
}


// Compare clocks - clock A is = B iff all versions are == corresponding version in B
bool VectorClock::operator==(const VectorClock& rhs) const
{
    for( const auto& [device_id, myVersion] : m_vector ) {
        if( myVersion != rhs.getVersion(device_id) )
            return false;
    }

    for( const auto& [device_id, rhsVersion] : rhs.m_vector ) {
        if( rhsVersion != getVersion(device_id) )
            return false;
    }

    return true;
}


bool VectorClock::operator<(const VectorClock& rhs) const
{
    // Vector clock A is strictly less than B if all versions
    // in A are less than or equal to those in B and at least
    // one is strictly less.
    bool foundStrict = false;

    for( const auto& [device_id, myVersion] : m_vector ) {
        const int rhsVersion = rhs.getVersion(device_id);
        if (rhsVersion < myVersion)
            return false;
        if (rhsVersion > myVersion)
            foundStrict = true;
    }

    // If we haven't found version that is strictly less then we need to look
    // for devs in rhs that are not in this. For example:
    //   {a:2, b:1} < {a:2, b:1, c:1}
    // where looking at devs in this only we would assume vectors are equal but
    // looking at c in rhs we can conclude that this < rhs
    for( const auto& [device_id, rhsVersion] : rhs.m_vector ) {
        if (getVersion(device_id) == 0) {
            foundStrict = true;
            break;
        }
    }

    return foundStrict;
}


void VectorClock::merge(const VectorClock& rhs)
{
    for( const auto& [device_id, rhsVersion] : rhs.m_vector ) {
        auto lookup = m_vector.find(device_id);
        if( lookup == m_vector.cend() ) {
            m_vector.try_emplace(device_id, rhsVersion);
        }
        else {
            lookup->second = std::max(rhsVersion, lookup->second);
        }
    }
}


void VectorClock::increment(const DeviceId& device_id)
{
    int& myVersion = m_vector[device_id];
    ++myVersion;
}


std::vector<DeviceId> VectorClock::getAllDevices() const
{
    std::vector<DeviceId> devices;

    for( const auto& [device_id, revision] : m_vector )
        devices.emplace_back(device_id);

    return devices;
}


void VectorClock::setVersion(const DeviceId& device_id, const int version)
{
    m_vector[device_id] = version;
}


void VectorClock::clear()
{
    m_vector.clear();
}


VectorClock VectorClock::CreateFromJson(const JsonNode& json_node)
{
    VectorClock vector_clock;

    for( const JsonNode& clock_node : json_node.GetArray() )
        vector_clock.m_vector.try_emplace(clock_node.Get<std::string>(JK::deviceId), clock_node.Get<int>(JK::revision));

    return vector_clock;
}


void VectorClock::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginArray();

    for( const auto& [device_id, revision] : m_vector )
    {
        json_writer.BeginObject()
                   .Write(JK::deviceId, device_id)
                   .Write(JK::revision, revision)
                   .EndObject();
    }

    json_writer.EndArray();
}
