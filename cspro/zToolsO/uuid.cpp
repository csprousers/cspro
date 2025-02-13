#include "StdAfx.h"

#ifndef WIN32
#include <external/stduuid/uuid.h>
#endif


std::wstring CreateUuid()
{
#ifdef WIN32
    UUID uuid;
    CoCreateGuid(&uuid);

    constexpr size_t UuidBufferLength = 36 + 2 + 1; // 36 for the GUID, 2 for { } , 1 for the null terminator
    TCHAR uuid_text[UuidBufferLength];
    int uuid_text_length = StringFromGUID2(uuid, uuid_text, UuidBufferLength) - 1;
    ASSERT(uuid_text_length == ( UuidBufferLength - 1 ));

    wstring_view uuid_sv = wstring_view(uuid_text, uuid_text_length);

    // StringFromGUID2 adds {} around uuid that we don't want and it uses uppercase
    // which is not consistent with Android
    return SO::ToLower(uuid_sv.substr(1, uuid_sv.length() - 2));

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

            UuidData uuid_data;

            uuid_data.seq = std::make_unique<std::seed_seq>(seed_data.cbegin(), seed_data.cend());
            uuid_data.generator = std::make_unique<std::mt19937>(*uuid_data.seq);
            uuid_data.uuid_random_generator = std::make_unique<uuids::uuid_random_generator>(*uuid_data.generator);

            return uuid_data;
        }();

    const uuids::uuid id = (*uuid_data.uuid_random_generator)();

    ASSERT(!id.is_nil());
    ASSERT(id.as_bytes().size() == 16);
    ASSERT(id.version() == uuids::uuid_version::random_number_based);
    ASSERT(id.variant() == uuids::uuid_variant::rfc);

    return uuids::to_string<wchar_t>(id);

#endif
}
