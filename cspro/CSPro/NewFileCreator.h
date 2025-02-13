#pragma once


enum class EntryApplicationStyle { CAPI, PAPI, OperationalControl };


// the routines will throw CSProException exceptions on error

class NewFileCreator
{
public:
    // creates a dictionary
    static std::unique_ptr<CDataDict> CreateDictionary(NullTerminatedString dictionary_filename);


    // returns the name of the default working storage dictionary that would be attached to an application
    static std::wstring GetDefaultWorkingStorageDictionaryFilename(wstring_view application_filename);


    // creates a working storage dictionary and adds it to the application;
    // the filename of the working storage dictionary is returned
    static std::wstring CreateWorkingStorageDictionary(Application& application);


    // creates a form file; the dictionary will be created as necessary
    static void CreateFormFile(const CString& form_filename, NullTerminatedString dictionary_filename, bool system_controlled);


    // creates an order file; the dictionary will be created as necessary
    static void CreateOrderFile(const CString& order_filename, NullTerminatedString dictionary_filename);


    // creates an entry application; all input files will be created as necessary
    static std::unique_ptr<Application> CreateEntryApplication(const CString& entry_application_filename,
        NullTerminatedString dictionary_filename, EntryApplicationStyle style = EntryApplicationStyle::CAPI,
        bool use_new_file_naming_scheme = true, bool add_working_storage_dictionary = false);


    // creates a batch application; all input files will be created as necessary
    static std::unique_ptr<Application> CreateBatchApplication(const CString& batch_application_filename,
        NullTerminatedString dictionary_filename, bool use_new_file_naming_scheme = true, bool add_working_storage_dictionary = false);


    // creates a tabulation application; all input files will be created as necessary
    static std::unique_ptr<Application> CreateTabulationApplication(const CString& tabulation_application_filename,
        NullTerminatedString dictionary_filename, bool use_new_file_naming_scheme = true);


    // shows a dialog with the possible file types and creates one as appropriate,
    // returning the filename and type of the file if one was successfully created
    static std::optional<std::tuple<CString, AppFileType>> InteractiveMode();

private:
    static CString GetBaseFilename(AppFileType app_file_type, const CString& application_filename, bool use_new_file_naming_scheme);
    static CString GetFilename(AppFileType app_file_type, const CString& application_filename, bool use_new_file_naming_scheme = true);

    template<typename CF>
    static std::unique_ptr<CDataDict> CreateDictionary(NullTerminatedString dictionary_filename, CF customize_callback);

    static std::shared_ptr<const CDataDict> CreateOrGetOpenDictionary(NullTerminatedString dictionary_filename);

    template<typename CF>
    static std::unique_ptr<Application> CreateApplication(EngineAppType engine_app_type, const CString& application_filename,
        bool use_new_file_naming_scheme, bool add_working_storage_dictionary, CF customize_callback);
};
