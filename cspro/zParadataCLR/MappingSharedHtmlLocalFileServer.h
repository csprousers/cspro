#pragma once

class SharedHtmlLocalFileServer;

namespace CSPro
{
    namespace ParadataViewer
    {
        public ref class MappingSharedHtmlLocalFileServer sealed
        {
        public:
            MappingSharedHtmlLocalFileServer();

            ~MappingSharedHtmlLocalFileServer() { this->!MappingSharedHtmlLocalFileServer(); }
            !MappingSharedHtmlLocalFileServer();

            System::String^ CreateProjectUrl(System::String^ url_from_project_root);

        private:
            SharedHtmlLocalFileServer* m_fileServer;
        };
    }
}
