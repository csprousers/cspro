#include "stdafx.h"
#include "Versioning.h"


namespace
{
    std::optional<int> RuntimeVersion;
}


int Versioning::GetCompiledLogicVersion()
{
    return RuntimeVersion.value_or(Serializer::GetCurrentVersion());
}

void Versioning::SetCompiledLogicVersion(int version)
{
    RuntimeVersion = version;
}
