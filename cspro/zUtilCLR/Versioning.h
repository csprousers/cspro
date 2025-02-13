#pragma once

namespace CSPro
{
    namespace Util
    {
        ///<summary>
        /// Expose CSPro version information to .NET
        ///</summary>
        public ref class Versioning
        {
        public:
            ///<summary>
            ///Get general version number e.g. 7.0
            ///</summary>
            static property double Number { double get(); }

            ///<summary>
            ///Get precise version number e.g. "7.0.0"
            ///</summary>
            static property System::String^ DetailedString { System::String^ get(); }

            ///<summary>
            ///Get release date
            ///</summary>
            static property System::String^ ReleaseDateString { System::String^ get(); }
        };
    }
}
