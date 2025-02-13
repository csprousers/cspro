#include "stdafx.h"
#include "VectorClock.h"

int VectorClock::getVersion(const CString& device) const
{
    const DeviceRevMap::const_iterator i = m_vector.find(device);
    return (i == m_vector.end()) ? 0 : i->second;
}

// Compare clocks - clock A is = B iff all versions are == corresponding
// version in B
bool VectorClock::operator==(const VectorClock& rhs) const
{
    for (DeviceRevMap::const_iterator i = m_vector.begin(); i != m_vector.end(); ++i) {
        const CString& dev = i->first;
        const int myVersion = i->second;
        const int rhsVersion = rhs.getVersion(dev);
        if (myVersion != rhsVersion)
            return false;
    }
    for (DeviceRevMap::const_iterator i = rhs.m_vector.begin(); i != rhs.m_vector.end(); ++i) {
        const CString& dev = i->first;
        const int rhsVersion = i->second;
        const int myVersion = getVersion(dev);
        if (myVersion != rhsVersion)
            return false;
    }
    return true;
}

bool VectorClock::operator!=(const VectorClock& rhs) const
{
    return !operator==(rhs);
}

bool VectorClock::operator<(const VectorClock& rhs) const
{
    // Vector clock A is strictly less than B if all versions
    // in A are less than or equal to those in B and at least
    // one is strictly less.

    bool foundStrict = false;
    for (DeviceRevMap::const_iterator i = m_vector.begin(); i != m_vector.end(); ++i) {
        const CString& dev = i->first;
        const int myVersion = i->second;
        const int rhsVersion = rhs.getVersion(dev);
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
    for (DeviceRevMap::const_iterator i = rhs.m_vector.begin(); i != rhs.m_vector.end(); ++i) {
        if (getVersion(i->first) == 0)
            foundStrict = true;
    }

    return foundStrict;
}

void VectorClock::merge(const VectorClock& rhs)
{
    for (DeviceRevMap::const_iterator i = rhs.m_vector.begin(); i != rhs.m_vector.end(); ++i) {
        const CString& dev = i->first;
        DeviceRevMap::iterator j = m_vector.find(dev);
        if (j == m_vector.end()) {
            m_vector.insert(DeviceRevMap::value_type(dev, i->second));
        }
        else {
            j->second = std::max(j->second, i->second);
        }
    }
}

void VectorClock::increment(const CString& device)
{
    DeviceRevMap::iterator i = m_vector.find(device);
    if (i == m_vector.end()) {
        m_vector.insert(DeviceRevMap::value_type(device, 1));
    }
    else {
        ++(i->second);
    }
}

std::vector<CString> VectorClock::getAllDevices() const
{
    std::vector<CString> devices;
    for (DeviceRevMap::const_iterator i = m_vector.begin(); i != m_vector.end(); ++i) {
        devices.push_back(i->first);
    }
    return devices;
}

void VectorClock::setVersion(const CString& device, int version)
{
    m_vector[device] = version;
}

void VectorClock::clear()
{
    m_vector.clear();
}
