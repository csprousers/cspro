#include "StdAfx.h"
#include "NewFileCreator.h"
#include "NewFileDlg.h"
#include "AplDoc.h"


CString NewFileCreator::GetBaseFilename(AppFileType app_file_type, const CString& application_filename, bool use_new_file_naming_scheme)
{
    if( !use_new_file_naming_scheme || app_file_type == AppFileType::Form ||
                                       app_file_type == AppFileType::Order ||
                                       app_file_type == AppFileType::TableSpec )
    {
        return PortableFunctions::PathRemoveFileExtension<CString>(application_filename);
    }

    return application_filename;
}


CString NewFileCreator::GetFilename(AppFileType app_file_type, const CString& application_filename, bool use_new_file_naming_scheme/* = true*/)
{
    return GetBaseFilename(app_file_type, application_filename, use_new_file_naming_scheme) +
        ( ( app_file_type == AppFileType::Form )         ? FileExtensions::WithDot::Form :
          ( app_file_type == AppFileType::Order )        ? FileExtensions::WithDot::Order :
          ( app_file_type == AppFileType::TableSpec )    ? FileExtensions::WithDot::TableSpec :
          ( app_file_type == AppFileType::Code )         ? FileExtensions::WithDot::Logic :
          ( app_file_type == AppFileType::Message )      ? FileExtensions::WithDot::Message :
          ( app_file_type == AppFileType::QuestionText ) ? FileExtensions::WithDot::QuestionText :
                                                           ReturnProgrammingError(_T("")) );
}


template<typename CF>
std::unique_ptr<CDataDict> NewFileCreator::CreateDictionary(NullTerminatedString dictionary_filename, CF customize_callback)
{
    ASSERT(!PortableFunctions::FileIsRegular(dictionary_filename));

    auto dictionary = std::make_unique<CDataDict>();

    // the label will be the filename
    dictionary->SetLabel(PortableFunctions::PathGetFilenameWithoutExtension<CString>(dictionary_filename));

    // the name will be label turned into a name
    CString base_name = CIMSAString::MakeNameRestrictLength(dictionary->GetLabel());
    dictionary->SetName(base_name + _T("_DICT"));

    dictionary->SetRecTypeStart(1);
    dictionary->SetRecTypeLen(1);
    dictionary->SetPosRelative(true);
    dictionary->SetZeroFill(false);
    dictionary->SetDecChar(true);
    dictionary->SetAllowDataViewerModifications(true);
    dictionary->SetAllowExport(true);

    // add an initial level
    DictLevel dict_level;

    dict_level.SetLabel(dictionary->GetLabel() + _T(" Level"));
    dict_level.SetName(base_name + _T("_LEVEL"));

    // add initial ID item
    CDictItem id_dict_item;
    id_dict_item.SetLabel(dictionary->GetLabel() + _T(" Identification"));
    id_dict_item.SetName(base_name + _T("_ID"));
    id_dict_item.SetStart(2);
    dict_level.GetIdItemsRec()->AddItem(&id_dict_item);

    // add an initial record
    CDictRecord dict_record;
    dict_record.SetLabel(dictionary->GetLabel() + _T(" Record"));
    dict_record.SetName(base_name + _T("_REC"));
    dict_record.SetRecTypeVal(_T("1"));
    dict_level.AddRecord(&dict_record);

    dictionary->AddLevel(std::move(dict_level));

    // customize the dictionary
    customize_callback(*dictionary);

    dictionary->BuildNameList();
    dictionary->UpdatePointers();

    dictionary->Save(dictionary_filename);

    return dictionary;
}


std::unique_ptr<CDataDict> NewFileCreator::CreateDictionary(NullTerminatedString dictionary_filename)
{
    return CreateDictionary(dictionary_filename, [](CDataDict& /*dictionary*/) {});
}


std::shared_ptr<const CDataDict> NewFileCreator::CreateOrGetOpenDictionary(NullTerminatedString dictionary_filename)
{
    // if necessary, create the dictionary 
    if( !PortableFunctions::FileIsRegular(dictionary_filename) )
        return CreateDictionary(dictionary_filename);

    // otherwise use an already-open dictionary...
    CMainFrame* pMainFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CMDlgBar& dlgBar = pMainFrame->GetDlgBar();
    CDDTreeCtrl& dictTree = dlgBar.m_DictTree;
    DictionaryDictTreeNode* dictionary_dict_tree_node = dictTree.GetDictionaryTreeNode(dictionary_filename);

    if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr )
        return dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary();

    // ...or load it from the disk
    return CDataDict::InstantiateAndOpen(dictionary_filename, true);
}


std::wstring NewFileCreator::GetDefaultWorkingStorageDictionaryFilename(wstring_view application_filename)
{
    return std::wstring(application_filename) + _T(".wrk") + FileExtensions::WithDot::Dictionary;
}


std::wstring NewFileCreator::CreateWorkingStorageDictionary(Application& application)
{
    std::wstring working_storage_dictionary_filename = GetDefaultWorkingStorageDictionaryFilename(application.GetApplicationFilename());

    if( !PortableFunctions::FileIsRegular(working_storage_dictionary_filename) )
    {
        CreateDictionary(working_storage_dictionary_filename,
            [&](CDataDict& dictionary)
            {
                // modify the default dictionary settings
                dictionary.SetLabel(application.GetLabel() + _T(" - Working Storage Dictionary"));
                dictionary.SetName(_T("WS_DICT"));

                DictLevel& dict_level = dictionary.GetLevel(0);
                dict_level.SetLabel(_T("Working Storage Level"));
                dict_level.SetName(_T("WS_LEVEL"));

                CDictItem* id_dict_item = dict_level.GetIdItemsRec()->GetItem(0);
                id_dict_item->SetLabel(_T("Dummy Id"));
                id_dict_item->SetName(_T("DUMMY_ID"));
                id_dict_item->SetStart(2);

                CDictRecord* dict_record = dict_level.GetRecord(0);
                dict_record->SetLabel(_T("Working Storage Record"));
                dict_record->SetName(_T("WS_REC"));

                // add a tabulation item
                if( application.GetEngineAppType() == EngineAppType::Tabulation)
                {
                    CDictItem total_dict_item;
                    total_dict_item.SetLabel(WORKVAR_TOTAL_LABEL);
                    total_dict_item.SetName(WORKVAR_TOTAL_NAME);
                    total_dict_item.SetStart(id_dict_item->GetStart() + id_dict_item->GetLen());
                    dict_record->AddItem(&total_dict_item);
                }
            });
    }

    application.AddExternalDictionaryFilename(WS2CS(working_storage_dictionary_filename));
    application.AddDictionaryDescription(DictionaryDescription(working_storage_dictionary_filename, DictionaryType::Working));

    return working_storage_dictionary_filename;
}


void NewFileCreator::CreateFormFile(const CString& form_filename, NullTerminatedString dictionary_filename, bool system_controlled)
{
    CMainFrame* pMainFrame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CWnd* pWnd = pMainFrame->GetActiveFrame();

    auto dictionary = CreateOrGetOpenDictionary(dictionary_filename);

    // create the form file, using the default drag options
    DragOptions drag_options;

    CDEFormFile form_file(form_filename, dictionary_filename);
    form_file.IsPathOn(system_controlled);
    form_file.CreateFormFile(dictionary.get(), pWnd->GetDC(), drag_options);

    if( !form_file.Save(form_filename) )
        throw CSProException(_T("Error saving the form file: %s"), form_filename.GetString());
}


void NewFileCreator::CreateOrderFile(const CString& order_filename, NullTerminatedString dictionary_filename)
{
    auto dictionary = CreateOrGetOpenDictionary(dictionary_filename);

    // create the order file
    CDEFormFile order_file(order_filename, dictionary_filename);
    order_file.CreateOrderFile(*dictionary, true);

    if( !order_file.Save(order_filename) )
        throw CSProException(_T("Error saving the order file: %s"), order_filename.GetString());
}




template<typename CF>
std::unique_ptr<Application> NewFileCreator::CreateApplication(EngineAppType engine_app_type, const CString& application_filename,
    bool use_new_file_naming_scheme, bool add_working_storage_dictionary, CF customize_callback)
{
    ASSERT(engine_app_type == EngineAppType::Entry ||
           engine_app_type == EngineAppType::Batch ||
           engine_app_type == EngineAppType::Tabulation);

    auto application = std::make_unique<Application>();

    application->SetApplicationFilename(application_filename);

    application->SetEngineAppType(engine_app_type);
    application->SetLogicSettings(LogicSettings::GetUserDefaultSettings());

    application->SetLabel(PortableFunctions::PathGetFilenameWithoutExtension<CString>(application_filename));
    application->SetName(WS2CS(CIMSAString::CreateUnreservedName(application->GetLabel())));

    // customize the application
    customize_callback(*application);

    // add the code and message files
    CAplDoc::AddDefaultCodeFile(*application, GetBaseFilename(AppFileType::Code, application_filename, use_new_file_naming_scheme));
    CAplDoc::AddDefaultMessageFile(*application, GetBaseFilename(AppFileType::Message, application_filename, use_new_file_naming_scheme));

    // conditionally add a working storage dictionary
    if( add_working_storage_dictionary )
        CreateWorkingStorageDictionary(*application);

    // save the application
    application->Save(application_filename);

    return application;
}


std::unique_ptr<Application> NewFileCreator::CreateEntryApplication(const CString& entry_application_filename, NullTerminatedString dictionary_filename,
    EntryApplicationStyle style/* = EntryApplicationStyle::CAPI*/, bool use_new_file_naming_scheme/* = true*/,
    bool add_working_storage_dictionary/* = false*/)
{
    bool capi_style = ( style == EntryApplicationStyle::CAPI );
    bool papi_style = ( style == EntryApplicationStyle::PAPI );
    bool operational_control_style = ( style == EntryApplicationStyle::OperationalControl );
    bool capi_or_operational_control_style = ( capi_style || operational_control_style );

    std::unique_ptr<Application> application = CreateApplication(EngineAppType::Entry, entry_application_filename,
        use_new_file_naming_scheme, add_working_storage_dictionary,
        [&](Application& application)
        {
            // customize the entry application based on the style
            bool system_controlled = capi_or_operational_control_style;

            application.SetAskOperatorId(papi_style);
            application.SetUseQuestionText(capi_or_operational_control_style);
            application.SetCreateListingFile(papi_style);
            application.SetCreateLogFile(papi_style);
            application.SetAutoAdvanceOnSelection(operational_control_style);
            application.SetShowErrorMessageNumbers(papi_style);
            application.SetComboBoxShowOnlyDiscreteValues(capi_or_operational_control_style);

            if( capi_style )
            {
                application.GetApplicationProperties().GetParadataProperties().SetCollectionType(ParadataProperties::CollectionType::AllEvents);
            }

            else if( operational_control_style )
            {
                application.SetCaseTreeType(CaseTreeType::Never);
            }


            // if necessary, create the form file (and dictionary)
            CString form_filename = GetFilename(AppFileType::Form, entry_application_filename);

            if( !PortableFunctions::FileIsRegular(form_filename) )
                CreateFormFile(form_filename, dictionary_filename, system_controlled);

            application.AddFormFilename(form_filename);
            application.AddDictionaryDescription(DictionaryDescription(dictionary_filename, CS2WS(form_filename), DictionaryType::Input));


            // if necessary, create the question text file
            CString question_text_filename = GetFilename(AppFileType::QuestionText, entry_application_filename, use_new_file_naming_scheme);

            if( !PortableFunctions::FileIsRegular(question_text_filename) )
            {
                CapiQuestionManager question_manager;
                question_manager.Save(CS2WS(question_text_filename));
            }

            application.SetQuestionTextFilename(question_text_filename);
        });


    // if using the operational control style, create a PFF with some options automatically set
    if( operational_control_style )
    {
        CString pff_filename = PortableFunctions::PathRemoveFileExtension<CString>(entry_application_filename) + FileExtensions::WithDot::Pff;

        if( !PortableFunctions::FileIsRegular(pff_filename) )
        {
            PFF pff;
            pff.SetPifFileName(pff_filename);
            pff.SetAppFName(entry_application_filename);
            pff.SetSingleInputDataConnectionString(ConnectionString::CreateNullRepositoryConnectionString());
            pff.SetStartMode(StartMode::Add, CString());
            pff.SetLockFlag(LockFlag::CaseListing, true);
            pff.SetFullScreenFlag(FULLSCREEN::FULLSCREEN_YES);
            pff.Save(true);
        }
    }

    return application;
}


std::unique_ptr<Application> NewFileCreator::CreateBatchApplication(const CString& batch_application_filename,
    NullTerminatedString dictionary_filename, bool use_new_file_naming_scheme/* = true*/, bool add_working_storage_dictionary/* = false*/)
{
    return CreateApplication(EngineAppType::Batch, batch_application_filename, use_new_file_naming_scheme, add_working_storage_dictionary,
        [&](Application& application)
        {
            // if necessary, create the order file (and dictionary)
            CString order_filename = GetFilename(AppFileType::Order, batch_application_filename);

            if( !PortableFunctions::FileIsRegular(order_filename) )
                CreateOrderFile(order_filename, dictionary_filename);

            application.AddFormFilename(order_filename);
            application.AddDictionaryDescription(DictionaryDescription(dictionary_filename, CS2WS(order_filename), DictionaryType::Input));
        });
}


std::unique_ptr<Application> NewFileCreator::CreateTabulationApplication(const CString& tabulation_application_filename,
    NullTerminatedString dictionary_filename, bool use_new_file_naming_scheme/* = true*/)
{
    return CreateApplication(EngineAppType::Tabulation, tabulation_application_filename, use_new_file_naming_scheme, true,
        [&](Application& application)
        {
            // if necessary, create the table spec file (and dictionary)
            CString tab_spec_filename = GetFilename(AppFileType::TableSpec, tabulation_application_filename);

            if( !PortableFunctions::FileIsRegular(tab_spec_filename) )
            {
                auto dictionary = CreateOrGetOpenDictionary(dictionary_filename);

                CTabSet table_spec(dictionary);

                //Savy (R) sampling app 20090206
                if( table_spec.GetName().CompareNoCase(dictionary->GetName()) == 0 )
                    table_spec.SetName(table_spec.GetName() + _T("_TAB"));

                if( !table_spec.Save(tab_spec_filename, dictionary_filename) )
                    throw CSProException(_T("Error saving the table specification file: %s"), tab_spec_filename.GetString());
            }

            application.AddTabSpecFilename(tab_spec_filename);
            application.AddDictionaryDescription(DictionaryDescription(dictionary_filename, CS2WS(tab_spec_filename), DictionaryType::Input));
        });
}


std::optional<std::tuple<CString, AppFileType>> NewFileCreator::InteractiveMode()
{
    // show a dialog, prompting what kind of file to open
    NewFileDlg new_file_dlg;

    if( new_file_dlg.DoModal() != IDOK )
        return std::nullopt;

    AppFileType app_file_type = new_file_dlg.GetAppFileType();
    EntryApplicationStyle entry_application_style = new_file_dlg.GetEntryApplicationStyle();
    CString expected_extension = GetFileExtension(app_file_type);

    CString filename;
    bool use_new_file_naming_scheme = true;

    // use the last folder as the initial directory when prompting for filenames
    CString directory = AfxGetApp()->GetProfileString(_T("Settings"), _T("Last Folder"));

    // loop until the inputs are properly satisfied
    while( true )
    {
        CString file_filter = FormatText(_T("%s Files (*.%s)|*.%s|All Files (*.*)|*.*||"),
                                         ToString(app_file_type), expected_extension.GetString(), expected_extension.GetString());
        CIMSAFileDialog file_dlg(FALSE, expected_extension, nullptr, OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_PATHMUSTEXIST, file_filter);

        if( !directory.IsEmpty() )
            file_dlg.m_ofn.lpstrInitialDir = directory;

        CString title = FormatText(_T("New %s Name"), ToString(app_file_type));
        file_dlg.m_ofn.lpstrTitle = title;

        if( file_dlg.DoModal() != IDOK )
            return std::nullopt;

        filename = file_dlg.GetPathName();
        directory = PortableFunctions::PathGetDirectory<CString>(filename);

        // make sure the extension is correct
        if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(filename), expected_extension) )
        {
            ASSERT(false);
            filename = PortableFunctions::PathAppendFileExtension(filename, expected_extension);
        }

        // don't allow the selection of existing files
        if( PortableFunctions::FileIsRegular(filename) )
        {
            AfxMessageBox(FormatText(_T("%s already exists!"), filename.GetString()), MB_OK | MB_ICONEXCLAMATION);
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
            std::vector<std::vector<CString>> existing_files(2);

            for( size_t i = 0; i < 2; ++i )
            {
                bool new_style = ( i == 0 );

                for( AppFileType file_type_to_check : file_types_to_check )
                {
                    CString this_filename = GetFilename(file_type_to_check, filename, new_style);

                    if( PortableFunctions::FileIsRegular(this_filename) )
                        existing_files[i].emplace_back(this_filename);
                };
            }

            // use the old style only if more files exist with that approach
            use_new_file_naming_scheme = ( existing_files[0].size() >= existing_files[1].size() );

            // confirm that any files should be used
            const auto& existing_files_set = existing_files[use_new_file_naming_scheme ? 0 : 1];

            if( !existing_files_set.empty() )
            {
                std::wstring message = _T("The following files are present. Do you want to use all of them?\n\n") +
                                       SO::CreateSingleString(existing_files_set, _T("\n"));

                if( AfxMessageBox(message.c_str(), MB_YESNO) != IDYES )
                    continue;
            }
        }

        // if here, all inputs have been satisfied
        break;
    }


    // dictionary
    // ----------------------------------------------
    if( app_file_type == AppFileType::Dictionary )
    {
        CreateDictionary(filename);
    }


    // form file
    // ----------------------------------------------
    else if( app_file_type == AppFileType::Form )
    {
        // query for the input dictionary
        CAplFileAssociationsDlg file_associations_dlg;
        file_associations_dlg.m_sAppName = filename;
        file_associations_dlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, ("Input Dictionary"), true);
        file_associations_dlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::Hidden;
        file_associations_dlg.m_sTitle = _T("New Form File");

        if( file_associations_dlg.DoModal() != IDOK )
            return std::nullopt;

        CString dictionary_filename = file_associations_dlg.m_fileAssociations.front().GetNewFilename();

        CreateFormFile(filename, dictionary_filename, false);
    }


    // applications
    // ----------------------------------------------
    else
    {
        ASSERT(IsApplicationType(app_file_type));

        // query for the inputs
        CAplFileAssociationsDlg file_associations_dlg;
        file_associations_dlg.m_sAppName = filename;
        file_associations_dlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, ("Input Dictionary"), true);

        // give room for three external dictionaries
        constexpr size_t NumberExternalDictionaries = 3;

        for( int i = 1; i <= NumberExternalDictionaries; ++i )
            file_associations_dlg.m_fileAssociations.emplace_back(FileAssociation::Type::Dictionary, FormatText(_T("External Dictionary %d"), i), false);

        // prefill in the working storage dictionary for tabulation applications
        if( app_file_type == AppFileType::ApplicationTabulation )
        {
            file_associations_dlg.m_bWorkingStorage = TRUE;
            file_associations_dlg.m_workingStorageType = CAplFileAssociationsDlg::WorkingStorageType::ReadOnly;
            file_associations_dlg.m_sWSDName = WS2CS(GetDefaultWorkingStorageDictionaryFilename(filename));
        }

        file_associations_dlg.m_sTitle = FormatText(_T("New %s"), ToString(app_file_type));

        if( file_associations_dlg.DoModal() != IDOK )
            return std::nullopt;

        // gets a dictionary filename and ensures that it has the proper extension
        auto get_dictionary_filename = [&](size_t index)
        {
            CString dictionary_filename = file_associations_dlg.m_fileAssociations[index].GetNewFilename();

            if( SO::IsBlank(dictionary_filename) )
                return CString();

            dictionary_filename = WS2CS(MakeFullPath(directory, CS2WS(dictionary_filename.Trim())));

            if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(dictionary_filename), FileExtensions::Dictionary) )
                dictionary_filename.Append(FileExtensions::WithDot::Dictionary);

            return dictionary_filename;
        };

        CString input_dictionary_filename = get_dictionary_filename(0);
        ASSERT(!input_dictionary_filename.IsEmpty());

        if( !PortableFunctions::FileIsRegular(input_dictionary_filename) )
        {
            if( AfxMessageBox(_T("The dictionary does not exist, create it?\n\n") + input_dictionary_filename, MB_YESNO) != IDYES )
                return std::nullopt;
        }

        std::unique_ptr<Application> application;

        if( app_file_type == AppFileType::ApplicationEntry )
        {
            application = CreateEntryApplication(filename, input_dictionary_filename, entry_application_style,
                                                 use_new_file_naming_scheme, file_associations_dlg.m_bWorkingStorage);
        }

        else if( app_file_type == AppFileType::ApplicationBatch )
        {
            application = CreateBatchApplication(filename, input_dictionary_filename,
                                                 use_new_file_naming_scheme, file_associations_dlg.m_bWorkingStorage);
        }

        else
        {
            application = CreateTabulationApplication(filename, input_dictionary_filename, use_new_file_naming_scheme);
        }

        // add (and create) any external dictionaries that are not already part of the application
        bool added_external_dictionaries = false;

        for( size_t i = 1; i <= NumberExternalDictionaries; ++i )
        {
            CString external_dictionary_filename = get_dictionary_filename(i);

            if( external_dictionary_filename.IsEmpty() ||
                external_dictionary_filename.CompareNoCase(input_dictionary_filename) == 0 ||
                application->GetDictionaryDescription(external_dictionary_filename) != nullptr )
            {
                continue;
            }

            if( !PortableFunctions::FileIsRegular(external_dictionary_filename) )
                CreateDictionary(external_dictionary_filename);

            application->AddExternalDictionaryFilename(external_dictionary_filename);
            application->AddDictionaryDescription(DictionaryDescription(CS2WS(external_dictionary_filename), DictionaryType::External));

            added_external_dictionaries = true;
        }

        // resave the application if dictionaries were added
        if( added_external_dictionaries )
            application->Save(filename);
    }


    AfxGetApp()->WriteProfileString(_T("Settings"), _T("Last Folder"), directory);

    return std::make_tuple(filename, app_file_type);
}
