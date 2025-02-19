#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zAppO/Application.h>


enum class EntryApplicationStyle { CAPI, PAPI, OperationalControl };


// NewFileCreator's routines will throw CSProException exceptions on error.

class CLASS_DECL_ZDESIGNERF NewFileCreator
{
public:
    // Creates a new dictionary.
    static std::unique_ptr<CDataDict> CreateDictionary(const std::string& dictionary_file_path);

    // Opens an existing dictionary. If the dictionary is already open (in the Designer),
    // it is returned; otherwise, it is loaded from the disk.
    static std::shared_ptr<const CDataDict> GetDictionary(const std::string& dictionary_file_path);

    // Calls CreateDictionary or GetDictionary based on whether the file exists.
    static std::shared_ptr<const CDataDict> CreateOrGetDictionary(const std::string& dictionary_file_path);

    // Returns the file path of the default working storage dictionary that would be attached to an application.
    static std::string GetDefaultWorkingStorageDictionaryFilePath(const std::string& application_file_path);

    // Creates a working storage dictionary. The file path of the working storage dictionary is returned.
    // The second version optionally adds the dictionary to the application as an external dictionary.
    static std::string CreateWorkingStorageDictionary(const Application& application);
    static std::string CreateWorkingStorageDictionary(Application& application, bool add_to_application_object);


    // Creates a form file. The dictionary will be created as necessary.
    static void CreateFormFile(const std::string& form_file_path,
                               const std::string& dictionary_file_path,
                               bool system_controlled);


    // Opens an existing form file. If the form file is already open (in the Designer),
    // it is returned; otherwise, it is loaded from the disk.
    static std::shared_ptr<const CDEFormFile> GetFormFile(const std::string& form_file_path);


    // Creates an order file. The dictionary will be created as necessary
    static void CreateOrderFile(const std::string& order_file_path,
                                const std::string& dictionary_file_path);


    // Creates an entry application. All input files will be created as necessary.
    static std::unique_ptr<Application> CreateEntryApplication(const std::string& entry_application_file_path,
                                                               const std::string& dictionary_file_path,
                                                               EntryApplicationStyle style = EntryApplicationStyle::CAPI,
                                                               bool use_new_file_naming_scheme = true,
                                                               bool add_working_storage_dictionary = false);

    // Creates a batch application. All input files will be created as necessary.
    static std::unique_ptr<Application> CreateBatchApplication(const std::string& batch_application_file_path,
                                                               const std::string& dictionary_file_path,
                                                               bool use_new_file_naming_scheme = true,
                                                               bool add_working_storage_dictionary = false);

    // Creates a tabulation application. All input files will be created as necessary.
    static std::unique_ptr<Application> CreateTabulationApplication(const std::string& tabulation_application_file_path,
                                                                    const std::string& dictionary_file_path,
                                                                    bool use_new_file_naming_scheme = true);


    // Creates a code file if one does not exist at the path, opening the file using TextSourceEditable.
    // The second version creates the application's main logic file.
    static CodeFile CreateOrOpenCodeFile(std::string code_file_path, CodeType code_type, const Application& application);
    static CodeFile CreateOrOpenCodeFile(const Application& application, bool use_new_file_naming_scheme = true);


    // Creates a message file if one does not exist at the path, opening the file using TextSourceExternal.
    // The second version creates the application's main message file
    static AppMessageFile CreateOrOpenMessageFile(std::string message_file_path, AppMessageFile::Type type, const Application& application);
    static AppMessageFile CreateOrOpenMessageFile(const Application& application, bool use_new_file_naming_scheme = true);


protected:
    // Returns the default file path for an application object based on the application's file path.
    static std::string GetDefaultFilePath(AppFileType app_file_type, const std::string& application_file_path, bool use_new_file_naming_scheme = true);

private:
    template<typename CF>
    static std::unique_ptr<CDataDict> CreateDictionary(const std::string& dictionary_file_path, CF customize_callback);

    template<typename CF>
    static std::unique_ptr<Application> CreateApplication(EngineAppType engine_app_type, const std::string& application_file_path,
                                                          bool use_new_file_naming_scheme, bool add_working_storage_dictionary,
                                                          CF customize_callback);
};
