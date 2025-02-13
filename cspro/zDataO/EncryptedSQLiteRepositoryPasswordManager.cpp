#include "stdafx.h"
#include "EncryptedSQLiteRepositoryPasswordManager.h"
#include "EncryptedSQLiteRepository.h"
#include "zDataO.h"
#include <zToolsO/Hash.h>
#include <zUtilO/CredentialStore.h>


std::map<std::wstring, std::wstring> EncryptedSQLiteRepositoryPasswordManager::m_previouslyUsedPasswordHashes;


EncryptedSQLiteRepositoryPasswordManager::EncryptedSQLiteRepositoryPasswordManager(const CDataDict* dictionary, std::wstring filename,
                                                                                   const OpenByPasswordCallback& file_open_by_password_callback,
                                                                                   const OpenByPasswordHashCallback& file_open_by_password_hash_callback)
    :   m_dictionary(dictionary),
        m_filename(std::move(filename)),
        m_fileOpenByPasswordCallback(file_open_by_password_callback),
        m_fileOpenByPasswordHashCallback(file_open_by_password_hash_callback)
{
}


void EncryptedSQLiteRepositoryPasswordManager::GetPassword()
{
    if( !GetPasswordHashFromCredentialManagerOrPreviousUseInCurrentSession() && !QueryForPassword() )
    {
        const std::wstring& formatter = MGF::GetMessageText(94303, _T("No valid password specified for the file %s"));
        throw DataRepositoryException::EncryptionError(FormatText(formatter.c_str(), PortableFunctions::PathGetFilename(m_filename)));
    }
}


namespace
{
    class EncryptedSQLiteRepositoryCredential
    {
    private:
        struct Header
        {
            char _version;
            double _storage_timestamp;
        };

        std::vector<std::byte> _bytes; // this will store the version number, the expiration timestamp, and the password hash
        static const char CurrentVersion = 1;

        EncryptedSQLiteRepositoryCredential()
            :   _bytes(sizeof(Header) + EncryptedSQLiteRepository::PasswordHashSize)
        {
        }

        char GetVersion() const
        {
            return reinterpret_cast<const Header*>(_bytes.data())->_version;
        }

    public:
        double GetStorageTimestamp() const
        {
            return reinterpret_cast<const Header*>(_bytes.data())->_storage_timestamp;
        }

        const std::byte* GetPasswordHash() const
        {
            return _bytes.data() + sizeof(Header);
        }

        EncryptedSQLiteRepositoryCredential(const std::byte* password_hash)
            :   EncryptedSQLiteRepositoryCredential()
        {
            reinterpret_cast<Header*>(_bytes.data())->_version = CurrentVersion;
            reinterpret_cast<Header*>(_bytes.data())->_storage_timestamp = GetTimestamp();
            memcpy(_bytes.data() + sizeof(Header), password_hash, EncryptedSQLiteRepository::PasswordHashSize);
        }

        std::wstring ToString() const
        {
            return Hash::BytesToHexString(_bytes.data(), _bytes.size());
        }

        static std::unique_ptr<EncryptedSQLiteRepositoryCredential> FromString(const wstring_view credential_string_sv)
        {
            std::unique_ptr<EncryptedSQLiteRepositoryCredential> credential(new EncryptedSQLiteRepositoryCredential);

            // the credential will be considered valid if the length is correct and if the version matches
            if( credential_string_sv.length() == ( credential->_bytes.size() * 2 ) )
            {
                Hash::HexStringToBytesBuffer(credential_string_sv, credential->_bytes.data(), false);

                if( credential->GetVersion() == CurrentVersion )
                    return credential;
            }

            return nullptr;
        }
    };


    class EncryptedSQLiteRepositoryCredentialStore : public CredentialStore
    {
    protected:
        std::wstring PrefixAttribute(const std::wstring& attribute) override
        {
            // instead of storing the full filename in the credentials, store a hash of it
            return _T("CSPro_data_") + Hash::Hash(attribute, 16);
        }
    };
}


bool EncryptedSQLiteRepositoryPasswordManager::GetPasswordHashFromCredentialManagerOrPreviousUseInCurrentSession() const
{
    if( !PortableFunctions::FileIsRegular(m_filename) )
        return false;

    // on the first pass, see if the password hash was previously used to open the file (in the current session)
    const auto& previously_used_password_hash_lookup = m_previouslyUsedPasswordHashes.find(m_filename);

    if( previously_used_password_hash_lookup != m_previouslyUsedPasswordHashes.cend() )
    {
        std::unique_ptr<const EncryptedSQLiteRepositoryCredential> credential = EncryptedSQLiteRepositoryCredential::FromString(previously_used_password_hash_lookup->second);

        if( m_fileOpenByPasswordHashCallback(credential->GetPasswordHash()) )
            return true;
    }

    // on the second pass, see if the password hash is in the credential manager
    if( m_dictionary != nullptr && m_dictionary->GetCachedPasswordMinutes() <= 0 )
        return false;

    const std::wstring credential_string = EncryptedSQLiteRepositoryCredentialStore().Retrieve(m_filename);

    if( !credential_string.empty() )
    {
        std::unique_ptr<const EncryptedSQLiteRepositoryCredential> credential = EncryptedSQLiteRepositoryCredential::FromString(credential_string);

        if( credential != nullptr )
        {
            if( m_dictionary == nullptr || ( GetTimestamp() < ( credential->GetStorageTimestamp() + 60.0 * m_dictionary->GetCachedPasswordMinutes() ) ) )
            {
                if( m_fileOpenByPasswordHashCallback(credential->GetPasswordHash()) )
                    return true;
            }
        }
    }

    return false;
}


void EncryptedSQLiteRepositoryPasswordManager::UpdatePasswordHashInCredentialManager(const CDataDict* dictionary, const std::byte* password_hash)
{
    EncryptedSQLiteRepositoryCredential credential(password_hash);
    const std::wstring credential_string = credential.ToString();

    m_previouslyUsedPasswordHashes[m_filename] = credential_string;

    if( dictionary == nullptr || dictionary->GetCachedPasswordMinutes() <= 0 )
        return;

    EncryptedSQLiteRepositoryCredentialStore().Store(m_filename, credential_string);
}


std::wstring EncryptedSQLiteRepositoryPasswordManager::GetPasswordQueryTitle() const
{
    ASSERT(m_dictionary != nullptr);
    const std::wstring& formatter = MGF::GetMessageText(94304, _T("Password for: %s"));
    return FormatTextCS2WS(formatter.c_str(), m_dictionary->GetLabel().GetString());
}


std::wstring EncryptedSQLiteRepositoryPasswordManager::GetPasswordQueryDescription() const
{
    const std::wstring& formatter = PortableFunctions::FileIsRegular(m_filename) ?
        MGF::GetMessageText(94305, _T("Enter the password previously used to encrypt the file %s:")) :
        MGF::GetMessageText(94306, _T("Enter a new password for the file %s. The password must be at %d characters and it should be something that you will remember as there is no way to decrypt this file without the password:"));

    return FormatTextCS2WS(formatter.c_str(), PortableFunctions::PathGetFilename(m_filename), static_cast<int>(EncryptedSQLiteRepository::PasswordMinimumLength));
}


void EncryptedSQLiteRepositoryPasswordManager::TryOpeningWithPassword(const std::wstring& password)
{
    const SuccessfulOpenCallback successful_open_callback =
        [&](const std::byte* password_hash, const std::function<std::unique_ptr<CDataDict>()>& get_embedded_dictionary_callback)
        {
            std::unique_ptr<CDataDict> embedded_dictionary;
            const CDataDict* dictionary = m_dictionary;

            // if opening the file without the dictionary specified, get the embedded dictionary
            if( dictionary == nullptr )
            {
                embedded_dictionary = get_embedded_dictionary_callback();
                dictionary = embedded_dictionary.get();
            }

            UpdatePasswordHashInCredentialManager(dictionary, password_hash);
        };

    if( !m_fileOpenByPasswordCallback(password, &successful_open_callback) )
        throw DataRepositoryException::EncryptionError(MGF::GetMessageText(94307, _T("The password is invalid")));
}


#ifdef WIN_DESKTOP

#include <zUtilO/WindowHelpers.h>
#include <zUtilO/WindowsWS.h>


INT_PTR CALLBACK EncryptedSQLiteRepositoryPasswordManager::PasswordDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    CWnd* dialog = CWnd::FromHandle(hwndDlg);

    if( uMsg == WM_INITDIALOG )
    {
        auto move_controls_and_resize_dialog = [&](const int height_change, const bool move_password_fields)
        {
            static const int control_ids[] =
            {
                IDC_TEXT_PASSWORD, IDC_EDIT_PASSWORD,
                IDC_TEXT_PASSWORD_REENTER, IDC_EDIT_PASSWORD_REENTER,
                IDOK, IDCANCEL
            };

            CRect rect;

            // move each control
            for( size_t i = ( move_password_fields ? 0 : 2 ); i < _countof(control_ids); ++i )
            {
                CWnd* control = dialog->GetDlgItem(control_ids[i]);
                control->GetWindowRect(&rect);
                OffsetRect(&rect, 0, height_change);
                dialog->ScreenToClient(&rect);
                control->SetWindowPos(nullptr, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }

            // resize the dialog
            dialog->GetWindowRect(&rect);
            dialog->SetWindowPos(nullptr, 0, 0, rect.Width(), rect.Height() + height_change, SWP_NOMOVE);
        };

        // set the title
        if( m_instance->m_dictionary != nullptr )
            WindowsWS::SetWindowText(dialog, m_instance->GetPasswordQueryTitle());

        // fill the description
        const std::wstring password_description = m_instance->GetPasswordQueryDescription();
        CWnd* password_description_text = dialog->GetDlgItem(IDC_TEXT_PASSWORD_DESCRIPTION);
        WindowsWS::SetWindowText(password_description_text, password_description);

        // get the height needed to show the description
        CRect rect;
        password_description_text->GetWindowRect(&rect);
        int original_description_height = rect.Height();

        CClientDC dc(password_description_text);
        CFont* font = password_description_text->GetFont();
        dc.SelectObject(font);
        dc.DrawText(password_description.c_str(), &rect, DT_CALCRECT | DT_WORDBREAK);

        // move the controls and resize the description
        move_controls_and_resize_dialog(rect.Height() - original_description_height, true);
        password_description_text->SetWindowPos(nullptr, 0, 0, rect.Width(), rect.Height(), SWP_NOMOVE);

        // if the file exists, hide the reenter password controls
        if( PortableFunctions::FileIsRegular(m_instance->m_filename) )
        {
            dialog->GetDlgItem(IDC_TEXT_PASSWORD_REENTER)->ShowWindow(SW_HIDE);

            CWnd* reentered_password_edit = dialog->GetDlgItem(IDC_EDIT_PASSWORD_REENTER);
            reentered_password_edit->ShowWindow(SW_HIDE);

            CRect control_rect;
            reentered_password_edit->GetWindowRect(&control_rect);

            CRect new_control_for_that_location_rect;
            dialog->GetDlgItem(IDOK)->GetWindowRect(&new_control_for_that_location_rect);

            move_controls_and_resize_dialog(control_rect.top - new_control_for_that_location_rect.top, false);
        }

        WindowHelpers::CenterOnScreen(hwndDlg);

        return TRUE;
    }

    else if( uMsg == WM_COMMAND )
    {
        switch( HIWORD(wParam) )
        {
            // enable the OK button only when the passwords match
            case EN_CHANGE:
            {
                const CWnd* password_edit = dialog->GetDlgItem(IDC_EDIT_PASSWORD);
                const std::wstring password = WindowsWS::GetWindowText(password_edit);

                bool enable_ok = !password.empty();

                if( enable_ok )
                {
                    const CWnd* reentered_password_edit = dialog->GetDlgItem(IDC_EDIT_PASSWORD_REENTER);

                    if( reentered_password_edit->IsWindowVisible() )
                    {
                        const std::wstring reentered_password = WindowsWS::GetWindowText(reentered_password_edit);
                        enable_ok = ( password == reentered_password );
                    }
                }

                CWnd* ok_button = dialog->GetDlgItem(IDOK);
                ok_button->EnableWindow(enable_ok);

                return TRUE;
            }

            // process the button clicks
            case BN_CLICKED:
            {
                if( LOWORD(wParam) == IDOK )
                {
                    CWnd* password_edit = dialog->GetDlgItem(IDC_EDIT_PASSWORD);
                    const std::wstring password = WindowsWS::GetWindowText(password_edit);

                    try
                    {
                        m_instance->TryOpeningWithPassword(password);
                        EndDialog(hwndDlg, IDOK);
                    }

                    catch( const DataRepositoryException::EncryptionError& exception )
                    {
                        ErrorMessage::Display(exception);
                        password_edit->SetFocus();
                    }
                }

                else // cancel
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }

                return TRUE;
            }
        }
    }

    return FALSE;
}


DWORD WINAPI EncryptedSQLiteRepositoryPasswordManager::LaunchPasswordDialog(LPVOID pParam)
{
    const INT_PTR dialog_result = DialogBox(zDataODLL.hModule, MAKEINTRESOURCE(IDD_CSDBE_PASSWORD), nullptr, PasswordDialogProc);
    *reinterpret_cast<bool*>(pParam) = ( dialog_result == IDOK );
    return 0;
}


EncryptedSQLiteRepositoryPasswordManager* EncryptedSQLiteRepositoryPasswordManager::m_instance = nullptr;


bool EncryptedSQLiteRepositoryPasswordManager::QueryForPassword()
{
    m_instance = this;

    // create the dialog in a new thread so that the calling thread hangs until this completes;
    // this will ensure that this acts like a modal dialog regardless of whether it is called
    // from a MFC application, a WPF application, etc.
    bool result = false;

    const HANDLE dialog_thread = CreateThread(nullptr, 0, LaunchPasswordDialog, &result, 0, nullptr);

    if( dialog_thread != nullptr )
        WaitForSingleObject(dialog_thread, INFINITE);

    return result;
}


#else

#include <zUtilO/Interapp.h>
#include <zPlatformO/PlatformInterface.h>


bool EncryptedSQLiteRepositoryPasswordManager::QueryForPassword()
{
    const std::wstring title = GetPasswordQueryTitle();
    const std::wstring description = GetPasswordQueryDescription();
    const bool exists = PortableFunctions::FileIsRegular(m_filename);

    while( true )
    {
        const std::optional<std::wstring> password = PlatformInterface::GetInstance()->GetApplicationInterface()->GetPassword(title, description, exists);

        if( !password.has_value() )
            return false;

        try
        {
            TryOpeningWithPassword(*password);
            return true;
        }

        catch( const DataRepositoryException::EncryptionError& exception )
        {
            PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(_T(""), exception.GetErrorMessage(), MB_OK);
        }
    }
}

#endif
