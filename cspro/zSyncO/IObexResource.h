#pragma once

#include <zSyncO/ObexConstants.h>
#include <zNetwork/HeaderList.h>


// A resource that can read/written on an Obex server.
// A resource could be file or another object on the server.
// It is accessed by name/content-type from an ObexHandler.

struct IObexResource
{
    virtual ~IObexResource() { }

    // Begin writing to resource
    // Must be called before any calls to write.
    virtual ObexResponseCode openForWriting() = 0;

    // Begin reading from resource
    // Must be called before any calls to getIStream or getTotalSize.
    virtual ObexResponseCode openForReading() = 0;

    // End reading/writing from resource
    // Must be called before any calls to getIStream or getTotalSize.
    virtual ObexResponseCode close() = 0;

    // Get total size in bytes of the resource. Returns -1 if size is unknown.
    virtual int64_t getTotalSize() = 0;

    // Get istream to read resource data from
    virtual std::istream* getIStream() = 0;

    // Get ostream to write resource data to
    virtual std::ostream* getOStream() = 0;

    // Retrieve optional HTTP style headers that should be sent with resource
    virtual const HeaderList& getHeaders() const = 0;
};
