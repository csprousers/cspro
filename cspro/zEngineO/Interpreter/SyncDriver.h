#pragma once

class ApplicationPackageManager;
class LoginAccessor;


class SyncDriver
{
public:
    SyncDriver(LogicInterpreter& interpreter);
    ~SyncDriver();

    LoginAccessor& GetLoginAccessor()                   { return *m_loginAccessor; }
    SyncClient& GetSyncClient()                         { return *m_syncClient; }
    std::shared_ptr<SyncListener> GetSharedSyncClient() { return m_syncListener; }

    std::optional<SyncDirection> EvaluateSyncDirection(int expression) const;

    static std::unique_ptr<ApplicationPackageManager> CreateApplicationPackageManager();

private:
    LogicInterpreter& m_interpreter;
    std::shared_ptr<LoginAccessor> m_loginAccessor;
    std::unique_ptr<SyncClient> m_syncClient;
    std::shared_ptr<SyncListener> m_syncListener;
};
