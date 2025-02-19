#include "StdAfx.h"
#include "InteractiveNewFileCreator.h"
#include "NewFileDlg.h"


std::optional<std::tuple<std::string, AppFileType>> InteractiveNewFileCreator::InteractiveMode()
{
    // show a dialog, prompting what kind of file to open
    NewFileDlg new_file_dlg;

    if( new_file_dlg.DoModal() != IDOK )
        return std::nullopt;

    const AppFileType app_file_type = new_file_dlg.GetAppFileType();
    const EntryApplicationStyle entry_application_style = new_file_dlg.GetEntryApplicationStyle();
    const std::string expected_extension = GetFileExtension(app_file_type);

    std::string file_path;
    bool use_new_file_naming_scheme = true;

    // use the last folder as the initial directory when prompting for filenames
    std::string directory = TC::ToUtf8(AfxGetApp()->GetProfileString(L"Settings", L"Last Folder"));

    // loop until the inputs are properly satisfied
    while( true )
    {
        const std::string filter = FormatText("%s Files (*.%s)|*.%s|All Files (*.*)|*.*||",
                                              ToString(app_file_type), expected_extension.c_str(), expected_extension.c_str());

        SaveFileDlg save_file_dlg(OFN_HIDEREADONLY | OFN_CREATEPROMPT, expected_extension, directory, filter);
        save_file_dlg.SetTitle(FormatText("New %s Name", ToString(app_file_type)));

        if( save_file_dlg.DoModal() != IDOK )
            return std::nullopt;

        file_path = save_file_dlg.GetFilePath();
        directory = PortableFunctions::PathGetDirectory(file_path);

        // make sure the extension is correct
        if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), expected_extension) )
        {
            ASSERT(false);
            PortableFunctions::MakePathAppendFileExtension(file_path, expected_extension);
        }

        // don't allow the selection of existing files
        if( PortableFunctions::FileIsRegular(file_path) )
        {
            ErrorMessage::Display("The file already exists: " + file_path);
            continue;
        }

        // only create applications if the user agrees to reuse any related files that exist
        if( IsApplicationType(app_file_type) )
        {
            std::vector<AppFileType> file_types_to_check =
            {
                ( app_file_type == AppFileType::ApplicationEntry ) ? AppFileType::Form :
                ( app_file_type == AppFileType::ApplicationBatch ) ? AppFileType::Order :
                                                                     AppFileType::TableSpec,
                AppFileType::Code,
                AppFileType::Message
            };

            if( app_file_type == AppFileType::ApplicationEntry )
                file_types_to_check.emplace_back(AppFileType::QuestionText);

            // check files using the new and old naming schemes
            std::vector<std::vector<std::string>> existing_files(2);

            for( size_t i = 0; i < 2; ++i )
            {
                const bool new_style = ( i == 0 );

                for( const AppFileType file_type_to_check : file_types_to_check )
                {
                    std::string this_file_path = GetDefaultFilePath(file_type_to_check, file_path, new_style);

                    if( PortableFunctions::FileIsRegular(this_file_path) )
                        existing_files[i].emplace_back(std::move(this_file_path));
                };
            }

            // use the old style only if more files exist with that approach
            use_new_file_naming_scheme = ( existing_files[0].size() >= existing_files[1].size() );

            // confirm that any files should be used
            const std::vector<std::string>& existing_files_set = existing_files[use_new_file_naming_scheme ? 0 : 1];

            if( !existing_files_set.empty() )
            {
                const std::string message = "The following files are present. Do you want to use all of them?\n\n" +
                                            SO::CreateSingleString(existing_files_set, SO::Newline_lf_sv);

                if( AfxMessageBox(TC::ToWide(message).c_str(), MB_YESNO) != IDYES )
                    continue;
            }
        }

        // if here, all inputs have been satisfied
        break;
    }


    // dictionary
    // --------------------------------------------------------------------------
    if( app_file_type == AppFileType::Dictionary )
    {
        CreateDictionary(file_path);
    }


    // form file
    // --------------------------------------------------------------------------
    else if( app_file_type == AppFileType::Form )
    {
        // query for the input dictionary
        CAplFileAssociationsDlg file_associations_dlg;
        file_associations_dlg.m_sAppName = UTF8_TODO::GetCString(file_path);
        file_associations_dlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, L"Input Dictionary", true);
        file_associations_dlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::Hidden;
        file_associations_dlg.m_sTitle = L"New Form File";

        if( file_associations_dlg.DoModal() != IDOK )
            return std::nullopt;

        const std::string& dictionary_file_path = UTF8_TODO::GetUtf8(file_associations_dlg.m_fileAssociations.front().GetNewFilename());

        CreateFormFile(file_path, dictionary_file_path, false);
    }


    // applications
    // --------------------------------------------------------------------------
    else
    {
        ASSERT(IsApplicationType(app_file_type));

        // query for the inputs
        CAplFileAssociationsDlg file_associations_dlg;
        file_associations_dlg.m_sAppName = UTF8_TODO::GetCString(file_path);
        file_associations_dlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, L"Input Dictionary", true);

        // give room for three external dictionaries
        constexpr size_t NumberExternalDictionaries = 3;

        for( int i = 1; i <= NumberExternalDictionaries; ++i )
            file_associations_dlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, FormatText(L"External Dictionary %d", i), false);

        // prefill in the working storage dictionary for tabulation applications
        if( app_file_type == AppFileType::ApplicationTabulation )
        {
            file_associations_dlg.m_bWorkingStorage = TRUE;
            file_associations_dlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::ReadOnly;
            file_associations_dlg.m_sWSDName = UTF8_TODO::GetCString(GetDefaultWorkingStorageDictionaryFilePath(file_path));
        }

        file_associations_dlg.m_sTitle = FormatText(L"New %s", UTF8_TODO::GetWide(ToString(app_file_type)).c_str());

        if( file_associations_dlg.DoModal() != IDOK )
            return std::nullopt;

        // gets a dictionary file path and ensures that it has the proper extension
        auto get_dictionary_file_path = [&](const size_t index)
        {
            std::string dictionary_file_path = UTF8_TODO::GetUtf8(file_associations_dlg.m_fileAssociations[index].GetNewFilename());
            SO::MakeTrim(dictionary_file_path);

            if( dictionary_file_path.empty() )
                return std::string();

            return PortableFunctions::PathEnsureFileExtension(MakeFullPath(directory, std::move(dictionary_file_path)),
                                                              FileExtensions::Dictionary);
        };

        const std::string input_dictionary_file_path = get_dictionary_file_path(0);
        ASSERT(!input_dictionary_file_path.empty());

        if( !PortableFunctions::FileIsRegular(input_dictionary_file_path) )
        {
            if( AfxMessageBox("The dictionary does not exist, create it?\n\n" + input_dictionary_file_path, MB_YESNO) != IDYES )
                return std::nullopt;
        }

        std::unique_ptr<Application> application;

        if( app_file_type == AppFileType::ApplicationEntry )
        {
            application = CreateEntryApplication(file_path, input_dictionary_file_path, entry_application_style,
                                                 use_new_file_naming_scheme, file_associations_dlg.m_bWorkingStorage);
        }

        else if( app_file_type == AppFileType::ApplicationBatch )
        {
            application = CreateBatchApplication(file_path, input_dictionary_file_path,
                                                 use_new_file_naming_scheme, file_associations_dlg.m_bWorkingStorage);
        }

        else
        {
            application = CreateTabulationApplication(file_path, input_dictionary_file_path, use_new_file_naming_scheme);
        }

        // add (and create) any external dictionaries that are not already part of the application
        bool added_external_dictionaries = false;

        for( size_t i = 1; i <= NumberExternalDictionaries; ++i )
        {
            const std::string external_dictionary_file_path = get_dictionary_file_path(i);

            if( external_dictionary_file_path.empty() ||
                SO::EqualsNoCase(external_dictionary_file_path, input_dictionary_file_path) ||
                application->GetDictionaryDescription(external_dictionary_file_path) != nullptr )
            {
                continue;
            }

            if( !PortableFunctions::FileIsRegular(external_dictionary_file_path) )
                CreateDictionary(external_dictionary_file_path);

            application->AddExternalDictionary(external_dictionary_file_path);
            application->AddDictionaryDescription(DictionaryDescription(external_dictionary_file_path, DictionaryType::External));

            added_external_dictionaries = true;
        }

        // resave the application if dictionaries were added
        if( added_external_dictionaries )
            application->Save(file_path);
    }


    AfxGetApp()->WriteProfileString(L"Settings", L"Last Folder", TC::ToWide(directory).c_str());

    return std::make_tuple(std::move(file_path), app_file_type);
}
