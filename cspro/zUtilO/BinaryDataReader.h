#pragma once

#include <zUtilO/BinaryData.h>


// any object (though generally a data repository) can create a subclass of the BinaryDataReader
// which will be called by the BinaryCaseItem class when reading data;

// data repositories can assume that the BinaryDataReader will only be used while the data
// repository is still open;

// all of the Get... methods can throw exceptions

class BinaryDataReader
{
public:
    virtual ~BinaryDataReader() { }

    // returns a BinaryData object (with the contents and metadata)
    virtual BinaryData GetBinaryData() = 0;

    // returns the metadata (without having to load the contents)
    virtual const BinaryDataMetadata& GetMetadata() = 0;

    // returns the size of the data (without having to load the contents)
    virtual uint64_t GetSize() = 0;

    // this optional method can be used by a data repository to, for example,
    // determine whether or not something has changed in a read/write repository;
    // if the data is removed (e.g., by removing a record occurrence), this method
    // will not be called, but it will be called if the data changes or is cleared
    virtual void OnBinaryDataChange() { }
};
