#pragma once

#include <zCaseO/zCaseO.h>
#include <zAppO/SyncTypes.h>


/* Timestamp for use in a distributed system.
* Instead of holding a single global timestamp, the vector
* contains a version number for each device in the system.
* When a device modifies a value, it increments its own version
* number in the vector. Vector clocks can be compared by comparing
* the versions of for each of the devices and may be merged
* by taking the max versions of devices in two vectors.
*/
class ZCASEO_API VectorClock
{
public:
    // Get version for specific device
    int getVersion(const DeviceId& device_id) const;

    // Compare clocks - clock A is = B iff all versions are == corresponding
    // version in B
    bool operator==(const VectorClock& rhs) const;
    bool operator!=(const VectorClock& rhs) const { return !operator==(rhs); }

    // Compare clocks - clock A is < B iff all versions are <= corresponding
    // version in B and at least one is strictly <
    bool operator<(const VectorClock& rhs) const;

    // Combine two vector clocks by taking max of versions for each device
    void merge(const VectorClock& rhs);

    // Increment version number for specific device
    void increment(const DeviceId& device_id);

    // Get all devices in the clock
    std::vector<DeviceId> getAllDevices() const;

    // Set specific revision
    void setVersion(const DeviceId& device_id, int version);

    void clear();

    // serialization
    static VectorClock CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

private:
    using DeviceRevMap = std::map<DeviceId, int>;
    DeviceRevMap m_vector;
};
