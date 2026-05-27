# CSPro Build Documentation

This document is a brief summary to help users get up and running buding CSPro or the CSEntry Android application. The document also describes limitations that result in code built from this [public repository](https://github.com/csprousers/cspro) differing from the [releases](https://csprousers.org/downloads) built from the [private repository](https://github.com/CSProDevelopment/cspro).

In addition to this document, [docs/build-environment-setup.md](https://github.com/csprousers/cspro/tree/dev/docs/build-environment-setup.md) describes the environment used by developers to build CSPro and CSEntry.


## External Libraries

The files committed to this repository do not include prebuilt external libraries, and other binary files,
upon which the CSPro build process depends. To successfully build CSPro, or the CSEntry Android application,
you can build these libraries yourself, using scripts in [build-tools/Build External Libraries](https://github.com/csprousers/cspro/tree/dev/build-tools/Build%20External%20Libraries),
or you can use prebuilt libraries that are hosted in the [cspro-libraries](https://github.com/csprousers/cspro-libraries) repository.
The prebuilt libraries used at this point in CSPro development are marked with the tag: **v2026-05-27-2501552**.

To use these libraries, download [v2026-05-27-2501552.zip](https://github.com/csprousers/cspro-libraries/archive/refs/tags/v2026-05-27-2501552.zip)
and decompress it into this repository's directory. This includes files such as the built .dll and .lib
files used on Windows, and the .a static libraries used on Android.


## Windows

The CSPro solution targets a 32-bit or 64-bit application that uses C++17 along with the [Microsoft Foundation Class](https://learn.microsoft.com/en-us/cpp/mfc/framework-mfc) (MFC) framework and the [Win32 API](https://learn.microsoft.com/en-us/windows/win32/api), with a few tools coded in C#. This code is built using [Microsoft Visual Studio 2026](https://visualstudio.microsoft.com). To successfully build CSPro, you must ensure that your Visual Studio installation includes the following components:

- Microsoft.VisualStudio.Component.VC.ATL
- Microsoft.VisualStudio.Component.VC.ATLMFC
- Microsoft.VisualStudio.Component.VC.Redist.14.Latest
- Microsoft.VisualStudio.Component.VC.Tools.x86.x64

To build the solution, open the solution file, [cspro/cspro.sln](https://github.com/csprousers/cspro/blob/dev/cspro/cspro.sln), and select *Build -> Build Solution*. All executables and other built files are output to the directory *cspro/build/x86/Debug/bin* or *cspro/build/x86/Release/bin* (or */x64/* if building 64-bit).

A list of files that are distributed with the CSPro installer is available in [build-tools/Installer Inputs/inputs.json](https://github.com/csprousers/cspro/blob/dev/build-tools/Installer%20Inputs/inputs.json).


## Android

[CSEntry](https://play.google.com/store/apps/details?id=gov.census.cspro.csentry), the Android application, is coded in Java and Kotlin and built using [Android Studio](https://developer.android.com/studio). Because the shared runtime is coded in C++, you will need the [NDK](https://developer.android.com/ndk) toolset.

To build the Android application, open Android Studio, select *File -> New -> Import Project* and choose the [cspro/CSEntryDroid](https://github.com/csprousers/cspro/tree/dev/cspro/CSEntryDroid) directory, and then select *Run -> Run 'app'*. You can debug the application, including the C++ code, by selecting *Run -> Debug 'app'*.

Although the Android user interface is coded in Java and Kotlin, the C++ shared runtime is what runs the CSPro engine and most functionality. The [Java Native Interface](https://en.wikipedia.org/wiki/Java_Native_Interface) (JNI) layer connects the C++ code to the Java/Kotlin code. These compilation units are located in [cspro/CSEntryDroid/app/src/main/jni/src](https://github.com/csprousers/cspro/tree/dev/cspro/CSEntryDroid/app/src/main/jni/src).


## Limitations

The code in the [public repository](https://github.com/csprousers/cspro) differs from the code in the [private repository](https://github.com/CSProDevelopment/cspro) that is used to create the released version of CSPro:

- The released version of CSPro uses the [SQLite Encryption Extension](https://sqlite.org/com/see.html) (SEE) to support encrypting SQLite databases. Because SEE requires a paid license, it cannot be released in the public repository. The SQLite compilation units included in the public repository are from the public SQLite release: [sqlite3.c](https://github.com/csprousers/cspro/blob/dev/cspro/external/SQLite/sqlite3.c) and [sqlite3.h](https://github.com/csprousers/cspro/blob/dev/cspro/external/SQLite/sqlite3.h). Any access to encrypted SQLite databases using code built from the public repository will result in an exception.

- The API keys used to access or use CSWeb, Dropbox, and Google Maps have been removed in the public repository (from [ApiKeys.h](https://github.com/csprousers/cspro/blob/dev/cspro/zToolsO/ApiKeys.h) and [api_keys.xml](https://github.com/csprousers/cspro/blob/dev/cspro/CSEntryDroid/app/src/main/res/values/api_keys.xml)). Those choosing to build CSPro from the public repository will have to provide their own API keys if they want to use these services.


## Project Overview

- [build-tools/Action Invoker Definition Updater](https://github.com/csprousers/cspro/tree/dev/build-tools/Action%20Invoker%20Definition%20Updater) (C++): Parses the [Action Invoker definitions](https://github.com/csprousers/cspro/blob/dev/cspro/zAction/action-definitions.json) and adds these actions to CSPro logic, as well as to the JavaScript script, [cspro/html/action-invoker.js](https://github.com/csprousers/cspro/blob/dev/cspro/html/action-invoker.js).

- [build-tools/Build External Libraries](https://github.com/csprousers/cspro/tree/dev/build-tools/Build%20External%20Libraries): Scripts to build cURL, zlib, etc.

- [build-tools/Licenses](https://github.com/csprousers/cspro/tree/dev/build-tools/Licenses) (C#). Contains the software licenses used by CSPro, as well as a tool to create the [combined license file](https://github.com/csprousers/cspro/blob/dev/build-tools/Licenses/Licenses.html).

- [build-tools/Messages Processor](https://github.com/csprousers/cspro/tree/dev/build-tools/Messages%20Processor) (C++): Tools to process CSPro's [system messages](https://csprousers.org/help/CSPro/message_file_mgf.html).

- [build-tools/Resource ID Numberer](https://github.com/csprousers/cspro/tree/dev/build-tools/Resource%20ID%20Numberer) (C++): A tool that ensures the uniqueness of numbers used in Windows [resource files](https://learn.microsoft.com/en-us/windows/win32/menurc/about-resource-files) (via resource.h files).

- [build-tools/Toolbar Creator](https://github.com/csprousers/cspro/tree/dev/build-tools/Toolbar%20Creator) (C#): Creates Windows toolbars based on the specifications in [build-tools/Image Inputs](https://github.com/csprousers/cspro/tree/dev/build-tools/Image%20Inputs).

- [cspro/CSBatch](https://github.com/csprousers/cspro/tree/dev/cspro/CSBatch) (C++): [Run Batch Program](https://csprousers.org/help/CSBatch), the executor of [batch applications](https://csprousers.org/help/CSPro/batch_edit_applications.html).

- [cspro/CSCode](https://github.com/csprousers/cspro/tree/dev/cspro/CSCode) (C++): [CSCode](https://csprousers.org/help/CSCode/introduction.html), a tool to edit and run code files.

- [cspro/CSConcat](https://github.com/csprousers/cspro/tree/dev/cspro/CSConcat) (C++): [Concatenate Data](https://csprousers.org/help/CSConcat/introduction_to_concatenate_data.html), a tool to combine multiple CSPro data files into a single data file.

- [cspro/CSDeploy](https://github.com/csprousers/cspro/tree/dev/cspro/CSDeploy) (C#): [Deploy Application](https://csprousers.org/help/CSDeploy/introduction_to_deployment.html), a tool to deploy applications to a server.

- [cspro/CSDiff](https://github.com/csprousers/cspro/tree/dev/cspro/CSDiff) (C++): [Compare Data](https://csprousers.org/help/CSDiff/introduction_to_compare_data.html), a tool to compare two CSPro data files.

- [cspro/CSDocument](https://github.com/csprousers/cspro/tree/dev/cspro/CSDocument) (C++): [CSDocument](https://csprousers.org/help/CSDocument/introduction.html), a tool to author documents using a simple, limited, markup language.

- [cspro/CSEntry](https://github.com/csprousers/cspro/tree/dev/cspro/CSEntry) (C++): [CSEntry](https://csprousers.org/help/CSEntry), the executor of [data entry applications](https://csprousers.org/help/CSPro/data_entry_applications.html).

- [cspro/CSExport](https://github.com/csprousers/cspro/tree/dev/cspro/CSExport) (C++): [Export Data](https://csprousers.org/help/CSExport/introduction_to_export_data.html), a tool to export CSPro data files to other formats.

- [cspro/CSFreq](https://github.com/csprousers/cspro/tree/dev/cspro/CSFreq) (C++): [Tabulate Frequencies](https://csprousers.org/help/CSFreq/introduction_to_tabulate_frequencies.html), a tool to produce frequency distributions from CSPro data files.

- [cspro/CSIndex](https://github.com/csprousers/cspro/tree/dev/cspro/CSIndex) (C++): [Index Data](https://csprousers.org/help/CSIndex/introduction_to_csindex.html), a tool to create CSPro data file indices and to identify duplicate cases.

- [cspro/CSPack](https://github.com/csprousers/cspro/tree/dev/cspro/CSPack) (C++): [Pack Application](https://csprousers.org/help/CSPack/introduction_to_pack_application.html), a tool to pack CSPro application files into a single ZIP file.

- [cspro/CSPro](https://github.com/csprousers/cspro/tree/dev/cspro/CSPro) (C++): [CSPro Designer](https://csprousers.org/help/CSPro), the application used to design CSPro applications.

- [cspro/CSReFmt](https://github.com/csprousers/cspro/tree/dev/cspro/CSReFmt) (C++): [Reformat Data](https://csprousers.org/help/CSReFmt/introduction_to_reformat_data.html), a tool to reformat CSPro data files based on changes to a dictionary.

- [cspro/CSSort](https://github.com/csprousers/cspro/tree/dev/cspro/CSSort) (C++): [Sort Data](https://csprousers.org/help/CSSort/introduction_to_sort_data.html), a tool to sort CSPro data files.

- [cspro/CSTab](https://github.com/csprousers/cspro/tree/dev/cspro/CSTab) (C++): [Run Tabulation Program](https://csprousers.org/help/CSTab), the executor of [tabulation applications](https://csprousers.org/help/CSPro/tabulation_applications.html).

- [cspro/CSView](https://github.com/csprousers/cspro/tree/dev/cspro/CSView) (C++): [CSView](https://csprousers.org/help/CSView/introduction.html), a program that displays files in a web browser that has access to the [Action Invoker](https://csprousers.org/help/CSPro/action_invoker.html).

- [cspro/DataViewer](https://github.com/csprousers/cspro/tree/dev/cspro/DataViewer) (C#): [Data Viewer](https://csprousers.org/help/DataViewer/introduction_to_data_viewer.html), a tool to view CSPro data files.

- [cspro/Excel2CSPro](https://github.com/csprousers/cspro/tree/dev/cspro/Excel2CSPro) (C#): [Excel to CSPro](https://csprousers.org/help/Excel2CSPro/introduction_to_excel_to_cspro.html), a tool to convert data from Excel workbooks to CSPro data files.

- [cspro/ParadataConcat](https://github.com/csprousers/cspro/tree/dev/cspro/ParadataConcat) (C++): [Paradata Concatenator](https://csprousers.org/help/ParadataConcat/introduction_to_paradata_concatenator.html), a tool to combine multiple [paradata logs](https://csprousers.org/help/CSPro/paradata_log_file_cslog.html) into a single log.

- [cspro/ParadataViewer](https://github.com/csprousers/cspro/tree/dev/cspro/ParadataViewer) (C#): [Paradata Viewer](https://csprousers.org/help/ParadataViewer/introduction_to_paradata_viewer.html), a tool to view [paradata](https://csprousers.org/help/CSPro/paradata.html) and display reports.

- [cspro/PFF Editor](https://github.com/csprousers/cspro/tree/dev/cspro/PFF%20Editor) (C#), [PFF Editor](https://csprousers.org/help/CSPro/tool_list.html), a tool to view and edit [PFFs](https://csprousers.org/help/CSPro/program_information_file_pff.html)

- [cspro/RunPff](https://github.com/csprousers/cspro/tree/dev/cspro/RunPff) (C++): [RunPFF](https://csprousers.org/help/CSPro/tool_list.html), a tool that reads a [PFF](https://csprousers.org/help/CSPro/program_information_file_pff.html) and launches the program that can handle the file.

- [cspro/Save Array Viewer](https://github.com/csprousers/cspro/tree/dev/cspro/Save%20Array%20Viewer) (C#): [Save Array Viewer](https://csprousers.org/help/CSPro/tool_list.html), a tool to view and modify [saved arrays](https://csprousers.org/help/CSPro/saved_arrays_file_sva.html).

- [cspro/TblView](https://github.com/csprousers/cspro/tree/dev/cspro/TblView) (C++): [Table Viewer](https://csprousers.org/help/TblView/introduction_to_table_viewer.html), a tool to view [CSPro tables](https://csprousers.org/help/CSPro/tables_file_tbw.html).

- [cspro/TextConverter](https://github.com/csprousers/cspro/tree/dev/cspro/TextConverter) (C++): [Text Encoding Converter](https://csprousers.org/help/CSPro/tool_list.html), a tool to modify the encoding of text files, generally to convert from ANSI to UTF-8 formats.

- [cspro/TextView](https://github.com/csprousers/cspro/tree/dev/cspro/TextView) (C++): [Text Viewer](https://csprousers.org/help/TextView/introduction_to_text_viewer.html), a tool to view the contents of a text file.

- [cspro/UriHandler](https://github.com/csprousers/cspro/tree/dev/cspro/UriHandler) (C++): [URI Handler](https://csprousers.org/help/CSPro/tool_list.html), a tool to handle URIs with the cspro:// scheme.

- [tools/CSPro Production Runner](https://github.com/csprousers/cspro/tree/dev/tools/CSPro%20Production%20Runner) (C++ WinForms): [Production Runner](https://csprousers.org/help/CSPro/tool_list.html), a tool to set up a series of CSPro processing tasks and then run them all at once.

- [tools/Operator Statistics Viewer](https://github.com/csprousers/cspro/tree/dev/tools/Operator%20Statistics%20Viewer) (C#): [Operator Statistics Viewer](https://csprousers.org/help/CSPro/tool_list.html), a tool to view [operator statistics files](https://csprousers.org/help/CSPro/operator_statistics_file_log.html).


## Files Overview

- **DLLs**: In addition to the projects listed above, which build to [executables](https://en.wikipedia.org/wiki/Executable), the CSPro solution contains many projects that build to [dynamic-link libraries](https://en.wikipedia.org/wiki/Dynamic-link_library) (DLLs). DLL projects nearly all start with the letter "z" and occasionally end with a suffix: "O" for "object" or "F" for "frame". For example, [zDictO](https://github.com/csprousers/cspro/tree/dev/cspro/zDictO) contains the dictionary objects, while [zDictF](https://github.com/csprousers/cspro/tree/dev/cspro/zDictF) contains the MFC UI code to edit dictionaries. Compilation units in DLLs with the suffix "CLR" are used by C# code to interface with C++ code. Notable DLLs include:
    - [zAction](https://github.com/csprousers/cspro/tree/dev/cspro/zAction): The [Action Invoker](https://csprousers.org/help/CSPro/action_invoker.html) runtime.
    - [zCaseO](https://github.com/csprousers/cspro/tree/dev/cspro/zCaseO): Routines to interact with CSPro cases.
    - [zDataO](https://github.com/csprousers/cspro/tree/dev/cspro/zDataO): The implementations of CSPro's [data sources](https://csprousers.org/help/CSPro/data_sources.html).
    - [zDictO](https://github.com/csprousers/cspro/tree/dev/cspro/zDictO): Routines to interact with CSPro [dictionaries](https://csprousers.org/help/CSPro/data_dictionary.html).
    - [zEngineO](https://github.com/csprousers/cspro/tree/dev/cspro/zEngineO) and [zLogicO](https://github.com/csprousers/cspro/tree/dev/cspro/zLogicO): The CSPro language compiler and interpreter.
    - [zSyncO](https://github.com/csprousers/cspro/tree/dev/cspro/zSyncO): Synchronization routines for CSWeb, Dropbox, etc..
    - [zToolsO](https://github.com/csprousers/cspro/tree/dev/cspro/zToolsO) and [zUtilO](https://github.com/csprousers/cspro/tree/dev/cspro/zUtilO): Base DLLs with file I/O, string manipulation, and other routines.

- **System message files**: The [system messages](https://csprousers.org/help/CSPro/message_file_mgf.html) are located in the [cspro](https://github.com/csprousers/cspro/tree/dev/cspro) directory. Messages only used at compile-time are in the file [CSProDesigner.mgf](https://github.com/csprousers/cspro/blob/dev/cspro/CSProDesigner.mgf), and runtime messages are in the files *CSProRuntime.\<language-code\>.mgf*. For example, English runtime messages are in [CSProRuntime.en.mgf](https://github.com/csprousers/cspro/blob/dev/cspro/CSProRuntime.en.mgf).

- **HTML directory**: The files in [cspro/html](https://github.com/csprousers/cspro/tree/dev/cspro/html) become the root directory of [CSPro's local web server](https://csprousers.org/help/CSPro/html_in_cspro.html). The subdirectory [cspro/html/dialogs](https://github.com/csprousers/cspro/tree/dev/cspro/html/dialogs) contain CSPro's [HTML dialogs](https://csprousers.org/help/CSPro/html_dialog_ui.html).

- **Logic functions**: The file [zLogicO/FunctionTable.cpp](https://github.com/csprousers/cspro/blob/dev/cspro/zLogicO/FunctionTable.cpp) contains information about each CSPro logic function, including what routine is used to compile the function.

- **Action Invoker definitions**: The file [cspro/zAction/action-definitions.json](https://github.com/csprousers/cspro/blob/dev/cspro/zAction/action-definitions.json) details the actions runnable using the [Action Invoker](https://csprousers.org/help/CSPro/action_invoker.html).


## Programming Principles

CSPro development started in 1998 and initially included code from its two DOS-based predecessors, IMPS and ISSA. A small number of developers have worked on the project while it has [grown over the years](https://csprousers.org/help/CSPro/release_history.html). Developers have different programming styles, and this is reflected in the varying coding styles found in CSPro. For new development, the development team attempts to follow a few principles:

1. Follow the coding style of the code you're modifying.
2. Write code for humans, prioritizing readability over cleverness.
3. Use meaningful names for variables, functions, and classes.
4. Avoid magic numbers, using named constants instead.
5. Write self-documenting code. Clear code does not require excessive comments, though comments are useful to explain why something is done rather than what is done.

Much of CSPro's code was written based on best practices from the 1990s, including using [Hungarian notation](https://en.wikipedia.org/wiki/Hungarian_notation) and using MFC classes rather than the [C++ Standard Library](https://en.wikipedia.org/wiki/C%2B%2B_Standard_Library). In addition, strings were originally stored using [CString](https://learn.microsoft.com/en-us/cpp/atl-mfc-shared/reference/cstringt-class) and then later [std::wstring](https://en.cppreference.com/w/cpp/string/basic_string). When writing new code, or refactoring existing code, the CSPro developers:

1. Use C++ Standard Library classes.
2. Use std::string with UTF-8 encoding, following the [UTF-8 Everywhere](https://utf8everywhere.org) philosophy. The [TC (Text Converter)](https://github.com/CSProDevelopment/cspro/blob/dev/cspro/zToolsO/Utf8.h) namespace contains routines to convert UTF-8 strings to wide character strings.
3. Use C++ exceptions, derived from [CSProException](https://github.com/csprousers/cspro/blob/dev/cspro/zToolsO/CSProException.h), rather than returning error codes.
4. Use smart pointers: [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) and [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr).
5. Use [move semantics](https://en.cppreference.com/w/cpp/utility/move).
6. Aim for [const correctness](https://isocpp.org/wiki/faq/const-correctness).
7. Prefer [constexpr](https://en.cppreference.com/w/cpp/language/constexpr) over [const](https://en.cppreference.com/w/cpp/language/constant_expression).
8. Use C++ casts ([static_cast](https://en.cppreference.com/w/cpp/language/static_cast) and [reinterpret_cast](https://en.cppreference.com/w/cpp/language/reinterpret_cast)) rather than C casts. Additionally, use [assert_cast](https://github.com/csprousers/cspro/blob/dev/cspro/zToolsO/assert_cast.h) when performing a dynamic cast that is guaranteed to success.
9. Minimize using [auto](https://en.cppreference.com/w/cpp/language/auto) instead of writing an explicit type.
10. Remove uses of Hungarian notation.
