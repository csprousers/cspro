#include "StdAfx.h"
#include "NewFileCreator.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/TextSourceEditable.h>
#include <zAppO/PFF.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zFormO/DragOptions.h>
#include <zTableO/Table.h>
#include <zCapiO/CapiQuestionManager.h>


std::string NewFileCreator::GetDefaultFilePath(const AppFileType app_file_type, const std::string& application_file_path,
                                               const bool use_new_file_naming_scheme/* = true*/)
{
    const bool strip_extension = ( !use_new_file_naming_scheme ||
                                   app_file_type == AppFileType::Form ||
                                   app_file_type == AppFileType::Order ||
                                   app_file_type == AppFileType::TableSpec );

    std::string file_path = strip_extension ? PortableFunctions::PathRemoveFileExtension(application_file_path) :
                                              application_file_path;

    return PortableFunctions::MakePathAppendFileExtension(file_path, GetFileExtension(app_file_type));
}


template<typename CF>
std::unique_ptr<CDataDict> NewFileCreator::CreateDictionary(const std::string& dictionary_file_path, const CF customize_callback)
{
    ASSERT(!PortableFunctions::FileIsRegular(dictionary_file_path));

    auto dictionary = std::make_unique<CDataDict>();

    // the label will be the filename
    dictionary->SetLabel(UTF8_TODO::GetCString(Path::GetFilenameWithoutExtension(dictionary_file_path)));

    // the name will be the label turned into a name
    const std::string base_name = UTF8_TODO::GetUtf8(CIMSAString::MakeNameRestrictLength(dictionary->GetLabel()));
    dictionary->SetName(base_name + "_DICT");

    dictionary->SetRecTypeStart(1);
    dictionary->SetRecTypeLen(1);
    dictionary->SetPosRelative(true);
    dictionary->SetZeroFill(false);
    dictionary->SetDecChar(true);
    dictionary->SetAllowDataManagerModifications(true);
    dictionary->SetAllowExport(true);

    // add an initial level
    DictLevel dict_level;

    dict_level.SetLabel(dictionary->GetLabel() + _T(" Level"));
    dict_level.SetName(base_name + "_LEVEL");

    // add initial ID item
    CDictItem id_dict_item;
    id_dict_item.SetLabel(dictionary->GetLabel() + _T(" Identification"));
    id_dict_item.SetName(base_name + "_ID");
    id_dict_item.SetStart(2);
    dict_level.GetIdItemsRec()->AddItem(&id_dict_item);

    // add an initial record
    CDictRecord dict_record;
    dict_record.SetLabel(dictionary->GetLabel() + _T(" Record"));
    dict_record.SetName(base_name + "_REC");
    dict_record.SetRecTypeVal(_T("1"));
    dict_level.AddRecord(&dict_record);

    dictionary->AddLevel(std::move(dict_level));

    // customize the dictionary
    customize_callback(*dictionary);

    dictionary->BuildNameList();
    dictionary->UpdatePointers();

    dictionary->Save(dictionary_file_path);

    return dictionary;
}


std::unique_ptr<CDataDict> NewFileCreator::CreateDictionary(const std::string& dictionary_file_path)
{
    return CreateDictionary(dictionary_file_path, [](CDataDict& /*dictionary*/) {});
}


std::shared_ptr<const CDataDict> NewFileCreator::GetDictionary(const std::string& dictionary_file_path)
{
    // try to use an already-open dictionary...
    std::variant<std::monostate, std::shared_ptr<const CDEFormFile>, std::shared_ptr<const CDataDict>> form_file_or_dictionary;

    if( WindowsDesktopMessage::Send(UWM::Designer::GetFormFileOrDictionary, &form_file_or_dictionary, &dictionary_file_path) == 1 )
    {
        ASSERT(std::holds_alternative<std::shared_ptr<const CDataDict>>(form_file_or_dictionary));
        return std::move(std::get<std::shared_ptr<const CDataDict>>(form_file_or_dictionary));
    }

    // ...or load it from the disk
    return CDataDict::InstantiateAndOpen(dictionary_file_path, true);
}


std::shared_ptr<const CDataDict> NewFileCreator::CreateOrGetDictionary(const std::string& dictionary_file_path)
{
    return PortableFunctions::FileIsRegular(dictionary_file_path) ? GetDictionary(dictionary_file_path) :
                                                                    CreateDictionary(dictionary_file_path);
}


std::string NewFileCreator::GetDefaultWorkingStorageDictionaryFilePath(const std::string& application_file_path)
{
    return SO::Concatenate(application_file_path, ".wrk.", FileExtensions::Dictionary);
}


std::string NewFileCreator::CreateWorkingStorageDictionary(const Application& application)
{
    std::string working_storage_dictionary_file_path = GetDefaultWorkingStorageDictionaryFilePath(application.GetApplicationFilePath());

    if( !PortableFunctions::FileIsRegular(working_storage_dictionary_file_path) )
    {
        CreateDictionary(working_storage_dictionary_file_path,
            [&](CDataDict& dictionary)
            {
                // modify the default dictionary settings
                dictionary.SetLabel(UTF8_TODO::GetCString(application.GetLabel() + " - Working Storage Dictionary"));
                dictionary.SetName("WS_DICT");

                DictLevel& dict_level = dictionary.GetLevel(0);
                dict_level.SetLabel(_T("Working Storage Level"));
                dict_level.SetName("WS_LEVEL");

                CDictItem* id_dict_item = dict_level.GetIdItemsRec()->GetItem(0);
                id_dict_item->SetLabel(_T("Dummy Id"));
                id_dict_item->SetName("DUMMY_ID");
                id_dict_item->SetStart(2);

                CDictRecord* dict_record = dict_level.GetRecord(0);
                dict_record->SetLabel(_T("Working Storage Record"));
                dict_record->SetName("WS_REC");

                // add a tabulation item
                if( application.GetEngineAppType() == EngineAppType::Tabulation)
                {
                    CDictItem total_dict_item;
                    total_dict_item.SetLabel(WORKVAR_TOTAL_LABEL);
                    total_dict_item.SetName(UTF8_TODO::GetUtf8(WORKVAR_TOTAL_NAME));
                    total_dict_item.SetStart(id_dict_item->GetStart() + id_dict_item->GetLen());
                    dict_record->AddItem(&total_dict_item);
                }
            });
    }

    return working_storage_dictionary_file_path;
}


std::string NewFileCreator::CreateWorkingStorageDictionary(Application& application, const bool add_to_application_object)
{
    std::string working_storage_dictionary_file_path = CreateWorkingStorageDictionary(application);

    if( add_to_application_object )
    {
        application.AddExternalDictionary(working_storage_dictionary_file_path);
        application.AddDictionaryDescription(DictionaryDescription(working_storage_dictionary_file_path, DictionaryType::Working));
    }

    return working_storage_dictionary_file_path;
}


void NewFileCreator::CreateFormFile(const std::string& form_file_path, const std::string& dictionary_file_path, const bool system_controlled)
{
    CWnd* const main_wnd = AfxGetMainWnd();
    CWnd* const active_wnd = ( main_wnd != nullptr && main_wnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)) ) ? assert_cast<CFrameWnd*>(main_wnd)->GetActiveFrame() :
                                                                                                       nullptr;

    const std::variant<CDC*, CSize> pDC_or_single_character_text_extent = ( active_wnd != nullptr ) ? std::variant<CDC*, CSize>(active_wnd->GetDC()) :
                                                                                                      ReturnProgrammingError(std::variant<CDC*, CSize>(CSize(8, 16)));

    const std::shared_ptr<const CDataDict> dictionary = CreateOrGetDictionary(dictionary_file_path);

    // create the form file, using the default drag options
    DragOptions drag_options;

    CDEFormFile form_file(UTF8_TODO::GetCString(form_file_path), UTF8_TODO::GetCString(dictionary_file_path));
    form_file.IsPathOn(system_controlled);
    form_file.CreateFormFile(dictionary.get(), pDC_or_single_character_text_extent, drag_options);

    if( !form_file.Save(form_file_path) )
        throw CSProException("Error saving the form file: " + form_file_path);
}


std::shared_ptr<const CDEFormFile> NewFileCreator::GetFormFile(const std::string& form_file_path)
{
    // try to use an already-open form file...
    std::variant<std::monostate, std::shared_ptr<const CDEFormFile>, std::shared_ptr<const CDataDict>> form_file_or_dictionary;

    if( WindowsDesktopMessage::Send(UWM::Designer::GetFormFileOrDictionary, &form_file_or_dictionary, &form_file_path) == 1 )
    {
        ASSERT(std::holds_alternative<std::shared_ptr<const CDEFormFile>>(form_file_or_dictionary));
        return std::move(std::get<std::shared_ptr<const CDEFormFile>>(form_file_or_dictionary));
    }

    // ...or load it from the disk
    auto form_file = std::make_unique<CDEFormFile>();

    if( !form_file->Open(form_file_path, true) )
        throw CSProException("There was an error reading the form file " + form_file_path);

    return form_file;
}


void NewFileCreator::CreateOrderFile(const std::string& order_file_path, const std::string& dictionary_file_path)
{
    const std::shared_ptr<const CDataDict> dictionary = CreateOrGetDictionary(dictionary_file_path);

    // create the order file
    CDEFormFile order_file(UTF8_TODO::GetCString(order_file_path), UTF8_TODO::GetCString(dictionary_file_path));
    order_file.CreateOrderFile(*dictionary, true);

    if( !order_file.Save(order_file_path) )
        throw CSProException("Error saving the order file: " + order_file_path);
}


template<typename CF>
std::unique_ptr<Application> NewFileCreator::CreateApplication(const EngineAppType engine_app_type, const std::string& application_file_path,
                                                               const bool use_new_file_naming_scheme, const bool add_working_storage_dictionary,
                                                               const CF customize_callback)
{
    ASSERT(engine_app_type == EngineAppType::Entry ||
           engine_app_type == EngineAppType::Batch ||
           engine_app_type == EngineAppType::Tabulation);

    auto application = std::make_unique<Application>();

    application->SetApplicationFilePath(application_file_path);

    application->SetEngineAppType(engine_app_type);
    application->SetLogicSettings(LogicSettings::GetUserDefaultSettings());

    application->SetLabel(Path::GetFilenameWithoutExtension(application_file_path));
    application->SetName(CIMSAString::CreateUnreservedName(application->GetLabel()));

    // customize the application
    customize_callback(*application);

    // add the code and message files
    application->AddCodeFile(CreateOrOpenCodeFile(*application, use_new_file_naming_scheme));
    application->AddMessageFile(CreateOrOpenMessageFile(*application, use_new_file_naming_scheme));

    // conditionally add a working storage dictionary
    if( add_working_storage_dictionary )
        CreateWorkingStorageDictionary(*application);

    // save the application
    application->Save(application_file_path);

    return application;
}


std::unique_ptr<Application> NewFileCreator::CreateEntryApplication(const std::string& entry_application_file_path,
                                                                    const std::string& dictionary_file_path,
                                                                    const EntryApplicationStyle style/* = EntryApplicationStyle::CAPI*/,
                                                                    const bool use_new_file_naming_scheme/* = true*/,
                                                                    const bool add_working_storage_dictionary/* = false*/)
{
    const bool capi_style = ( style == EntryApplicationStyle::CAPI );
    const bool papi_style = ( style == EntryApplicationStyle::PAPI );
    const bool operational_control_style = ( style == EntryApplicationStyle::OperationalControl );
    const bool capi_or_operational_control_style = ( capi_style || operational_control_style );

    std::unique_ptr<Application> application = CreateApplication(EngineAppType::Entry, entry_application_file_path,
                                                                 use_new_file_naming_scheme, add_working_storage_dictionary,
        [&](Application& application)
        {
            // customize the entry application based on the style
            const bool system_controlled = capi_or_operational_control_style;

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
            std::string form_file_path = GetDefaultFilePath(AppFileType::Form, entry_application_file_path);

            if( !PortableFunctions::FileIsRegular(form_file_path) )
                CreateFormFile(form_file_path, dictionary_file_path, system_controlled);

            application.AddForm(form_file_path);
            application.AddDictionaryDescription(DictionaryDescription(dictionary_file_path, std::move(form_file_path), DictionaryType::Input));


            // if necessary, create the question text file
            std::string question_text_file_path = GetDefaultFilePath(AppFileType::QuestionText, entry_application_file_path, use_new_file_naming_scheme);

            if( !PortableFunctions::FileIsRegular(question_text_file_path) )
            {
                CapiQuestionManager question_manager;
                question_manager.Save(question_text_file_path);
            }

            application.SetQuestionTextFilePath(std::move(question_text_file_path));
        });


    // if using the operational control style, create a PFF with some options automatically set
    if( operational_control_style )
    {
        std::string pff_file_path = PortableFunctions::PathReplaceFileExtension(entry_application_file_path, FileExtensions::Pff);

        if( !PortableFunctions::FileIsRegular(pff_file_path) )
        {
            PFF pff;
            pff.SetPifFileName(std::move(UTF8_TODO::GetCString(pff_file_path)));
            pff.SetAppFName(UTF8_TODO::GetCString(entry_application_file_path));
            pff.SetSingleInputDataConnectionString(ConnectionString::CreateNullRepositoryConnectionString());
            pff.SetStartMode(StartMode::Add, CString());
            pff.SetLockFlag(LockFlag::CaseListing, true);
            pff.SetFullScreenFlag(FULLSCREEN::FULLSCREEN_YES);
            pff.Save(true);
        }
    }

    return application;
}


std::unique_ptr<Application> NewFileCreator::CreateBatchApplication(const std::string& batch_application_file_path,
                                                                    const std::string& dictionary_file_path,
                                                                    const bool use_new_file_naming_scheme/* = true*/,
                                                                    const bool add_working_storage_dictionary/* = false*/)
{
    return CreateApplication(EngineAppType::Batch, batch_application_file_path,
                             use_new_file_naming_scheme, add_working_storage_dictionary,
        [&](Application& application)
        {
            // if necessary, create the order file (and dictionary)
            std::string order_file_path = GetDefaultFilePath(AppFileType::Order, batch_application_file_path);

            if( !PortableFunctions::FileIsRegular(order_file_path) )
                CreateOrderFile(order_file_path, dictionary_file_path);

            application.AddForm(order_file_path);
            application.AddDictionaryDescription(DictionaryDescription(dictionary_file_path, std::move(order_file_path), DictionaryType::Input));
        });
}


std::unique_ptr<Application> NewFileCreator::CreateTabulationApplication(const std::string& tabulation_application_file_path,
                                                                         const std::string& dictionary_file_path,
                                                                         const bool use_new_file_naming_scheme/* = true*/)
{
    return CreateApplication(EngineAppType::Tabulation, tabulation_application_file_path,
                             use_new_file_naming_scheme, true,
        [&](Application& application)
        {
            // if necessary, create the table spec file (and dictionary)
            std::string table_spec_file_path = GetDefaultFilePath(AppFileType::TableSpec, tabulation_application_file_path);

            if( !PortableFunctions::FileIsRegular(table_spec_file_path) )
            {
                const std::shared_ptr<const CDataDict> dictionary = CreateOrGetDictionary(dictionary_file_path);

                CTabSet table_spec(dictionary);

                //Savy (R) sampling app 20090206
                if( SO::EqualsNoCase(table_spec.GetName(), dictionary->GetName()) )
                    table_spec.SetName(table_spec.GetName() + _T("_TAB"));

                if( !table_spec.Save(UTF8_TODO::GetCString(table_spec_file_path), UTF8_TODO::GetCString(dictionary_file_path)) )
                    throw CSProException("Error saving the table specification file: " + table_spec_file_path);
            }

            application.AddTableSpec(table_spec_file_path);
            application.AddDictionaryDescription(DictionaryDescription(dictionary_file_path, std::move(table_spec_file_path), DictionaryType::Input));
        });
}


CodeFile NewFileCreator::CreateOrOpenCodeFile(std::string code_file_path, const CodeType code_type, const Application& application)
{
    if( !PortableFunctions::FileIsRegular(code_file_path) )
    {
        ASSERT(IsLogic(code_type) || IsJavaScript(code_type));

        const std::string default_text = IsLogic(code_type) ? application.GetLogicSettings().GetDefaultFirstLineForTextSource(application.GetLabel(), AppFileType::Code) :
                                                              FormatText("// JavaScript code for application '%s' generated by CSPro\n", application.GetLabel().c_str());

        FileIO::WriteText(code_file_path, default_text, true);
    }

    return CodeFile(code_type, TextSourceEditable::FindOpenOrCreate(std::move(code_file_path)));
}


CodeFile NewFileCreator::CreateOrOpenCodeFile(const Application& application, const bool use_new_file_naming_scheme/* = true*/)
{
    return CreateOrOpenCodeFile(GetDefaultFilePath(AppFileType::Code, application.GetApplicationFilePath(), use_new_file_naming_scheme),
                                CodeType::LogicMain,
                                application);
}


AppMessageFile NewFileCreator::CreateOrOpenMessageFile(std::string message_file_path, const AppMessageFile::Type type, const Application& application)
{
    if( !PortableFunctions::FileIsRegular(message_file_path) )
    {
        ASSERT(type == AppMessageFile::Type::User);

        const std::string default_text = application.GetLogicSettings().GetDefaultFirstLineForTextSource(application.GetLabel(), AppFileType::Message);
        FileIO::WriteText(message_file_path, default_text, true);
    }

    return AppMessageFile(type, TextSourceEditable::FindOpenOrCreate(std::move(message_file_path)));
}


AppMessageFile NewFileCreator::CreateOrOpenMessageFile(const Application& application, const bool use_new_file_naming_scheme/* = true*/)
{
    return CreateOrOpenMessageFile(GetDefaultFilePath(AppFileType::Message, application.GetApplicationFilePath(), use_new_file_naming_scheme),
                                   AppMessageFile::Type::User,
                                   application);
}
