#pragma once

class CDataDict;


class EncryptedSQLiteRepositoryPasswordManager
{
public:
    using GetEmbeddedDictionaryCallback = std::function<std::unique_ptr<CDataDict>()>;
    using SuccessfulOpenCallback = std::function<void(const std::byte*, const GetEmbeddedDictionaryCallback&)>;
    using OpenByPasswordCallback = std::function<bool(const std::string&, const SuccessfulOpenCallback*)>;
    using OpenByPasswordHashCallback = std::function<bool(const std::byte*)>;

    EncryptedSQLiteRepositoryPasswordManager(const CDataDict* dictionary, std::string file_path,
                                             const OpenByPasswordCallback& file_open_by_password_callback,
                                             const OpenByPasswordHashCallback& file_open_by_password_hash_callback);

    void GetPassword();

private:
    bool GetPasswordHashFromCredentialManagerOrPreviousUseInCurrentSession() const;
    void UpdatePasswordHashInCredentialManager(const CDataDict* dictionary, const std::byte* password_hash);

    bool QueryForPassword();
    void TryOpeningWithPassword(const std::string& password);

    std::string GetPasswordQueryTitle() const;
    std::string GetPasswordQueryDescription() const;

#ifdef WIN_DESKTOP
    static INT_PTR CALLBACK PasswordDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI LaunchPasswordDialog(LPVOID pParam);
    static EncryptedSQLiteRepositoryPasswordManager* m_instance;
#endif

    const CDataDict* const m_dictionary;
    const std::string m_filePath;
    const OpenByPasswordCallback& m_fileOpenByPasswordCallback;
    const OpenByPasswordHashCallback& m_fileOpenByPasswordHashCallback;

    static std::map<std::string, std::string> m_previouslyUsedPasswordHashes;
};
