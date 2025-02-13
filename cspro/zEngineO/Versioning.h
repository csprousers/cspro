#pragma once

#include <zEngineO/zEngineO.h>
#include <zToolsO/Serializer.h>


namespace Versioning
{
    ZENGINEO_API int GetCompiledLogicVersion();

    ZENGINEO_API void SetCompiledLogicVersion(int version);

    inline bool MeetsCompiledLogicVersion(int version)
    {
        ASSERT(version > Serializer::GetEarliestSupportedVersion());
        return GetCompiledLogicVersion() >= version;
    }

    inline bool PredatesCompiledLogicVersion(int version)
    {
        ASSERT(version > Serializer::GetEarliestSupportedVersion());
        return !MeetsCompiledLogicVersion(version);
    }
}
