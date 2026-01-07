# Build External Libraries


### EditorConfig

1. Find the latest version number and commit here: https://github.com/editorconfig/editorconfig-core-c/releases/latest/
2. Edit the batch script, *editorconfig.bat*, setting **ec_version** and **ec_commit**.
3. Run the batch script from a Visual Studio command prompt.
4. This builds only a x64 version of EditorConfig.
5. The built libraries, only used by build tools, are not committed to the repository.


### JSMin

1. Run the batch script, *jsmin.bat*.
2. This copies files into the CSPro solution.
3. There are CSPro additions that will have to restored in:
    * jsmin.cpp
4. The JSMin license is at the top of *jsmin.cpp*, so check if it should be updated.
5. The library is built as part of the CSPro solution.


### libgit2

1. Find the latest version here: https://github.com/libgit2/libgit2/releases/latest/
2. Edit the batch script, *libgit2.bat*, setting **lg_version**.
3. Run the batch script from a Visual Studio command prompt.
4. This builds only a x64 version of libgit2.
5. The built libraries, only used by build tools, are not committed to the repository.


### libxlsxwriter

1. Run the batch script, *libxlsxwriter.bat*.
2. This copies files into the CSPro solution, including some that are not necessary.
3. Remove anything that is not already committed.
4. The library is built as part of the CSPro solution.


### libexif

1. Find the latest version here: https://github.com/libexif/libexif/releases/latest/
2. Edit the batch script, *libexif.bat*, setting **lx_version**.
3. Run the batch script. 
4. This copies files into the CSPro solution, including some that are not necessary. 
5. Remove anything that is not already committed.
6. There are modifications made to many files that will have to be restored so that the library builds in the CSPro environment. These modification relate to header inclusion and bindtextdomain.
7. The library is built as part of the CSPro solution.


### md4c

1. Find the latest tag here: https://github.com/mity/md4c/tags
2. Edit the batch script, *md4c.bat*, setting **md_tag**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary. 
5. Remove anything that is not already committed.
6. There are CSPro additions that will have to restored in:
    * entity.h
7. The library is built as part of the CSPro solution.


### pugixml

1. Find the latest version here: https://github.com/zeux/pugixml/releases/latest/
2. Edit the batch script, *pugixml.bat*, setting **px_version**.
3. Run the batch script. 
4. This copies files into the CSPro solution.
5. The library is built as part of the CSPro solution.


### RapidFuzz

1. Find the latest version here: https://github.com/rapidfuzz/rapidfuzz-cpp/releases/latest/
2. Edit the batch script, *rapidfuzz.bat*, setting **rz_version**.
3. Run the batch script. 
4. This copies files into the CSPro solution, including some that are not necessary. 
5. Remove anything that is not already committed.
6. The library is built as part of the CSPro solution.


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
