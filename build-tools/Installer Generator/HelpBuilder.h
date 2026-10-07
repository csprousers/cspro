#pragma once


namespace CSPro::InstallerGenerator
{
    class HelpBuilderWorker;

    public delegate void OnLogText(System::String^ text);

    public ref class HelpBuilder sealed
    {
    public:
        HelpBuilder(System::String^ helps_directory, OnLogText^ on_log_text_callback);

        !HelpBuilder();
        ~HelpBuilder() { this->!HelpBuilder(); }

        void Create(System::String^ program);

    private:
        HelpBuilderWorker* m_worker;
    };
}
