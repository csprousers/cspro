#include "StdAfx.h"

#ifndef WIN32
#include <external/stduuid/uuid.h>
#endif


std::string CreateUuid()
{
#ifdef WIN32
    UUID uuid;
    CoCreateGuid(&uuid);

    constexpr size_t UuidLength = 36;
    constexpr size_t UuidBufferLength = UuidLength + 2 + 1; // 36 for the GUID, 2 for { } , 1 for the null terminator

    wchar_t uuid_text[UuidBufferLength];
    const int uuid_text_length = StringFromGUID2(uuid, uuid_text, UuidBufferLength) - 1;
    ASSERT(uuid_text_length == ( UuidBufferLength - 1 ));

    // StringFromGUID2 adds {} around the UUID that we don't want so skip past the {
    ASSERT(uuid_text[0] == '{');
    std::string uuid_string = TC::AsciiToUtf8(uuid_text + 1, UuidLength);

    // make the UUID lowercase to make it consistent with Android
    SO::MakeLower(uuid_string);

    return uuid_string;

#else
    // based off: https://github.com/mariusbancila/stduuid

    struct UuidData
    {
        std::unique_ptr<std::seed_seq> seq;
        std::unique_ptr<std::mt19937> generator;
        std::unique_ptr<uuids::uuid_random_generator> uuid_random_generator;
    };

    static const UuidData uuid_data =
        []() -> UuidData
        {
            std::random_device rd;

            std::array<int, std::mt19937::state_size> seed_data { };
            std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));

            UuidData this_uuid_data;

            this_uuid_data.seq = std::make_unique<std::seed_seq>(seed_data.cbegin(), seed_data.cend());
            this_uuid_data.generator = std::make_unique<std::mt19937>(*this_uuid_data.seq);
            this_uuid_data.uuid_random_generator = std::make_unique<uuids::uuid_random_generator>(*this_uuid_data.generator);

            return this_uuid_data;
        }();

    const uuids::uuid id = (*uuid_data.uuid_random_generator)();

    ASSERT(!id.is_nil());
    ASSERT(id.as_bytes().size() == 16);
    ASSERT(id.version() == uuids::uuid_version::random_number_based);
    ASSERT(id.variant() == uuids::uuid_variant::rfc);

    return uuids::to_string<char>(id);

#endif
}
