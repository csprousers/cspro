#pragma once

#include <zSyncO/ObexConstants.h>
#include <zNetwork/HeaderList.h>
#include <stdint.h>


/// <summary>A resource that can read/written on an Obex server.</summary>
/// A resource could be file or another object on the server.
/// It is accessed by name/content-type from an ObexHandler.
struct IObexResource {

    virtual ~IObexResource() { }

    /// <summary>Begin writing to resource</summary>
    /// Must be called before any calls to write.
    virtual ObexResponseCode openForWriting() = 0;

    /// <summary>Begin reading from resource</summary>
    /// Must be called before any calls to getIStream or getTotalSize.
    virtual ObexResponseCode openForReading() = 0;

    /// <summary>End reading/writing from resource</summary>
    /// Must be called before any calls to getIStream or getTotalSize.
    virtual ObexResponseCode close() = 0;

    /// <summary>Get total size in bytes of the resource. Returns -1 if size is unknown.</summary>
    virtual int64_t getTotalSize() = 0;

    /// <summary>Get istream to read resource data from</summary>
    virtual std::istream* getIStream() = 0;

    /// <summary>Get ostream to write resource data to</summary>
    virtual std::ostream* getOStream() = 0;

    /// <summary>Retrieve optional HTTP style headers that should be sent with resource</summary>
    virtual const HeaderList& getHeaders() const = 0;
};
