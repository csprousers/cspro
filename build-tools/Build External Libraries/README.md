# Build External Libraries


### Bootstrap + Bootstrap Icons

1. Find the latest versions here:
    * https://github.com/twbs/bootstrap/releases/latest/
    * https://github.com/twbs/icons/releases/latest/
2. Edit the batch script, *bootstrap.bat*, setting **bs_version** and **bs_icons_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### bzip2

1. Find the latest version here: https://www.sourceware.org/bzip2/downloads.html
2. Edit the batch script, *bzip2.bat*, setting **bzip2_version**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. There are CSPro modifications that have to be restored in:
    * bzlib_private.h
7. The library is built as part of the CSPro solution.


### Chart.js

1. Find the latest version here: https://github.com/chartjs/Chart.js/releases/latest/
2. Edit the batch script, *chart-js.bat*, setting **cj_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### CHMLib

*(This library has not been updated in years.)*

1. Run the batch script, *chmlib.bat*.
2. This copies files into the CSPro solution, including some that are not necessary.
3. Remove anything that is not already committed.
4. There are CSPro modifications that have to be restored in:
    * chm_lib.c
    * chm_lib.h
5. The library is built as part of the CSPro solution.


### curl

1. Build **zlib** prior to building curl.
2. Find the latest version here: https://github.com/curl/curl/releases/latest/
3. Edit the batch script, *curl.bat*, setting **curl_version**.
4. Run the batch script from a Visual Studio command prompt.
5. This copies files into the CSPro solution, including some that are not necessary.
6. Remove anything that is not already committed.
7. This builds both x86 and x64 versions of curl.
8. The built libraries are committed to the repository.


### Easylogging++

*(This library is archived and no longer updated.)*

1. Find the latest version here: https://github.com/abumq/easyloggingpp/releases/latest/
2. Edit the batch script, *easylogging.bat*, setting **el_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
3. There are CSPro modifications that have to be restored in:
    * easylogging++.cc
    * easylogging++.h
5. The library is built as part of the CSPro solution.


### EditorConfig

1. Find the latest version and commit here: https://github.com/editorconfig/editorconfig-core-c/releases/latest/
2. Edit the batch script, *editorconfig.bat*, setting **ec_version** and **ec_commit**.
3. Run the batch script from a Visual Studio command prompt.
4. This builds only a x64 version of EditorConfig.
5. The built libraries, only used by build tools, are not committed to the repository.


### geometry.hpp

1. Find the latest version here: https://github.com/mapbox/geometry.hpp/releases/latest/
2. Edit the batch script, *geometry-hpp.bat*, setting **geohpp_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. The library is built as part of the CSPro solution.


### github-markdown-css

1. Find the latest version here: https://github.com/sindresorhus/github-markdown-css/releases/latest/
2. Edit the batch script, *github-markdown-css.bat*, setting **gmc_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### gumbo-parser

*(This library is archived and no longer updated.)*

1. Find the latest version here: https://github.com/google/gumbo-parser/releases/latest/
2. Edit the batch script, *gumbo-parser.bat*, setting **gp_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. The library is built as part of the CSPro solution.


### Handlebars.js.txt

1. Find the latest version here: https://github.com/handlebars-lang/handlebars.js/releases/latest/
2. Edit the batch script, *handlebars.bat*, setting **hb_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### jQuery + jQuery UI

1. Find the latest versions here:
    * https://github.com/jquery/jquery/releases/latest/
    * https://github.com/jquery/jquery-ui/releases/latest/
2. Edit the batch script, *jquery.bat*, setting **jq_version** and **jq_ui_version**.
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


### Leaflet

1. Find the latest version here: https://github.com/Leaflet/Leaflet/releases/latest/
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


### libgit2

1. Find the latest version here: https://github.com/libgit2/libgit2/releases/latest/
2. Edit the batch script, *libgit2.bat*, setting **lg_version**.
3. Run the batch script from a Visual Studio command prompt.
4. This builds only a x64 version of libgit2.
5. The built libraries, only used by build tools, are not committed to the repository.


### librdata

1. Run the batch script, *librdata.bat*.
2. This copies files into the CSPro solution.
3. There are CSPro modifications that have to be restored in:
    * rdata.h
    * rdata_io_unistd.c
    * rdata_write.c
4. The library is built as part of the CSPro solution.
5. This library should be updated at the same time as ReadStat.


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
6. There are CSPro modifications made to many files that have to be restored so that the library builds in the CSPro environment. These modification relate to header inclusion and bindtextdomain.
7. The library is built as part of the CSPro solution.


### md4c

1. Find the latest tag here: https://github.com/mity/md4c/tags
2. Edit the batch script, *md4c.bat*, setting **md_tag**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. There are CSPro modifications that have to be restored in:
    * entity.h
7. The library is built as part of the CSPro solution.


### mustache.js

1. Find the latest version here: https://github.com/janl/mustache.js/releases/latest/
2. Edit the batch script, *mustache.bat*, setting **ms_version**.
3. Run the batch script.
4. This copies files into the CSPro's *html* directory.
5. Run the batch script *Update Android HTML Assets*.


### pugixml

1. Find the latest version here: https://github.com/zeux/pugixml/releases/latest/
2. Edit the batch script, *pugixml.bat*, setting **px_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. The library is built as part of the CSPro solution.


### QR-Code-generator

1. Find the latest version here: https://github.com/nayuki/QR-Code-generator/releases
2. Edit the batch script, *qrcodegen.bat*, setting **qrcg_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. The license is at the bottom of *README.md* and is not automatically updated, so check if it should be updated.
6. The library is built as part of the CSPro solution.


### RapidFuzz

1. Find the latest version here: https://github.com/rapidfuzz/rapidfuzz-cpp/releases/latest/
2. Edit the batch script, *rapidfuzz.bat*, setting **rz_version**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. The library is built as part of the CSPro solution.


### ReadStat

1. Find the latest version here: https://github.com/WizardMac/ReadStat/releases/latest/
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

1. Find the latest version here: https://github.com/ReactiveX/RxCpp/releases/latest/
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

1. Find the latest version here: https://github.com/mariusbancila/stduuid/releases/latest/
2. Edit the batch script, *stduuid.bat*, setting **stduuid_version**.
3. Run the batch script.
4. This copies files into the CSPro solution.
5. There are CSPro modifications that have to be restored in:
    * uuid.h
6. The library is built as part of the CSPro solution.


### yaml-cpp

1. Find the latest version here: https://github.com/jbeder/yaml-cpp/releases/latest/
2. Edit the batch script, *yaml-cpp.bat*, setting **yaml_cpp_version**.
3. Run the batch script.
4. This copies files into the CSPro solution, including some that are not necessary.
5. Remove anything that is not already committed.
6. The library is built as part of the CSPro solution.


### zlib

1. Run the batch script, *zlib.bat*, from a Visual Studio command prompt.
2. This builds both x86 and x64 versions of zlib.
3. The built libraries are committed to the repository.
