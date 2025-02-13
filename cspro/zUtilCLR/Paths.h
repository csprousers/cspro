#pragma once

namespace CSPro
{
    namespace Util
    {
        public ref class Paths
        {
        public:
            static System::String^ MakeRelativeFilename(System::String^ relativePath,System::String^ filename);
            static System::String^ MakeAbsoluteFilename(System::String^ relativePath,System::String^ filename);
        };
    }
}
