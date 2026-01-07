# Build External Libraries


### EditorConfig

1. Find the latest version number and commit here: https://github.com/editorconfig/editorconfig-core-c/releases/latest/
2. Edit the batch script, *editorconfig.bat*, setting **ec_version** and **ec_commit**.
3. Run the batch script from a Visual Studio command prompt.
4. This builds only a x64 version of EditorConfig.
5. The built libraries, only used by build tools, are not committed to the repository.


### libgit2

1. Find the latest version here: https://github.com/libgit2/libgit2/releases/latest/
2. Edit the batch script, *libgit2.bat*, setting **lg_version**.
3. Run the batch script from a Visual Studio command prompt.
4. This builds only a x64 version of libgit2.
5. The built libraries, only used by build tools, are not committed to the repository.


### SQLite

1. Run the *Update SQLite* build tool.
2. The SQLite license, which is not really much of a license, is not automatically updated, so check if it should be updated.
3. The library is built as part of the CSPro solution.


### zlib

1. Run the batch script, *zlib.bat*, from a Visual Studio command prompt.
2. This builds both x86 and x64 versions of zlib.
3. The built libraries are committed to the repository.



// --------------------------------------------------------------------------
// curl
// --------------------------------------------------------------------------

    These scripts must be run in a Visual Studio command prompt matching x86/x64.


// --------------------------------------------------------------------------
// libxlsxwriter
// --------------------------------------------------------------------------

    This script is no longer used because it is built as part of the CSPro
    solution but is kept around for reference.
