# CSPro's External Libraries

This document details information about the external libraries used by CSPro.

Some libraries are created using build scripts located in the [cspro-libraries-third-party](https://github.com/csprousers/cspro-libraries-third-party) repository.
The built libraries are available in the [cspro-libraries](https://github.com/csprousers/cspro-libraries) repository.

Batch scripts to update many of the libraries are located here:

- *build-tools/Build External Libraries*


## Libraries in the Third-Party Libraries Repository


### bzip2

*Current version: 1.0.8*

1. CSPro-specific code is located in a cloned repository: https://github.com/csprousers/cspro-libraries-fork-bzip2
2. Incorporate any new code, if available, into the *cspro* branch: https://www.sourceware.org/bzip2/downloads.html
3. Run the build script.
4. This copies files into CSPro's *third_party/prebuilt* directory.
5. The built libraries are also available in the *cspro-libraries* repository.


### CHMLib

*Current version: 0.3 (this library no longer appears to be updated)*

1. CSPro-specific code is located in a forked repository: https://github.com/csprousers/cspro-libraries-fork-CHMLib
2. Confirm that there are no longer updates: https://github.com/jedwing/CHMLib
3. Run the build script.
4. This copies files into CSPro's *third_party/prebuilt* directory.
5. The built libraries are also available in the *cspro-libraries* repository.


### curl

*Current version: 8.22.0*

1. CSPro-specific code is located in a forked repository: https://github.com/csprousers/cspro-libraries-fork-curl
2. Incorporate any new code, if available, into the *cspro* branch: https://github.com/curl/curl/releases/latest
3. Run the build script.
4. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
5. Remove anything that is not already committed.
6. The built libraries are also available in the *cspro-libraries* repository.


### EditorConfig

*Current version: 0.12.11*

1. CSPro-specific code is located in a forked repository: https://github.com/csprousers/cspro-libraries-fork-editorconfig-core-c
2. Incorporate any new code, if available, into the *cspro* branch: https://github.com/editorconfig/editorconfig-core-c/releases/latest
3. Run the build script.
4. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
5. Remove anything that is not already committed.
6. The built libraries are also available in the *cspro-libraries* repository.


### GPAC

*Current version: 2.4.0*

1. CSPro-specific code is located in a forked repository: https://github.com/csprousers/cspro-libraries-fork-gpac
2. Incorporate any new code, if available, into the *cspro* branch: https://github.com/gpac/gpac/releases/latest
3. Run the build script.
4. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
5. Remove anything that is not already committed.
6. The built libraries are also available in the *cspro-libraries* repository.


### gumbo-parser

*Current version: 0.10.1 (this library is archived and no longer updated)*

1. CSPro-specific code is located in a forked repository: https://github.com/csprousers/cspro-libraries-fork-gumbo-parser
2. Confirm that there are no longer updates: https://github.com/google/gumbo-parser
3. Run the build script.
4. This copies files into CSPro's *third_party/prebuilt* directory.
5. The built libraries are also available in the *cspro-libraries* repository.


### libexif

*Current version: 0.6.26*

1. CSPro-specific code is located in a forked repository: https://github.com/csprousers/cspro-libraries-fork-libexif
2. Merge the latest release into the *cspro* branch: https://github.com/libexif/libexif/releases/latest
3. Modify the version numbers in *config.h*.
4. Run the build script.
5. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
6. Remove anything that is not already committed.
7. The built libraries are also available in the *cspro-libraries* repository.


### libgit2

*Current version: 1.9.7*

1. Set the Git submodule to the latest release: https://github.com/libgit2/libgit2/releases/latest
2. Run the build script.
3. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
4. Remove anything that is not already committed.
5. The built libraries are also available in the *cspro-libraries* repository.


### libwebm

*Current version: 1.0.0.32*

1. Set the Git submodule to the latest release: https://github.com/webmproject/libwebm/tags
2. Run the build script.
3. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
4. Remove anything that is not already committed.
5. The built libraries are also available in the *cspro-libraries* repository.


### libwebp

*Current version: 1.6.0*

1. Set the Git submodule to the latest release: https://github.com/webmproject/libwebp/tags
2. Run the build script.
3. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
4. Remove anything that is not already committed.
5. The built libraries are also available in the *cspro-libraries* repository.


### libxlsxwriter

*Current version: 1.2.4*

1. CSPro-specific code is located in a forked repository: https://github.com/csprousers/cspro-libraries-fork-libxlsxwriter
2. Merge the latest release into the *cspro* branch: https://github.com/jmcnamara/libxlsxwriter/releases/latest
3. Run the build script.
4. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
5. Remove anything that is not already committed.
6. The built libraries are also available in the *cspro-libraries* repository.


### md4c

*Current version: 0.5.3*

1. CSPro-specific code is located in a forked repository: https://github.com/csprousers/cspro-libraries-fork-md4c
2. Merge the latest release into the *cspro* branch: https://github.com/mity/md4c/tags
3. Run the build script.
4. This copies files into CSPro's *third_party/prebuilt* directory.
5. The built libraries are also available in the *cspro-libraries* repository.


### miniz

*Current version: 3.1.2*

1. Set the Git submodule to the latest release: https://github.com/richgel999/miniz/releases/latest
2. Run the build script.
3. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
4. Remove anything that is not already committed.
5. The built libraries are also available in the *cspro-libraries* repository.


### pugixml

*Current version: 1.16*

1. Set the Git submodule to the latest release: https://github.com/zeux/pugixml/releases/latest
2. Run the build script.
3. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
4. Remove anything that is not already committed.
5. The built libraries are also available in the *cspro-libraries* repository.


### yaml-cpp

*Current version: 0.9.0*

1. Set the Git submodule to the latest release: https://github.com/jbeder/yaml-cpp/releases/latest
2. Run the build script.
3. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
4. Remove anything that is not already committed.
5. The built libraries are also available in the *cspro-libraries* repository.


### zlib

*Current version: *1.3.2*

1. Set the Git submodule to the latest release: https://github.com/madler/zlib/releases/latest
2. Run the build script.
3. This copies files into CSPro's *third_party/prebuilt* directory, including some that are not necessary.
4. Remove anything that is not already committed.
5. Different build configurations may result in different versions of *zconf.h*. Commit the Windows version.
6. The built libraries are also available in the *cspro-libraries* repository.



## Libraries With Build Scripts


### Bootstrap + Bootstrap Icons

1. Find the latest versions here:
    * https://github.com/twbs/bootstrap/releases/latest
    * https://github.com/twbs/icons/releases/latest
2. Edit the batch script, *bootstrap.bat*, setting **bs_version** and **bs_icons_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### Chart.js

1. Find the latest version here: https://github.com/chartjs/Chart.js/releases/latest
2. Edit the batch script, *chart-js.bat*, setting **cj_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.




### CodeMirror

*(This is a legacy version of the library, replaced by CodeMirror 6.)*

1. Find the latest version here: https://github.com/codemirror/codemirror5/releases/latest
2. Edit the shell script, *codemirror.sh*, setting **CODE_MIRROR_VERSION**.
3. Run the shell script (e.g., from the Git terminal).
4. This copies files into the CSPro's *html* directory.


### cpp-httplib

*(The 32-bit version of this library is no longer updated.)*

1. Find the latest version here: https://github.com/yhirose/cpp-httplib/releases/latest
2. Edit the batch script, *cpp-httplib.bat*, setting **httplib_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. There are CSPro modifications that have to be restored in:
    * httplib.h
6. The library is built as part of the CSPro solution.


### Easylogging++

*(This library is archived and no longer updated.)*

1. Find the latest version here: https://github.com/abumq/easyloggingpp/releases/latest
2. Edit the batch script, *easylogging.bat*, setting **el_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
3. There are CSPro modifications that have to be restored in:
    * easylogging++.cc
    * easylogging++.h
5. The library is built as part of the CSPro solution.


### FakeIt

1. Find the latest version here: https://github.com/eranpeer/FakeIt/releases/latest
2. Edit the batch script, *fakeit.bat*, setting **fakeit_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. The library is built as part of the CSPro solution's tests.


### geometry.hpp

1. Find the latest version here: https://github.com/mapbox/geometry.hpp/releases/latest
2. Edit the batch script, *geometry-hpp.bat*, setting **geohpp_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. The library is built as part of the CSPro solution.


### github-markdown-css

1. Find the latest version here: https://github.com/sindresorhus/github-markdown-css/releases/latest
2. Edit the batch script, *github-markdown-css.bat*, setting **gmc_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### Handlebars.js.txt

1. Find the latest version here: https://github.com/handlebars-lang/handlebars.js/releases/latest
2. Edit the batch script, *handlebars.bat*, setting **hb_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### jQuery + jQuery UI

1. Find the latest versions here:
    * https://github.com/jquery/jquery/releases/latest
    * https://github.com/jquery/jquery-ui/releases/latest
2. Edit the batch script, *jquery.bat*, setting **jq_version** and **jq_ui_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### JsBarcode

1. Find the latest version here: https://github.com/lindell/JsBarcode/releases/latest
2. Edit the batch script, *js-barcode.bat*, setting **jsb_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### JSMin

*(This library has not been updated in years.)*

1. Run the batch script, *jsmin.bat*.
2. This copies files into the CSPro solution.
3. There are CSPro modifications that have to be restored in:
    * jsmin.cpp
4. The JSMin license is at the top of *jsmin.cpp*, so check if it should be updated.
5. The library is built as part of the CSPro solution.


### jsoncons

1. Find the latest version here: https://github.com/danielaparker/jsoncons/releases/latest
2. Edit the batch script, *jsoncons.bat*, setting **jsoncons_version**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. There are CSPro modifications that have to be restored in:
    * basic_json.hpp
    * json_encoder.hpp
    * json_exception.hpp
    * json_options.hpp
    * sink.hpp
    * config/compiler_support.hpp
7. The library is built as part of the CSPro solution.


### Leaflet

1. Find the latest version here: https://github.com/Leaflet/Leaflet/releases/latest
2. Edit the batch script, *leaflet.bat*, setting **leaflet_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory, including some that are not necessary.
5. Remove anything that is not already committed.
6. Run the batch script *Update Android HTML Assets*.


### leaflet-ajax

*(This library has not been updated in years.)*

1. Find the latest tag here: https://github.com/calvinmetcalf/leaflet-ajax/tags
2. Edit the batch script, *leaflet-ajax.bat*, setting **leaflet_ajax_tag**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### Lexilla

1. Find the latest tag here: https://github.com/ScintillaOrg/lexilla/tags
2. Edit the batch script, *lexilla.bat*, setting **lexilla_tag**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. There are CSPro modifications made to many files that have to be restored.
7. The library is built as part of the CSPro solution.
8. This library should be updated at the same time as Scintilla and ScintillaCtrl / ScintillaView.


### librdata

1. Run the batch script, *librdata.bat*.
2. This copies files into the CSPro solution.
3. There are CSPro modifications that have to be restored in:
    * rdata.h
    * rdata_io_unistd.c
    * rdata_write.c
4. The library is built as part of the CSPro solution.
5. This library should be updated at the same time as ReadStat.


### mustache.js

1. Find the latest version here: https://github.com/janl/mustache.js/releases/latest
2. Edit the batch script, *mustache.bat*, setting **ms_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### QR-Code-generator

1. Find the latest version here: https://github.com/nayuki/QR-Code-generator/releases
2. Edit the batch script, *qrcodegen.bat*, setting **qrcg_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. The license is at the bottom of *README.md* and is not automatically updated, so check if it should be updated.
6. The library is built as part of the CSPro solution.


### QuickJS-NG

1. Find the latest version here: https://github.com/quickjs-ng/quickjs/releases/latest
2. Edit the batch script, *quickjs.bat*, setting **quickjs_version**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. There are CSPro modifications made to many files that have to be restored.
7. The library is built as part of the CSPro solution.


### RapidFuzz

1. Find the latest version here: https://github.com/rapidfuzz/rapidfuzz-cpp/releases/latest
2. Edit the batch script, *rapidfuzz.bat*, setting **rz_version**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. The library is built as part of the CSPro solution.


### ReadStat

1. Find the latest version here: https://github.com/WizardMac/ReadStat/releases/latest
2. Edit the batch script, *readstat.bat*, setting **rs_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. There are CSPro modifications that have to be restored in:
    * ..\librdata+ReadStat\readstat_bits.c
    * readstat.h
    * readstat_iconv.h
    * readstat_writer.c
    * readstat_writer.h
    * spss\readstat_sav_write.c
    * spss\readstat_spss.c
    * spss\readstat_spss.h
    * stata\readstat_dta.c
    * stata\readstat_dta_write.c
6. The library is built as part of the CSPro solution.
7. This library should be updated at the same time as librdata.


### RxCpp

1. Find the latest version here: https://github.com/ReactiveX/RxCpp/releases/latest
2. Edit the batch script, *rxcpp.bat*, setting **rx_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. The library is built as part of the CSPro solution.


### Scintilla

1. Find the latest version number here: https://www.scintilla.org/ScintillaDownload.html
2. Edit the batch script, *scintilla.bat*, setting **scintilla_version**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. There are CSPro modifications that have to be restored in:
    * win32\ScintillaWin.cxx
7. The library is built as part of the CSPro solution.
8. This library should be updated at the same time as Lexilla and ScintillaCtrl / ScintillaView.


### ScintillaCtrl / ScintillaView

1. Run the batch script, *scintilla-ctrl-view.bat*.
2. This copies files into the CSPro solution.
3. There are CSPro modifications that have to be restored.
4. The license is at the top of *ScintillaCtrl.h*, so check if it should be updated.
5. The library is built as part of the CSPro solution.
6. This library should be updated at the same time as Lexilla and Scintilla.


### scrypt

1. Find the latest tag here: https://github.com/Tarsnap/scrypt/tags
2. Edit the batch script, *scrypt.bat*, setting **sc_tag**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. There are CSPro modifications that have to be restored in:
    * sha256.c (regarding "static restrict" and unneeded header files)
6. The library is built as part of the CSPro solution.


### sprintf-js

1. Find the latest tag here: https://github.com/alexei/sprintf.js/tags
2. Edit the batch script, *sprintf-js.bat*, setting **spjs_tag**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### SQLite

1. Run the *Update SQLite* build tool.
2. The SQLite license, which is not really much of a license, is not automatically updated, so check if it should be updated.
3. The library is built as part of the CSPro solution.


### stb

1. Run the batch script, *stb.bat*.
2. This copies files into the CSPro solution.
3. There are CSPro modifications that have to be restored in:
    * stb_image.h
4. The library is built as part of the CSPro solution.


### stduuid

1. Find the latest version here: https://github.com/mariusbancila/stduuid/releases/latest
2. Edit the batch script, *stduuid.bat*, setting **stduuid_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. There are CSPro modifications that have to be restored in:
    * uuid.h
6. The library is built as part of the CSPro solution.


### Summernote + summernote-rtl-plugin

*(Summernote is still maintained, but the summernote-rtl-plugin library has not been updated in years.)*

1. Find the latest version here: https://github.com/summernote/summernote/releases/latest
2. Edit the batch script, *summernote.bat*, setting **sn_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory, including some that are not necessary.
5. Remove anything that is not already committed.



## Libraries Without Build Scripts


### cpp-base64

There is a more recent version of this library, but it differs significantly (e.g., throwing exceptions). Additionally, the CSPro version supports encoding and decoding std::vector\<std\:\:byte\>. These changes could be applied to the newer version of the library at some point.


### rtf2html

There is a more recent version of this library, but many changes were made to the CSPro version, and RTF is no longer used except when upgrading old question text files, so updating the library is not particularly important.
