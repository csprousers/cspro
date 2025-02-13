## Overview

Because most CSPro development occurs on a [private repository](https://github.com/CSProDevelopment/cspro), the history of this public repository does not reveal much about CSPro development. Because of this, this document lists information about each pull request merged into the private repository.


## CSPro 8.0.1

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro8.0.1.exe](https://www.csprousers.org/downloads/cspro/cspro8.0.1.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro8.0.1_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro8.0.1_releasenotes.txt)


## CSPro 8.0.0

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro8.0.0.exe](https://www.csprousers.org/downloads/cspro/cspro8.0.0.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro8.0.0_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro8.0.0_releasenotes.txt)

**Merged pull requests**:

| Date | Branch | Pull Request Message |
| --- | --- | --- |
| 2024&#8209;02&#8209;02 | [release/8.0.0](https://github.com/CSProDevelopment/cspro/commit/730804afa60be472776eecec350a7bc721d6a439) | prepared for the CSPro 8.0.0 release + added final bug fixes |
| 2024&#8209;01&#8209;31 | [json-symbol-finalization-for-8.0-part2](https://github.com/CSProDevelopment/cspro/commit/6355507cc37c01689fe741c8dac368909e2a5fce) | improved the JSON serialization of symbols, adding support for reading/writing engine values |
| 2024&#8209;01&#8209;30 | [binary-dictionary-items-for-8.0-final](https://github.com/CSProDevelopment/cspro/commit/3f4870cd8785b0fda7b6fe2df0a773252bc26fab) | added the binary dictionary item implementations concerning the dictionary tree / engine / pre-7.4 case data that have been part of the beta branches for a while |
| 2024&#8209;01&#8209;30 | [bug-fixes+small-work-2024-01](https://github.com/CSProDevelopment/cspro/commit/42f7f16924e59946f5b6cc3b729d5f1bf77e9dab) | fixed some bugs and worked on some small tasks |
| 2024&#8209;01&#8209;11 | [connection-string-camelCase](https://github.com/CSProDevelopment/cspro/commit/2d7604577a026a653d28589389f9c86b2dc94de1) | reworked the connection string properties to use camelCase |
| 2024&#8209;01&#8209;07 | [CString-removal-2024-01](https://github.com/CSProDevelopment/cspro/commit/d73656d269c8a0161539329916de8cd84fdc779a) | refactored the uses of CString in zExportO and several of the tools |
| 2024&#8209;01&#8209;05 | [release-prep+bug-fixes](https://github.com/CSProDevelopment/cspro/commit/1a3d6e5cde14d785f400a866fb0063f459a93a98) | prepared the code for the 8.0 release + reworked the Installer Generator to read inputs from JSON + fixed some bugs |
| 2024&#8209;01&#8209;02 | [view-standardization](https://github.com/CSProDevelopment/cspro/commit/af0565aa0a483b379854ce8f108e734679ff8930) | standardized the [symbol].view functions so that they all support the optional viewer options |
| 2023&#8209;12&#8209;29 | [beta-bug-fixes-2023-12](https://github.com/CSProDevelopment/cspro/commit/fd96ebd3ce231289f8ed6a2657e94f5f2fffb27a) | fixed some bugs from beta testing |
| 2023&#8209;12&#8209;29 | [components-framework-for-8.0](https://github.com/CSProDevelopment/cspro/commit/0dd4af92222ff620d7cc898525ccfa9a3d4b47c9) | added the framework for accessing HTML components for CSPro 8.0 |
| 2023&#8209;12&#8209;29 | [Chart.js-frequency](https://github.com/CSProDevelopment/cspro/commit/bd197a49afa17d585b992cb04dec5a94bb9d3dc1) | added the chart component and implemented displaying frequency charts in Table Viewer |
| 2023&#8209;12&#8209;29 | [json-symbol-finalization-for-8.0-part1](https://github.com/CSProDevelopment/cspro/commit/8b9a74f51f62df506d4029f578e7c696b1dcf84c) | implemented Array and .sva file JSON serialization + modified some symbol serialization routines based on observations while documenting |
| 2023&#8209;12&#8209;18 | [documenting-8.0](https://github.com/CSProDevelopment/cspro/commit/e3b020f58a0a5c0fac5588a6298ae38c012ac4d3) | modifications made based on observations documenting CSPro 8.0 features |
| 2023&#8209;11&#8209;14 | [beta-8.0-release2+bug-fixes](https://github.com/CSProDevelopment/cspro/commit/4e9c6aa31be8ef69c2ca94f7619e45d16ca00af7) | fixed some bugs and and prepped for the second 8.0 beta |
| 2023&#8209;11&#8209;13 | [questionnaire-viewer-integration](https://github.com/CSProDevelopment/cspro/commit/97767258795e86371db48641a034914a15ae2aa6) | implemented the ability to view questionnaires from the case listing |
| 2023&#8209;11&#8209;13 | [ActionInvokerActivity](https://github.com/CSProDevelopment/cspro/commit/1df1f43d60f363dd155f3490b0822536110672b6) | added features to support Android inter-application communication using the Action Invoker |
| 2023&#8209;10&#8209;16 | [beta-prep+small-work+Hash-Sqlite-actions](https://github.com/CSProDevelopment/cspro/commit/2bbebc0e8d2b65c1f08b399953b7a80596eb7466) | prepared the first 8.0 beta release + fixed some bugs and worked on some small tasks + added to the Action Invoker hash and SQLite-related actions |
| 2023&#8209;09&#8209;28 | [questionnaire-viewer](https://github.com/CSProDevelopment/cspro/commit/4b2f17ab098fbf57ab850ab2831a3a83a9bc93a3) | fleshed out the questionnaire viewer, adding the initial rendering implementation and reworking the way it gets data to use the Action Invoker |
| 2023&#8209;09&#8209;27 | [small-work-2023-09-27](https://github.com/CSProDevelopment/cspro/commit/b5084f36364fae4a6846de5503440ba9b5d0fae1) | fixed some bugs and worked on some small tasks |
| 2023&#8209;09&#8209;20 | [deep-link-fix](https://github.com/CSProDevelopment/cspro/commit/fc6c737a001d52a296a95f9c0476c77eb85764f4) | Added deep link activity back. |
| 2023&#8209;09&#8209;15 | [action-invoker+access-token](https://github.com/CSProDevelopment/cspro/commit/d9e8287f1a7430d2ab0bf21ce3317cc854d8ac9e) | implemented Action Invoker access tokens + added File and Message actions |
| 2023&#8209;09&#8209;12 | [CSDocument-small-work-2023-09](https://github.com/CSProDevelopment/cspro/commit/ccebd1cc32e3b05c64bcb4f09dd32e64a74e2bd7) | added some enhancements to CSDocument + fixed some bugs and worked on some small tasks |
| 2023&#8209;09&#8209;12 | [Scintilla-Lexilla-upgrade](https://github.com/CSProDevelopment/cspro/commit/779563136f5624561215f18554cd89139e030c1e) | upgraded Scintilla to version 5.3.6 (with Lexilla 5.2.6) |
| 2023&#8209;09&#8209;06 | [CSView](https://github.com/CSProDevelopment/cspro/commit/41c57cd69b15c8f10b64a1735cdb671e977a9d86) | added a new tool, CSView, that displays files in a web browser that has access to the Action Invoker |
| 2023&#8209;09&#8209;06 | [small-work-2023-09](https://github.com/CSProDevelopment/cspro/commit/d5113456fab54261fc2f2112aff055d718d28d73) | fixed some bugs and worked on some small tasks |
| 2023&#8209;09&#8209;01 | [specification-file-downgrader](https://github.com/CSProDevelopment/cspro/commit/a846bec511d7bb408fd9e0f9a2129fc88c92c486) | added JavaScript routines to downgrade dictionary/application specification files to CSPro 7.7 format |
| 2023&#8209;09&#8209;01 | [string-literals-and-newline-escaping](https://github.com/CSProDevelopment/cspro/commit/d8c8c709ac6e611ac96aab5d9be885d9379651f4) | added support for newlines throughout CSPro + added verbatim string literals to the CSPro language |
| 2023&#8209;08&#8209;16 | [Bootstrap-5.3.0](https://github.com/CSProDevelopment/cspro/commit/729eb651a3584d67f3ed85fa774b4edd566c7328) | Bootstrap 5.3.0 |
| 2023&#8209;08&#8209;15 | [bug-fixes+small-work+beta-prep](https://github.com/CSProDevelopment/cspro/commit/5158fee52fb1a14594aacb53086ff689d1116e03) | fixed some bugs, worked on some small tasks, and prepped for the first 8.0 beta |
| 2023&#8209;08&#8209;15 | [binary-dictionary-items-for-8.0](https://github.com/CSProDevelopment/cspro/commit/1bcf449791fb39b7ac34b8f0975fce6787a16c2b) | enabled binary dictionary items + reworked the binary symbols to support these items + added some Item/Map/Symbol logic functions + fixed binary sync bugs + modified the logic colorization of dot-notation words |
| 2023&#8209;06&#8209;20 | [wasm-framework](https://github.com/CSProDevelopment/cspro/commit/353d895fe078fd1816285015b9eecd59a3595cc7) | added the framework for building CSEntry in the WASM environment |
| 2023&#8209;06&#8209;09 | [external-library-update-for-8.0](https://github.com/CSProDevelopment/cspro/commit/a646926a0aef6ae9b2d1f95525dff54da81eb333) | updated external libraries in preparation for the CSPro 8.0 build |
| 2023&#8209;06&#8209;07 | [binary-sync-dropbox-ftp](https://github.com/CSProDevelopment/cspro/commit/f06380d560b5d1a54b79aa83c912a113c23d9b75) | Merge binary sync changes to dev |
| 2023&#8209;06&#8209;07 | [CSDocument](https://github.com/CSProDevelopment/cspro/commit/35ff1925fcfe080ad8d90943ffee480394937c2f) | added a new tool, CSDocument, to replace (and extend) the Help Generator tool |
| 2023&#8209;06&#8209;07 | [action-invoker](https://github.com/CSProDevelopment/cspro/commit/d1ee4eb478726d607df06969c2fad0dd4239cb07) | added the framework for the Action Invoker, along with the initial set of actions |
| 2023&#8209;02&#8209;02 | [small-work-2023-01-set1](https://github.com/CSProDevelopment/cspro/commit/92868e73bbc0b6de886b35367254ed935a2e80ca) | fixed some bugs and addressed some requests (January 2023, set 1) |
| 2023&#8209;01&#8209;31 | [chart.js-framework](https://github.com/CSProDevelopment/cspro/commit/ba9d934f76b1776acf46380aa8b911b197a88b47) | added a framework for minimal charting support using the Chart.js framework |
| 2023&#8209;01&#8209;30 | [compiler-to-single-dll-01](https://github.com/CSProDevelopment/cspro/commit/a0a320bd83998ea6d03002618e62f267ce5eec2f) | started moving the CSPro logic compiler to the zEngineO DLL |
| 2023&#8209;01&#8209;24 | [symbol-storage-centralization](https://github.com/CSProDevelopment/cspro/commit/4a6f08884382d02a16bc188d2e326e836e490072) | reworked the storage of symbols so that the symbol table stores all symbols as shared pointers |
| 2022&#8209;12&#8209;20 | [small-work-2022-11+12-set1](https://github.com/CSProDevelopment/cspro/commit/640bd82c357124b2c970010c8210eea3fbac0a09) | fixed some bugs (November and December 2022, set 1) |
| 2022&#8209;12&#8209;20 | [qr-code-creation](https://github.com/CSProDevelopment/cspro/commit/de0af069999537de00a1bcd42d3db5c9c4766909) | added support for creating QR codes from logic |
| 2022&#8209;12&#8209;20 | [cscode](https://github.com/CSProDevelopment/cspro/commit/437e7189e837b0b04acaec0a23cd64db0c00485e) | added the start of CSCode, a new tool for editing CSPro-related code files |
| 2022&#8209;12&#8209;20 | [tree-node-refactor](https://github.com/CSProDevelopment/cspro/commit/27182d225fbacb4494dffac2229f351c0d30a598) | refactored the dictionary, order, table, and file (application) trees to store data in TreeNode subclasses |
| 2022&#8209;12&#8209;20 | [CodeFile](https://github.com/CSProDevelopment/cspro/commit/1f66cd519339120d41fe6a24a62ea6a8fb5f07c4) | added CodeFile, a new framework for adding code files to an application (which can eventually support CSPro modules and JavaScript) |
| 2022&#8209;11&#8209;21 | [chm-browser](https://github.com/CSProDevelopment/cspro/commit/2f01dcd68980fe099e5c669551b3c18aabbd5799) | created a help (CHM) file reader and allowed files to be viewed using a local server |
| 2022&#8209;10&#8209;18 | [clipboard-logic-access](https://github.com/CSProDevelopment/cspro/commit/851e8e9b801bf4d517dd6130ac07dbcd1fd406db) | added logic functions to support copying and pasting text to the clipboard |
| 2022&#8209;10&#8209;05 | [small-work-2022-10-set1](https://github.com/CSProDevelopment/cspro/commit/2b2ead1468320946626a0ca6c5aaeabc70ccb7b8) | fixed some bugs and addressed some requests (October 2022, set 1) |
| 2022&#8209;10&#8209;03 | [string-literal-helpers](https://github.com/CSProDevelopment/cspro/commit/38923338f7d0ea9a14eb751133b474cccd14aaca) | added the Paste as String Literal feature, Path Adjuster dialog, and String Encoder dialog |
| 2022&#8209;10&#8209;03 | [symbols-json+persistent](https://github.com/CSProDevelopment/cspro/commit/d121f5242d17e7257ced57125b7b002e69cc90da) | added JSON serializers for most symbols + added the getSymbolJSON, getSymbolValueJSON, runLogic, and updateSymbolFromJSON functions |
| 2022&#8209;09&#8209;13 | [symbols-smart-pointers+wstring](https://github.com/CSProDevelopment/cspro/commit/0cc39f7a5019fff7dc7ccf2d1f44105e1af22421) | refactored the uses of CString in zEngineO's symbols to use std::wstring + reworked how EngineUI is processed |
| 2022&#8209;09&#8209;06 | [compiler-settings+case-sensitive-symbols+new-multiline-comments](https://github.com/CSProDevelopment/cspro/commit/187ee9c3749c14dd547856e1e77863a4ff38d771) | added new logic options, including using /* */ for multiline comments and allowing the enforcement of case sensitive symbols |
| 2022&#8209;08&#8209;23 | [questionnaire-viewer-framework](https://github.com/CSProDevelopment/cspro/commit/d780ffa4a7fed15929ac272d8b5599df4aa96216) | added the questionnaire viewer framework, including the designer view, the caseviewer object, and the CSEntry access pointers |
| 2022&#8209;08&#8209;16 | [toolbar-automator-and-standardizer](https://github.com/CSProDevelopment/cspro/commit/29e74e0a5037f1405b9593bb703b21ce3d8aff7f) | added toolbar icons to the Toolbar Creator automator, standardizing the graphics across projects |
| 2022&#8209;08&#8209;10 | [zListingO-wstring-refactor+batch-output-bug-fix+json-frequencies](https://github.com/CSProDevelopment/cspro/commit/39e5b60112a68c043c444c65a8a5459a00dfa7bd) | added frequencies to the JSON lister + refactored zListingO's uses of CString to use std::wstring and wstring_view |
| 2022&#8209;08&#8209;08 | [small-work-2022-08-set1](https://github.com/CSProDevelopment/cspro/commit/a09ba86211a8236611270dde7865bbba8978b823) | refactored the uses of CString in RunPFF and Text Converter + served GeoJSON content using a virtual file mapping |
| 2022&#8209;08&#8209;08 | [wstring-template-default](https://github.com/CSProDevelopment/cspro/commit/d4fc2553a6c197cb501571cdcfd1fdd916e256c2) | modified the default template option to wstring for functions added to support both CString and wstring objects |
| 2022&#8209;08&#8209;05 | [zJson+zJavascript+zHtml-wstring-refactor](https://github.com/CSProDevelopment/cspro/commit/80492bec1f2c735ee3ae8fbed8f7dd94d334567d) | refactored the uses of CString in zJson, zJavaScript, and zHtml to use std::wstring and wstring_view |
| 2022&#8209;08&#8209;04 | [pff-to-zAppO](https://github.com/CSProDevelopment/cspro/commit/2ba1679530ce870c641abe927aa943be81cfe16d) | moved the CNPifFileBase (renamed PFF) class from zBridgeO to zAppO + added PFF Editor to the main solution + added wstring DDX handlers for edit and combo box controls |
| 2022&#8209;08&#8209;02 | [zFreqO-wstring-refactor](https://github.com/CSProDevelopment/cspro/commit/9e6fa0e6e83654032f80d04ae93a72137aed1afc) | refactored zFreqO's uses of CString to use std::wstring and wstring_view |
| 2022&#8209;08&#8209;01 | [portable-mfc-container-removal](https://github.com/CSProDevelopment/cspro/commit/e23b8ad2061802013a4d29f5bb750b5679d4363f) | removed uses of MFC container classes in portable code |
| 2022&#8209;07&#8209;27 | [zMessageO-wstring-refactor](https://github.com/CSProDevelopment/cspro/commit/0d0a847d12d2a0b544143e4082c65ff290a004a3) | refactored zMessageO's uses of CString to use std::wstring and wstring_view |
| 2022&#8209;07&#8209;27 | [json-lister](https://github.com/CSProDevelopment/cspro/commit/2f155b74a432932ace6cedead249a9109e1c899a) | implemented the JSON lister (with frequency support unimplemented for now) |
| 2022&#8209;07&#8209;15 | [json-frequency-printer](https://github.com/CSProDevelopment/cspro/commit/d5e886ae3f8eb7646decdc1bbd8b87371c89ead8) | allowed frequencies to be written to JSON format |
| 2022&#8209;07&#8209;14 | [small-work-2022-07-set1](https://github.com/CSProDevelopment/cspro/commit/7825db30fca7ae335d05d22104898f7a03468650) | fixed some bugs and addressed some requests (July 2022, set 1) |
| 2022&#8209;07&#8209;08 | [cspack-refactor](https://github.com/CSProDevelopment/cspro/commit/4311fe4dd3df85f9557ae505b70c457cd8fb8ef3) | redesigned the Pack Application tool to include a specification file and to support packing multiple inputs |
| 2022&#8209;06&#8209;27 | [json-repository](https://github.com/CSProDevelopment/cspro/commit/7913d58a157d05318657177ac55f7100f0b483bd) | added JSON and In-Memory data sources |
| 2022&#8209;06&#8209;15 | [virtual-file-mapping](https://github.com/CSProDevelopment/cspro/commit/f88085770e4504806c2a6c5187140b8d2ad05a60) | added a mechanism to map virtual files to the local server to serve files or content |
| 2022&#8209;05&#8209;18 | [json-question-text](https://github.com/CSProDevelopment/cspro/commit/42e1d655a795661796ee9d5f84a0872a65c17215) | added a JSON writer for question text + reworked the question text .pen file serialization to use the serialization format used throughout CSPro rather than embedding YAML into the .pen file |
| 2022&#8209;05&#8209;17 | [form-cleanup-1](https://github.com/CSProDevelopment/cspro/commit/a1700e7d3e1afaff4c3875743de9424498b794cb) | cleaned up the zFormO and zFormF projects (part 1) + added the PortableFont class |
| 2022&#8209;05&#8209;05 | [small-work-2022-05-set1](https://github.com/CSProDevelopment/cspro/commit/dd814deb976c2f27efd6db89b89d78d28c0e3843) | fixed some bugs and addressed some requests (May 2022, set 1) |
| 2022&#8209;05&#8209;04 | [QuickJS-integration](https://github.com/CSProDevelopment/cspro/commit/d45f0ee37dc084e226d35951b7dd64b6d5018e06) | added QuickJS as a JavaScript executor to the CSPro solution (via the zJavaScript DLL) |
| 2022&#8209;04&#8209;27 | [small-work-2022-04-set2](https://github.com/CSProDevelopment/cspro/commit/45f9bd6dc344f55441b744c302616dbe944a0c7f) | implemented freq_name.save(REPORT_NAME) as a way to embed frequencies into reports + fixed some bugs and addressed some requests (April 2022, set 2) |
| 2022&#8209;04&#8209;27 | [json-application](https://github.com/CSProDevelopment/cspro/commit/057e7bb813bd299570da4ab5185b4a728e978b38) | converted the application file to JSON format + reworked the specification of the application properties file (.csprops) so that it can be done as part of the Application Properties dialog |
| 2022&#8209;04&#8209;14 | [small-work-2022-04-set1](https://github.com/CSProDevelopment/cspro/commit/fb63c7c36f58048c685024e12bff57527453674d) | fixed some bugs and addressed some requests (April 2022, set 1) |
| 2022&#8209;04&#8209;14 | [json-dictionary](https://github.com/CSProDevelopment/cspro/commit/786676617a2f811491154e1b4780f1434275d8b2) | converted the dictionary to JSON format |
| 2022&#8209;04&#8209;11 | [dictionary-clipboard-json](https://github.com/CSProDevelopment/cspro/commit/353e7cd99411e06cdeda7f5c9bfa98fc78b0ee65) | refactored the copying and pasting of dictionary elements to use JSON serialization |
| 2022&#8209;03&#8209;28 | [DictLevel-refactoring](https://github.com/CSProDevelopment/cspro/commit/c4a8f22cc8c98e968c941ce29d58ace9fc0f155c) | removed the dependency of dictionary levels on MFC classes and modified the storage to be objects, not pointers to objects |
| 2022&#8209;03&#8209;21 | [new-file-dialog-redesign+operational-control-settings](https://github.com/CSProDevelopment/cspro/commit/30e81db2abada62d04e581c8969ce69e0a237cdb) | redesigned the Designer's New File dialog, creating a three-column approach (category/type/description) + added an option to create data entry applications with default settings for operational control systems |
| 2022&#8209;03&#8209;21 | [BCMenu-upgrade](https://github.com/CSProDevelopment/cspro/commit/857f8118ba9dc8120b22b7be9595a84fae3a58b7) | updated BCMenu to the latest version (from 2002) |
| 2022&#8209;03&#8209;21 | [bug-fixes-2022-03](https://github.com/CSProDevelopment/cspro/commit/e5e5149464c06dde6ab24dd3ca2dff436fda00f5) | fixed some bugs and addressed some requests (March 2022) |
| 2022&#8209;03&#8209;15 | [binary-sync](https://github.com/CSProDevelopment/cspro/commit/83d505b07bbb17747347cf321473799d31416b4e) | Read and write binary data |
| 2022&#8209;03&#8209;15 | [value-set-refactoring](https://github.com/CSProDevelopment/cspro/commit/d4ec2f29aed6299a4a7c472e932a85696f702d56) | removed the dependency of dictionary value sets and relations on MFC classes and modified the storage to be objects, not pointers to objects |
| 2022&#8209;03&#8209;14 | [image-property-panel](https://github.com/CSProDevelopment/cspro/commit/d61774ec7f8b8ccbb540116700c32d4aa8ba7f68) | added a property panel control for editing filenames and image filenames |
| 2022&#8209;03&#8209;07 | [LabelSet](https://github.com/CSProDevelopment/cspro/commit/0848e6c394298c23bcab210b2dc83b9ddfbe1632) | added a class, LabelSet, for managing labels in multiple languages + used this for dictionary labels and occurrence labels |
| 2022&#8209;03&#8209;04 | [remove-CDEField-occurrences](https://github.com/CSProDevelopment/cspro/commit/769d4873f20a15de44504143fc75d574b266d86a) | removed code related to the never-fully-implemented feature of being able to have separate CDEField objects for multiple occurrences of an item |
| 2022&#8209;02&#8209;22 | [question-text-height-resize](https://github.com/CSProDevelopment/cspro/commit/cb0867f61bd6ea7d796ebf37e489276f49f70db0) | allowed the question text window height to be resized via JavaScript using CSPro.setDisplayOptions |
| 2022&#8209;02&#8209;10 | [trs-removal](https://github.com/CSProDevelopment/cspro/commit/466bd42995c2798691201e6eb155d6493900860a) | removed TRS from CSPro |
| 2022&#8209;02&#8209;10 | [json-spec-files-tools-part1](https://github.com/CSProDevelopment/cspro/commit/1ff8738367fedb133b97bf7126be5f5faf3aff45) | converted the .cmp, .ssf, .trs, .fqf, .xl2cs, and .exf formats to JSON format + standardized the .csds format with the others |
| 2022&#8209;01&#8209;25 | [visual-studio-2022-upgrade](https://github.com/CSProDevelopment/cspro/commit/83613156b8cdcf85e5870d599d3c97f18cc52bc3) | updated the CSPro solution and tools for Visual Studio 2022 |
| 2022&#8209;01&#8209;25 | [zAppO+language-centralization](https://github.com/CSProDevelopment/cspro/commit/f0c62b31cda3008aafbda471a654092ef8191d63) | moved application-related objects to a new DLL, zAppO + refactored CDictLanguage and CapiLanguage to use a new object, Language |
| 2022&#8209;01&#8209;18 | [persistent-variables](https://github.com/CSProDevelopment/cspro/commit/8cfb15a1a042893692b799a623c9815389fa096a) | added the persistent variable modifier as a way to persist variable values across program runs |
| 2022&#8209;01&#8209;18 | [json-reader](https://github.com/CSProDevelopment/cspro/commit/d8afd785f16e08861a937083d8e7efd100284445) | added a JSON reader (wrapping jsoncons functionality) |
| 2022&#8209;01&#8209;06 | [cURL-to-zNetwork](https://github.com/CSProDevelopment/cspro/commit/6361e55fa2134655f7a2760226d4d6f904e31d1a) | created a new DLL, zNetwork, with cURL-related files |
| 2022&#8209;01&#8209;05 | [json-writer](https://github.com/CSProDevelopment/cspro/commit/8450f75e93d55db0f58901f0576030e4737507fe) | added a JSON writer (wrapping jsoncons functionality) |
| 2021&#8209;12&#8209;28 | [numeric-config-variables](https://github.com/CSProDevelopment/cspro/commit/a9bc3d3021bbe74c434318719df00b0ecaed53d4) | added support for numeric config variables |
| 2021&#8209;12&#8209;22 | [code-cleanup](https://github.com/CSProDevelopment/cspro/commit/24bb285fef0ab5a91a9e92967fac4c17fa9ddc2b) | cleaned up various parts of the code and added some efficiencies |
| 2021&#8209;12&#8209;20 | [always-visual-value-property](https://github.com/CSProDevelopment/cspro/commit/b65ef1abae4ec8a02c8f61a526948a1029d4190c) | added a field property, "Always Visual Value" |


## CSPro 7.7.3

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro7.7.3.exe](https://www.csprousers.org/downloads/cspro/cspro7.7.3.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro7.7.3_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro7.7.3_releasenotes.txt)

**Merged pull requests**:

| Date | Branch | Pull Request Message |
| --- | --- | --- |
| 2022&#8209;08&#8209;08 | [Glenn_fixes](https://github.com/CSProDevelopment/cspro/commit/1a2f0214a3e4133f4eac5565f517a0b952647b3c) | Glenn's fixes to CSFreq bugs |


## CSPro 7.7.2

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro7.7.2.exe](https://www.csprousers.org/downloads/cspro/cspro7.7.2.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro7.7.2_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro7.7.2_releasenotes.txt)

**Merged pull requests**:

| Date | Branch | Pull Request Message |
| --- | --- | --- |
| 2022&#8209;06&#8209;16 | [dropbox-oauth](https://github.com/CSProDevelopment/cspro/commit/0283a3d22956adc53f4bec832b04462df4f000aa) | changes to use Dropbox short lived tokens |


## CSPro 7.7.1

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro7.7.1.exe](https://www.csprousers.org/downloads/cspro/cspro7.7.1.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro7.7.1_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro7.7.1_releasenotes.txt)

**Merged pull requests**:

| Date | Branch | Pull Request Message |
| --- | --- | --- |
| 2022&#8209;02&#8209;08 | [portable-cstring-append-bug](https://github.com/CSProDevelopment/cspro/commit/1e6c0945f81854e00d7a803e3b1337b929dc64a8) | fixed a bug whereby appended strings got truncated around 1024 characters on Android |
| 2022&#8209;02&#8209;07 | [ShowExtendedControlTitle-flag](https://github.com/CSProDevelopment/cspro/commit/3b2fd8e6d20fe04ac3811fd6ae21eba6e6c517b1) | added the ShowExtendedControlTitle property as a way to turn on/off extended control captions without getting deprecation warnings |
| 2022&#8209;02&#8209;07 | [getos-hashmap](https://github.com/CSProDevelopment/cspro/commit/ee8757fa851571e479bc073a9a57afa88871ddb4) | allowed the getos logic function to take a hashmap as an argument with the hashmap being filled with details about the operating system |
| 2022&#8209;02&#8209;07 | [HtmlDialogs-override](https://github.com/CSProDevelopment/cspro/commit/1f2df78d79561135f2ff9a3f75e28983c985ce55) | added a HtmlDialogs PFF attribute to allow users to override where the HTML dialogs are located |
| 2022&#8209;02&#8209;07 | [7.7.0-bug-fixes](https://github.com/CSProDevelopment/cspro/commit/7f3ae2d102469258880a60d2d379a4f03a4762f0) | fixed some CSPro 7.7.0 bugs |


## CSPro 7.7.0

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro7.7.0.exe](https://www.csprousers.org/downloads/cspro/cspro7.7.0.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro7.7.0_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro7.7.0_releasenotes.txt)

**Merged pull requests**:

| Date | Branch | Pull Request Message |
| --- | --- | --- |
| 2022&#8209;01&#8209;12 | [cspro77-beta1-final-modifications](https://github.com/CSProDevelopment/cspro/commit/a268b85602ee152527a0961c080e43eda6c3814b) | some final CSPro 7.7 beta modifications |
| 2021&#8209;12&#8209;21 | [cspro77-beta1-20211221-modifications](https://github.com/CSProDevelopment/cspro/commit/239510e9ebc4d86f965e12cacd71c61730c81d94) | CSPro 7.7 beta modifications for 2021-12-21 |
| 2021&#8209;12&#8209;20 | [cspro77-beta1-20211220-modifications](https://github.com/CSProDevelopment/cspro/commit/85cad12e72958d5883bbd71dc9d81c36a77d335b) | CSPro 7.7 beta modifications for 2021-12-20 |
| 2021&#8209;12&#8209;14 | [cspro77-beta1-20211214-modifications](https://github.com/CSProDevelopment/cspro/commit/232aefb3c55a721f4a61cd64e4698eb7c2b15503) | CSPro 7.7 beta modifications for 2021-12-14 |
| 2021&#8209;12&#8209;13 | [setbluetoothname-function](https://github.com/CSProDevelopment/cspro/commit/976f75cfda6a83502ac641f2deb06858cbdab736) | added a function, SetBluetoothName, to allow the setting of a device's Bluetooth broadcasting name |
| 2021&#8209;12&#8209;09 | [cspro77-beta1-20211209-modifications](https://github.com/CSProDevelopment/cspro/commit/10be2eebd2637545a0b92d98278f8a89204f2b14) | CSPro 7.7 beta modifications for 2021-12-09, including adding new code folding options |
| 2021&#8209;11&#8209;30 | [cspro77-beta1-prep](https://github.com/CSProDevelopment/cspro/commit/3f7c5a4838b14ca8dc3f1c142f8ec56085029a4d) | miscellaneous preparations for the first CSPro 7.7 beta release |
| 2021&#8209;11&#8209;23 | [logic-based-reports](https://github.com/CSProDevelopment/cspro/commit/28fd86d5732e30df4d4e377e76a703c94b088d7a) | added a reporting system that allows the use of logic while creating dynamic reports |
| 2021&#8209;11&#8209;22 | [scintilla-for-coloring](https://github.com/CSProDevelopment/cspro/commit/a34696ac236fbf92ec8db1d2c3c72e61d977e151) | refactored the logic coloring routines to use the styles from the Scintilla controls |
| 2021&#8209;11&#8209;11 | [deploy-paradatasync-fixes](https://github.com/CSProDevelopment/cspro/commit/fd7cf01606110ae422cab8b43da56f6eea3bdada) | fixed a syncparadata bug when chunking needed to be used + added standalone dictionaries to CSDeploy's list of dictionaries that can be synced |
| 2021&#8209;11&#8209;10 | [invoke-function](https://github.com/CSProDevelopment/cspro/commit/aba4f554836685399d97f1f2cddf8adb39b7f0da) | added the invoke function |
| 2021&#8209;11&#8209;08 | [77-alpha-testing-modifications-20211108](https://github.com/CSProDevelopment/cspro/commit/450484acefb8a9ed91ebb583cd24ed73bdc5e8f8) | fixed issued discovered while testing the CSPro 7.7 alpha release |
| 2021&#8209;11&#8209;05 | [recursion-and-function-pointer-enhancements](https://github.com/CSProDevelopment/cspro/commit/86df9523c725b2f5f8ecfe9585b1b9dc9f564cb8) | enhanced user-defined functions so that more symbol types could be included as the parameters to function pointers as well as used in the body of functions called recursively |
| 2021&#8209;10&#8209;27 | [engine-compilation-unit-cleanup](https://github.com/CSProDevelopment/cspro/commit/109e37d0c3de2e1764468a30bb94fa5b10f2ef34) | moved runtime functions into more type-specific compilation units + used the same date calculation routines on Windows/Android |
| 2021&#8209;10&#8209;26 | [downloads-directory](https://github.com/CSProDevelopment/cspro/commit/a8dee8df5aaa60d2aae2585266754d394d6fece6) | added support for interacting with the Downloads folder (with path.selectFile and functions that access path types) |
| 2021&#8209;10&#8209;25 | [2021-10-misc-fixes](https://github.com/CSProDevelopment/cspro/commit/ef733e6238b3eb007bb181268c7b86963c11bd9c) | miscellaneous fixes to issues raised in October 2021 |
| 2021&#8209;10&#8209;07 | [77-new-object-modifications](https://github.com/CSProDevelopment/cspro/commit/e37b52c450863b3c5aff08fe6e785d154d5fd2b5) | modified the image.resample function so that images can be resampled using the image's aspect ratio + added options for max width/height resampling |
| 2021&#8209;10&#8209;05 | [2021-10-05-bugs](https://github.com/CSProDevelopment/cspro/commit/7bd389ff6f09944a52fc583df95b10b7d6e6badd) | fixed some bugs with new CSPro 7.7 features |
| 2021&#8209;10&#8209;05 | [text-edit-disable-on-empty-text](https://github.com/CSProDevelopment/cspro/commit/e10a65361256a65f34de474af9800e98c76b0820) | added a flag to the HTML Text Input dialog to indicate whether blank text is allowed |
| 2021&#8209;10&#8209;05 | [path.selectFile](https://github.com/CSProDevelopment/cspro/commit/9b4f251fb09c8f43b95c13bfa3a6cd1e0ac78f12) | added a logic function, path.selectFile, that allows users to visually select files |
| 2021&#8209;10&#8209;05 | [cspro-javascript-sync-async](https://github.com/CSProDevelopment/cspro/commit/af5856ae27dd5178c0be2ed87546bdc053a48129) | fleshed out the CSPro <-> JavaScript interface, allowing synchronous and asynchronous calls into CSPro + partially implemented path.selectFile |
| 2021&#8209;09&#8209;23 | [view(freq)+minor-fixes](https://github.com/CSProDevelopment/cspro/commit/bcaacb446e0ea2fd633bf8080d7f5626ea969d59) | allowed the view function to take a freq object + fixed a bug passing frequencies as user-defined arguments + fixed a bug with Windows mapping in release |
| 2021&#8209;09&#8209;23 | [htmldialog-function](https://github.com/CSProDevelopment/cspro/commit/0e1899adb26bfbe1cdaedbcc71def79c9740608a) | added a logic function, HtmlDialog, that allows users to create new UI elements using HTML and JavaScript |
| 2021&#8209;09&#8209;16 | [synctime](https://github.com/CSProDevelopment/cspro/commit/7794e160f0bdbc2cb62d22131bedd89f228c1332) | added a function, synctime, that returns the last sync time in general or specifically for a case |
| 2021&#8209;09&#8209;15 | [cspro77-new-feature-modifications-and-release-prep](https://github.com/CSProDevelopment/cspro/commit/87ac33e46de8b0c6b147f637b7370b7d12b352e0) | made slight modifications to some new CSPro 7.7 features and started preparing the code for the release |
| 2021&#8209;09&#8209;10 | [standardize-where-clauses](https://github.com/CSProDevelopment/cspro/commit/0a03a2a394e928c157ce1add61b73c5a1fa31601) | standardized the way that "where" clauses are evaluated by the engine |
| 2021&#8209;09&#8209;09 | [html-dialogs-both-platforms](https://github.com/CSProDevelopment/cspro/commit/e13cc2f8b021adeb44d35afad767e3ffdb7a0556) | reworked the UI for several engine functions to use HTML dialogs |
| 2021&#8209;09&#8209;02 | [html-dialog-framework](https://github.com/CSProDevelopment/cspro/commit/b9588a6af90a9c60b350f802eb1c4218fe0a1285) | added CSHtmlDlg, a dialog for displaying dialogs fully designed in HTML (to be used to redesign logic UI elements) |
| 2021&#8209;08&#8209;30 | [html-consolidation](https://github.com/CSProDevelopment/cspro/commit/d4af77537f13ec2b4d7a3141b39daa2431c00ef7) | consolidated HTML-related functionality to a DLL, zHtml + moved HTML/CSS/JavaScript files to a single directory, html |
| 2021&#8209;08&#8209;25 | [qsf-io-bug-unicode-characters](https://github.com/CSProDevelopment/cspro/commit/be810ee15ff851aa394f6d09471464e6b1ae8377) | fixed bug whereby question text couldn't be loaded/saved if non-Latin characters were in the path |
| 2021&#8209;08&#8209;24 | [zUtil2O-to-zUtilF+zUtilO-cleanup](https://github.com/CSProDevelopment/cspro/commit/982f49df9621ede6831df18f6207820e0c67a2c5) | renamed zUtil2O to zUtilF and moved UI-themed classes from zUtilO to zUtilF |
| 2021&#8209;08&#8209;23 | [extended-controls-sizing-large-field-bug](https://github.com/CSProDevelopment/cspro/commit/a3ba21620c7e71cdb76b09d1128354756ecc3d90) | fixed a bug whereby the extended control window for extraordinarily large fields would be launched with inappropriately (unnecessarily) large sizes |
| 2021&#8209;08&#8209;23 | [zip-filename-slashes](https://github.com/CSProDevelopment/cspro/commit/1937eb3c5401ca8a6c6813d238bade2259db9300) | fixed bug which failed to remove files from a zip file when using syncapp over Bluetooth (because of mismatched path slashes) |
| 2021&#8209;08&#8209;23 | [application-properties-paradata](https://github.com/CSProDevelopment/cspro/commit/eebcdd51fd7637e3b41921004a7fe214405fb296) | refactored the paradata properties into the application properties framework |
| 2021&#8209;08&#8209;19 | [application-properties-and-mapping-options](https://github.com/CSProDevelopment/cspro/commit/3cdff555b32325fa500349ed7934bccb5ae7624f) | added the application properties infrastructure + implemented mapping properties |
| 2021&#8209;08&#8209;17 | [csdeploy-barcode-issues](https://github.com/CSProDevelopment/cspro/commit/907790609dc26fee95adae8075be4b4fa4753e61) | removed the CS logo in the deployment barcode (because the new version of QRCoder doesn't render it properly) + trimmed the server URL in the barcode to avoid download issues later (where the server isn't trimmed) |
| 2021&#8209;08&#8209;09 | [bug-fixes-from-smg-2021-08](https://github.com/CSProDevelopment/cspro/commit/b70e10d386c50b52068198538aa2f81664b8e910) | fixed a crash that occurred when setting the default text font + reworked the managing of question text heights |
| 2021&#8209;08&#8209;09 | [web-browser-cspro-interaction](https://github.com/CSProDevelopment/cspro/commit/9caf77d835fd6b3690be691c0064c442bdf6833c) | added an interface for calling CSPro user-defined functions from JavaScript in question text or in web pages shown using the view function |
| 2021&#8209;08&#8209;05 | [bug-fixes](https://github.com/CSProDevelopment/cspro/commit/b58b4d66f9cf6e411148d27afe11329a0dcc6b73) | Various bug fixes |
| 2021&#8209;08&#8209;03 | [gps-readinteractive-select](https://github.com/CSProDevelopment/cspro/commit/818155f76e94504e295f3e9ca70a1e17b89480d4) | added a location widget for capturing GPS coordinates |
| 2021&#8209;08&#8209;02 | [windows-mapping](https://github.com/CSProDevelopment/cspro/commit/b0e63df2cf3e5b6a00e7225bdafc8541b34c67e3) | implemented the map object's functionality on Windows |
| 2021&#8209;07&#8209;12 | [dropbox-barcode-addapplication](https://github.com/CSProDevelopment/cspro/commit/30406dd071c9c525f3eeae7aebbbd76d8f56ff63) | Fixed issue when application could not be downloaded from DropBox using QR code |
| 2021&#8209;07&#8209;07 | [windows-engine-webview-interaction](https://github.com/CSProDevelopment/cspro/commit/5d1b60cdc82cf0236890ecaeb9309b2ff1cae5f8) | allowed web pages (displayed using the engine's View HTML dialog) to use JavaScript to call CSPro user-defined functions (on Windows) |
| 2021&#8209;07&#8209;02 | [paradata-location-mapper-leaflet](https://github.com/CSProDevelopment/cspro/commit/b49b9c322ee93a3dea9e14c63efb8afccb9e862e) | refactored the Paradata Viewer's Location Mapper plugin to use Leaflet |
| 2021&#8209;07&#8209;01 | [generic-webview-dialog](https://github.com/CSProDevelopment/cspro/commit/41fe7786258a05b722b849c37c2e920529781fef) | added a generic dialog that can show HTML (via the WebView2 control) + added functionality to show this on the UI thread of engine-based application |
| 2021&#8209;06&#8209;25 | [new-qsf-content-extractor](https://github.com/CSProDevelopment/cspro/commit/11a2d8112bd6b7f5504bd5cbe894c2451a5b25ca) | updated the Content Extractor to work with HTML-based question text files |
| 2021&#8209;06&#8209;23 | [scintilla-upgrade-and-cleanup](https://github.com/CSProDevelopment/cspro/commit/f6e92aea2b78f7b6cb2db6161829c74a4860cb28) | upgraded Scintilla to the latest 4.x version + removed unused lexers |
| 2021&#8209;06&#8209;23 | [string-syntax-highlight](https://github.com/CSProDevelopment/cspro/commit/5fe8567ee4d81a7a71370c8f5edc7e656a68cf89) | Fixed the bug where " and ' would be escaped in syntax highlighting |
| 2021&#8209;06&#8209;23 | [accept-html-dialog](https://github.com/CSProDevelopment/cspro/commit/546339d1c14cbf4c9319ebd6d5a8d0d5ad890419) | implemented a HTML version of the accept function |
| 2021&#8209;06&#8209;08 | [single-class-html-dialog](https://github.com/CSProDevelopment/cspro/commit/fd522817e6fceb848e4f9d9e070109b340b6aa84) | refactored the Windows HTML UI dialog from two classes to one + moved the HTML implementation of errmsg/select so that it can be used on Android |
| 2021&#8209;06&#8209;08 | [dictionary-capture-type](https://github.com/CSProDevelopment/cspro/commit/e1f6a7b07899516554cecdf1a39cdf2c4976e976) | allowed capture types to be specified in the dictionary |
| 2021&#8209;05&#8209;25 | [DataAccess-to-CaseAccess-and-DataItem-to-CaseItem-rename](https://github.com/CSProDevelopment/cspro/commit/e8530029ea4c264594c0f361705655efb84cfa2d) | renamed DataAccess to CaseAccess and *DataItem to *CaseItem |
| 2021&#8209;05&#8209;25 | [aliases](https://github.com/CSProDevelopment/cspro/commit/9f4ba83a88ea0d3b1acd27fe56e1dfe6cd23696e) | added the ability to specify aliases (using the property panel) in the dictionary editor |
| 2021&#8209;05&#8209;24 | [property-panel-occurrence-labels](https://github.com/CSProDevelopment/cspro/commit/d2c11f0cd570efe0877696b1a9ad8fed95fc6ae7) | added the editing of occurrence labels to the property panel |
| 2021&#8209;05&#8209;20 | [html-dialogs](https://github.com/CSProDevelopment/cspro/commit/37eee4d0a690100df47dd658996c06a9fd54ed70) | added a cross platform way to display HTML-based dialogs |
| 2021&#8209;05&#8209;19 | [property-grid-object-oriented-refactor+add-textcolor-property](https://github.com/CSProDevelopment/cspro/commit/ddab588fc068922f0ded6be4c86e831f9800a5e4) | refactored property panel to use a more object oriented approach and added the value label text color property |
| 2021&#8209;05&#8209;14 | [message-file-translations-bug](https://github.com/CSProDevelopment/cspro/commit/6cf14d4da818cf8038b22bae5342a0e3316cfbce) | fixed a bug with message translations becoming inaccessible if translations were provided before any messages |
| 2021&#8209;05&#8209;12 | [colored-value-labels](https://github.com/CSProDevelopment/cspro/commit/ce5d599acf25403c447a5050cd3b7f02ad0a999b) | added support for colored value set labels on Android and in the valueset.add function |
| 2021&#8209;05&#8209;10 | [new-batch-driver-framework](https://github.com/CSProDevelopment/cspro/commit/8b09fb78cf092fa2ef4466b39e5a3bbebe1c1e3e) | added the beginning of a new batch driver, implementing the reading writing of cases |
| 2021&#8209;05&#8209;05 | [advance-following-reenter-bug](https://github.com/CSProDevelopment/cspro/commit/ef20e4474779cf02ed68e04d5ce2a0a0f70c1e3a) | fixed a bug whereby an advance executed in a reenter's field's logic wouldn't stop the evaluation of the reenter |
| 2021&#8209;05&#8209;04 | [exporters-as-data-repositories](https://github.com/CSProDevelopment/cspro/commit/d9efe3e62c900177eca1941bd6ae06a7754d248d) | refactored the export writers to be accessed as data repositories |
| 2021&#8209;04&#8209;28 | [inadvance-function](https://github.com/CSProDevelopment/cspro/commit/943019b6ed7fe5f96d53d56d52ddeb21cfc272ce) | added the inadvance logic function |
| 2021&#8209;04&#8209;28 | [qsf-serialize-form-order](https://github.com/CSProDevelopment/cspro/commit/44c62109f4106a6694db3998cfdc599d80222622) | saved the CAPI questions in the .qsf file in form order |
| 2021&#8209;04&#8209;27 | [questionnaire-printer-framework](https://github.com/CSProDevelopment/cspro/commit/80ffda60d250c51a25291098eda79c3d91b5312e) | added the questionnaire printer framework |
| 2021&#8209;04&#8209;21 | [external-logic-in-designer](https://github.com/CSProDevelopment/cspro/commit/188ae9a633b58f34594457dd8a7dcba21d21fd63) | added support for editing external logic files to the Designer |
| 2021&#8209;04&#8209;13 | [toggle-button](https://github.com/CSProDevelopment/cspro/commit/adc01dddaf83068ad6963e1cbe45b13fa678888d) | added the Toggle Button capture type |
| 2021&#8209;04&#8209;12 | [capi-logic-symbol-search-fix](https://github.com/CSProDevelopment/cspro/commit/d0df26a3a1aaf79e03260d92370f30ec47e90552) | fixed the searching of CAPI logic symbols so that an invalid symbol doesn't get compiled as its parent symbol |
| 2021&#8209;04&#8209;08 | [multiple-output-files-in-batch](https://github.com/CSProDevelopment/cspro/commit/d5a5f02738f166ee0bee7f614c3a5c57f2bf9ab8) | allowed batch applications to write to multiple output data sources |
| 2021&#8209;04&#8209;07 | [stata-export](https://github.com/CSProDevelopment/cspro/commit/7467ee4fc49ffdec6915023ec883377c22b347a8) | added a Stata export writer to natively write .dta files |
| 2021&#8209;04&#8209;06 | [update-libraries](https://github.com/CSProDevelopment/cspro/commit/44e92178c67f5805b5e90ecbc8a3348f41b0c9a4) | upgraded several external libraries to the latest versions |
| 2021&#8209;04&#8209;06 | [sas-exporter](https://github.com/CSProDevelopment/cspro/commit/056da66c169417c4a7d86b62bfb85239a783d9bf) | added a SAS export writer to natively write .xpt/.sas files |
| 2021&#8209;03&#8209;30 | [spss-export](https://github.com/CSProDevelopment/cspro/commit/032546633744060fe2d62123160d0b4ea7178c11) | added a SPSS export writer to natively write .sav files |
| 2021&#8209;03&#8209;24 | [geometry-document-assignment](https://github.com/CSProDevelopment/cspro/commit/f1d3cb0cc20d46c2d028bbfcd23690347c51e459) | allowed the assignment of document=geometry and geometry=document |
| 2021&#8209;03&#8209;23 | [audio-capture-type](https://github.com/CSProDevelopment/cspro/commit/b2fd5164da9d20a0ef24e75b0ec8cc9236d94b61) | added Audio to the CaptureType options |
| 2021&#8209;03&#8209;23 | [setproperty-with-numeric-value+signature-capture-type](https://github.com/CSProDevelopment/cspro/commit/0a1d27ed5631ae47d58fdd40a4ec1d0347dfb7ab) | allowed setProperty to take a number as the value argument + added Signature to the CaptureType options |
| 2021&#8209;03&#8209;22 | [symbol-reset](https://github.com/CSProDevelopment/cspro/commit/1bbdb3caf5d70405454471a3c8aa31103ca52d69) | added a Symbol::Reset virtual method to simplify resetting symbols |
| 2021&#8209;03&#8209;22 | [geometry-testing-and-setproperty-with-numerics](https://github.com/CSProDevelopment/cspro/commit/6d67aeefb6b0b567f62a9ac7d2c2c577e5f59c2e) | allowed geometry.setProperty to take a number as the value argument + fixed some geometry bugs |
| 2021&#8209;03&#8209;19 | [geom-properties](https://github.com/CSProDevelopment/cspro/commit/98f62af18138ac15f6a70f489ebfec1414f6a4c5) | Support properties on geometry object |
| 2021&#8209;03&#8209;17 | [geom-fixes](https://github.com/CSProDevelopment/cspro/commit/f98b692abb51b7f11418291319a2853be5116bdf) | Geometry bug fixes |
| 2021&#8209;03&#8209;16 | [edit-geom](https://github.com/CSProDevelopment/cspro/commit/0390750aa77fee9dbeec4b7b32876d88dc3b4926) | Edit captured poly |
| 2021&#8209;03&#8209;16 | [geojson-parser-memory](https://github.com/CSProDevelopment/cspro/commit/a881b8ca7d35cfee1099201b9866d0c334295a65) | Make geojson parser a bit more memory efficient |
| 2021&#8209;03&#8209;15 | [geom-area](https://github.com/CSProDevelopment/cspro/commit/7a8257b50710c4cc5e822cccff5dff7673c91b3f) | Add area and perimeter functions to geometry object |
| 2021&#8209;03&#8209;15 | [fix-geom-submod](https://github.com/CSProDevelopment/cspro/commit/7e9a74c03917a60c0940a0db6ceae2ab5e23e120) | Fix external libraries that were added as git submodules |
| 2021&#8209;03&#8209;15 | [map-geom](https://github.com/CSProDevelopment/cspro/commit/78fdacfe088c728c99264e06fd01e79e03a00737) | Geojson & polygon capture support |
| 2021&#8209;03&#8209;08 | [DataViewer-WebView2](https://github.com/CSProDevelopment/cspro/commit/ef347e261dc5c5c5337222821f0805c990237cab) | modified Data Viewer to use the WebView2 control + allowed audio items to be played (just as images are displayed) |
| 2021&#8209;03&#8209;03 | [move-case-objects-from-zDataO-to-zCaseO](https://github.com/CSProDevelopment/cspro/commit/48a70b2c7afec79764cc144bde5fe15b0cd32078) | moved case-related objects from zDataO to zCaseO and the ConnectionString classes to zUtilO/zUtilCLR |
| 2021&#8209;03&#8209;03 | [cspro-exporter+data-viewer-exporter](https://github.com/CSProDevelopment/cspro/commit/8e42b344a3105bab49ca45d75689ccd4cef66988) | added an Export Data feature to Data Viewer |
| 2021&#8209;03&#8209;01 | [audio-widget](https://github.com/CSProDevelopment/cspro/commit/7c16a318485b4d83416acbafd935ca0042e6286c) | Audio widget Android implementation |
| 2021&#8209;03&#8209;01 | [sync-binary-data-taskb](https://github.com/CSProDevelopment/cspro/commit/6dcbb186009904a8386f4b6cb3d21b6deaa24546) | sync-binary-data-taskb: |
| 2021&#8209;02&#8209;26 | [r-export](https://github.com/CSProDevelopment/cspro/commit/fd3797b3b41382e24b8eff2f82db18c030d5bac4) | added a R export writer to natively write .Rdata files |
| 2021&#8209;02&#8209;26 | [sync-binary-data-task1](https://github.com/CSProDevelopment/cspro/commit/0d5c07e26b7b356cd15260feb9fae6c25250ade9) | sync-binary-data-task1: |
| 2021&#8209;02&#8209;26 | [trim-signature](https://github.com/CSProDevelopment/cspro/commit/60587bad9de52870af15618817586d8801b3e76f) | Trim signature bitmap to only area with drawing |
| 2021&#8209;02&#8209;26 | [signature-widget](https://github.com/CSProDevelopment/cspro/commit/bbc356f4e8b4c792359202160f7f0686588106f6) | Initial implementation of signature widget on Android |
| 2021&#8209;02&#8209;23 | [document-image-audio-integration](https://github.com/CSProDevelopment/cspro/commit/e471b5a4d1cc6f393e01be1e51f7d926d3294f1a) | allowed valid assignments between audio, document, and image objects |
| 2021&#8209;02&#8209;23 | [document-object](https://github.com/CSProDevelopment/cspro/commit/a9339d7de64c0156e067283598c780d048afd7b6) | added the document logic object for loading/saving/viewing documents |
| 2021&#8209;02&#8209;22 | [export-framewokr-excel](https://github.com/CSProDevelopment/cspro/commit/e50682309a800109ef015f13e103d08da1a171a7) | Export framewokr excel |
| 2021&#8209;02&#8209;22 | [hookup-photo-object-android](https://github.com/CSProDevelopment/cspro/commit/e24a709a4ec1b582ff92a3ec6b67fe427195c8fa) | Android/JNI side of the image object methods to take photo and capture signature. |
| 2021&#8209;02&#8209;19 | [android-photo-widget](https://github.com/CSProDevelopment/cspro/commit/4dd1cb9d1528bfe428f923c2ec8548f991e07172) | Android photo widget |
| 2021&#8209;02&#8209;19 | [image-object](https://github.com/CSProDevelopment/cspro/commit/b9e8e5cffcee052f08f011891d23712b93e4508b) | added the image logic function for loading/saving/viewing/resampling images and taking photos and capturing signatures |
| 2021&#8209;02&#8209;18 | [export-framework](https://github.com/CSProDevelopment/cspro/commit/c7ebc0f550a4f725cda452b1f131cad21bff1e49) | added the framework for Case-based export routines + add the export writers for comma/semicolon/tab-delimited files |
| 2021&#8209;02&#8209;18 | [preserveLegacyExternalStorage](https://github.com/CSProDevelopment/cspro/commit/fd4299258de0bc2e3f25ff14d0b0cd9a772a406b) | Fix issues with csentry directory on upgrade |
| 2021&#8209;02&#8209;17 | [wrong-csentry-after-backup](https://github.com/CSProDevelopment/cspro/commit/347fc853c9618d9f355f8cb94912a5b89f1a2c47) | Fix unable to write to csentry dir when old path backed up |
| 2021&#8209;02&#8209;17 | [data-repository-case-logic-functions](https://github.com/CSProDevelopment/cspro/commit/aa7c41b976a73f57f5ab3f0404ddfac5da12c4df) | refactored the case loading functions to work with datasource and case objects + added the currentkey function |
| 2021&#8209;02&#8209;11 | [datasource-object](https://github.com/CSProDevelopment/cspro/commit/b38a128f146949fe28950ba2a4c5c29aa68c368f) | added a new logic object, datasource, that allows for multiple data repositories associated with the same dictionary |
| 2021&#8209;02&#8209;10 | [dictionary-editor-fixes](https://github.com/CSProDevelopment/cspro/commit/cfce4d57b4c6a40ef40c5186d1f2bdf797cc9bfb) | prevented binary items from having value sets + fixed some other issues in the dictionary editor |
| 2021&#8209;02&#8209;10 | [case-object](https://github.com/CSProDevelopment/cspro/commit/2e5a62b505588464d9931b36c95e08ac0b2f62aa) | added a new logic object, case, that allows for multiple versions of a dictionary's cases in a program |
| 2021&#8209;02&#8209;09 | [EngineDictionary-introduction](https://github.com/CSProDevelopment/cspro/commit/23c483ac97742545bcf08f9f52c4b6c0870c9cff) | introduced the EngineDictionary symbol and its related classes |
| 2021&#8209;02&#8209;09 | [android-cleanup](https://github.com/CSProDevelopment/cspro/commit/c535f3fb2c672fe85b9c6ba41a4f5cc0c418510c) | Android code cleanup |
| 2021&#8209;02&#8209;09 | [binary-items-csdb](https://github.com/CSProDevelopment/cspro/commit/ec2f629ebbcce3b439bbd7927405c56165eeff48) | Store binary items in csdb file |
| 2021&#8209;02&#8209;08 | [EngineRecord-introduction](https://github.com/CSProDevelopment/cspro/commit/f910752184152e838f5938f72cf8ec2ea036bb1a) | introduced the EngineRecord symbol |
| 2021&#8209;02&#8209;05 | [subitem-check-crash](https://github.com/CSProDevelopment/cspro/commit/275c8bf2f0093e59f4de09b3e1bd58e9a2793d16) | Fix issues with binary types in dict editor |
| 2021&#8209;02&#8209;04 | [binary-data-items-in-tools](https://github.com/CSProDevelopment/cspro/commit/bf0c04f1a4f8989307b81655b2840418b23a8fdc) | added support for BinaryDataItems to CSDiff, CSReFmt, and CSSort + fleshed out the BinaryDataReader + added support for documents/images to the Generate Random Data File function |
| 2021&#8209;02&#8209;03 | [content-types-dict-editor](https://github.com/CSProDevelopment/cspro/commit/1fa428ed573e04bcf3d2b919fa0413a976b0e247) | Support for new binary types in dictionary editor |
| 2021&#8209;02&#8209;02 | [data-viewer-binary-data-items](https://github.com/CSProDevelopment/cspro/commit/137ebb23f1b8fe985ae35481d8765afd03309bbb) | added to Data Viewer support for BinaryDataItems |
| 2021&#8209;01&#8209;29 | [binary-data-item-lazy-loading](https://github.com/CSProDevelopment/cspro/commit/b8fb4f780197248dae052fb6a6e9fc16a741bbff) | reworked the BinaryDataItem to only load data on demand |
| 2021&#8209;01&#8209;28 | [add-datatype+binary-capturetype+document+audio+image](https://github.com/CSProDevelopment/cspro/commit/661e4c32223c43f7aa6ddd6e937784741013e24b) | added the Binary DataType and the Document/Audio/Image CaptureTypes + renamed FileDataItem -> BinaryDataItem + added routines to issue warnings when using a repository that doesn't support a certain CaptureType |
| 2021&#8209;01&#8209;28 | [viewhtml-modeless](https://github.com/CSProDevelopment/cspro/commit/16792f67141b02ef53109e36bc1ba7beab0088d9) | Fix view(<htmlfile>) not showing content |
| 2021&#8209;01&#8209;28 | [DataType-to-enum-class](https://github.com/CSProDevelopment/cspro/commit/cf9d783cbd566d538da3f93c011c7029eeaee26c) | refactored DATA_TYPE into the enum class ContentType and added references to places where CDictItem::GetContentType needs to be handled when there are more than two ContentTypes |


## CSPro 7.6.2

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro7.6.2.exe](https://www.csprousers.org/downloads/cspro/cspro7.6.2.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro7.6.2_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro7.6.2_releasenotes.txt)

**Merged pull requests**:

| Date | Branch | Pull Request Message |
| --- | --- | --- |
| 2021&#8209;09&#8209;10 | [qsf-html-content-relative-path-fix](https://github.com/CSProDevelopment/cspro/commit/1e6dd500253cd3f24c6fc2210b689cd4566eee96) | (Android) Added class for resolving html content requests to assets outside of application directory |
| 2021&#8209;08&#8209;02 | [android11-systemapp-bug](https://github.com/CSProDevelopment/cspro/commit/10ae64889b9e41632ddf01ac445873a981560004) | added the QUERY_ALL_PACKAGES permission so that SystemApp works on Android 11 |
| 2021&#8209;07&#8209;26 | [webview-file-access-bug-with-sdk30](https://github.com/CSProDevelopment/cspro/commit/5b0ea48fe6ec850bf03c227df01bbaa8299fe4b4) | fixed a bug whereby the Android web browser couldn't access files (with the switch to SDK 30) |
| 2021&#8209;07&#8209;01 | [barcode-addapplication](https://github.com/CSProDevelopment/cspro/commit/25213f51a88583ce4c88d2cf8b084b8cdc98d93b) | Barcode addapplication |
| 2021&#8209;07&#8209;01 | [startApp-crash](https://github.com/CSProDevelopment/cspro/commit/96eb17c9cf14c4c7fe22a23b95d6c389a5f67bcf) | fix for java.lang.NullPointerException in gov.census.cspro.csentry.ui… |
| 2021&#8209;06&#8209;29 | [closing-repository-in-loop-bug](https://github.com/CSProDevelopment/cspro/commit/e877d27e0bb5016a9e728662e3e5d3c4d17a5b40) | fixed a bug whereby the transaction started by a call to writecase/delcase was not ended when a data repository was closed in the loop |
| 2021&#8209;06&#8209;28 | [paradata-concat-not-closing-input-databases-bug](https://github.com/CSProDevelopment/cspro/commit/dce64909b899f9763689f49ea343edcf15ad5b2e) | fixed a bug whereby the input databases to a paradata concatenation operation were not properly closed |
| 2021&#8209;06&#8209;24 | [7.6.2-prep](https://github.com/CSProDevelopment/cspro/commit/86b51f49804d935a09403d8766b7c3697a83256d) | marked the code for a 2021-06-24 CSPro 7.6.2 release |
| 2021&#8209;06&#8209;15 | [casetree-bug](https://github.com/CSProDevelopment/cspro/commit/c91a8c8ef60db8c6823712ae463c9c087cdaf5ed) | Casetree bug |
| 2021&#8209;06&#8209;15 | [casetree-bug](https://github.com/CSProDevelopment/cspro/commit/78b4b514ea26b43a70980bcc25faff2bf6a80bfd) | Casetree bug |
| 2021&#8209;06&#8209;01 | [multiple-field-properties-uppercase-denominator](https://github.com/CSProDevelopment/cspro/commit/ba7caf0d8ef067e73f952cec6f95c346e2cbd257) | modified the denominator of the upper case tri-state checkbox to only look at alpha fields |
| 2021&#8209;05&#8209;28 | [grid-cell-delete-crash](https://github.com/CSProDevelopment/cspro/commit/5fa64bcbad225cd7fdf117b9220fedb1e968ec44) | fixed a crash that occurred when deleting an item from a roster by clicking on the grid cell and pressing delete |
| 2021&#8209;05&#8209;17 | [webview2-bug-with-execpff-wait](https://github.com/CSProDevelopment/cspro/commit/0af28a79582cc2be0910f15da84acbea1e4869a4) | fixed Webview2 hanging issue caused by using WaitForSingleObject (which is used by execpff with the wait property) |
| 2021&#8209;05&#8209;17 | [select-with-workstring-bug](https://github.com/CSProDevelopment/cspro/commit/1d1e2e120c14180f4b72f3338fa2f2fb7afb6fcb) | fix to errmsg with select with an alpha/string variable (which no longer worked following the introduction of the WorkString symbol) |
| 2021&#8209;05&#8209;17 | [remove-unnecessary-qsf-loading](https://github.com/CSProDevelopment/cspro/commit/5e2ca632d3505e723def4afa567630eec87aa1c1) | stopped restarting the QSF server when dictionaries are dropped from an application + removed code in BuildQuestMgr that unnecessarily built the symbol table |
| 2021&#8209;05&#8209;04 | [invalid-color-table-bug](https://github.com/CSProDevelopment/cspro/commit/13410537c69e7eafb62162edb9b7803f2665752b) | fixed a bug with the RTF->HTML conversion when an invalid color table entry is used |


## CSPro 7.6.1

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro7.6.1.exe](https://www.csprousers.org/downloads/cspro/cspro7.6.1.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro7.6.1_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro7.6.1_releasenotes.txt)

**Merged pull requests**:

| Date | Branch | Pull Request Message |
| --- | --- | --- |
| 2021&#8209;04&#8209;20 | [7.6.1-prep](https://github.com/CSProDevelopment/cspro/commit/89c8220c88069fd40e38ef626bb4dc4a49f3d641) | marked the code for a 2021-04-20 CSPro 7.6.1 release |
| 2021&#8209;04&#8209;15 | [textviewer-open-order](https://github.com/CSProDevelopment/cspro/commit/31d6cc83db3b0ddbae549e2a4eb6ebd762f699ed) | modified the loading of files as Text Viewer starts up to ensure that the order of files opened via inter-app messaging is preserved |
| 2021&#8209;04&#8209;15 | [capi-bug-fixes](https://github.com/CSProDevelopment/cspro/commit/b4063890e0158a2774830948593ce7af6a4e4b86) | fixed a CAPI bug related to modifying/deleting the language name and fixed the warning when not using () in function names in CAPI fills |
| 2021&#8209;04&#8209;14 | [dictionary-macro-range-bug](https://github.com/CSProDevelopment/cspro/commit/2a4c2ae485fda36de4db2ceacce99a4de45f2a45) | fixed Dictionary Macros random data generation bugs related to range values and cases generated with no records |
| 2021&#8209;04&#8209;13 | [impute-item-with-parent-item-bug](https://github.com/CSProDevelopment/cspro/commit/5c02d56d85a13b1b3b41acae93d580b7995867cf) | fixed bug whereby parent items were not properly updated when a child item was imputed |
| 2021&#8209;04&#8209;13 | [bug-Pressing-backspace-does-not-work](https://github.com/CSProDevelopment/cspro/commit/9cffc5c2971380ab59c99428611583780f776cdf) | Pressing backspace does not work to delete the last character of an alphanumeric |
| 2021&#8209;04&#8209;13 | [file-handles-in-listing-bug](https://github.com/CSProDevelopment/cspro/commit/ed064154d0aa5082b7cb75f6187477687e5e6802) | fixed a bug whereby the PFF-defined (logic) file associations did not display correctly in the listing report |
| 2021&#8209;04&#8209;05 | [userbar-bytecode-bug](https://github.com/CSProDevelopment/cspro/commit/13c54e1dc3c6242e94749fcba8a19c8d40eb8171) | fixed a bug whereby the bytecode of userbar functions with no arguments was improperly generated |


## CSPro 7.6.0

**Installer**: [https://www.csprousers.org/downloads/cspro/cspro7.6.0.exe](https://www.csprousers.org/downloads/cspro/cspro7.6.0.exe)

**Release notes**: [https://www.csprousers.org/downloads/cspro/cspro7.6.0_releasenotes.txt](https://www.csprousers.org/downloads/cspro/cspro7.6.0_releasenotes.txt)

**Merged pull requests**:

| Date | Branch | Pull Request Message |
| --- | --- | --- |
| 2021&#8209;04&#8209;03 | [release/7.6.0](https://github.com/CSProDevelopment/cspro/commit/f8b68c375573c7a1dceb3998c8baa77bbb384c43) | release/7.6.0 |
| 2021&#8209;03&#8209;19 | [7.6.0-markings](https://github.com/CSProDevelopment/cspro/commit/6738dba9981972bd7df6b637a12418d43ebe4216) | marked the code for a 2021-03-31 CSPro 7.6.0 release |
| 2021&#8209;03&#8209;17 | [bluetooth-server-read-only](https://github.com/CSProDevelopment/cspro/commit/6f21bd506ecec18d4627852dd5f8265537cf0d40) | Update questionnaireSerializer when syncing read-only repo |
| 2021&#8209;03&#8209;08 | [suppress-altitude-in-paradata](https://github.com/CSProDevelopment/cspro/commit/329b39529aac270901c240c07c2d2e0bba2f75f2) | suppressed storing GPS/altitude in the paradata log when partially collecting GPS data |
| 2021&#8209;03&#8209;08 | [data-access-initialization-timing](https://github.com/CSProDevelopment/cspro/commit/3b2ff04cd8c926b82ff93beb28b2a87d24731080) | modified the timing of initializing each dictionary's data access so that it occurs following the PFF being filled |
| 2021&#8209;03&#8209;02 | [excel2cspro-ignore-invalid-values-when-analyzing-worksheets](https://github.com/CSProDevelopment/cspro/commit/ed041767fe67396278db28cadd07210d68e7f00d) | ignored invalid Excel cells (such as #N/A) when analyzing a dictionary in Excel to CSPro |
| 2021&#8209;02&#8209;17 | [keylist-dictionary-access-and-modified-buffer-bugs](https://github.com/CSProDevelopment/cspro/commit/c307a7db90c3434acb0a77fff37dbf034e4af11d) | fixed bugs with keylist not obeying the dictionary access parameters and the logic buffer not being marked as modified when doing things like cutting/pasting/clearing text |
| 2021&#8209;02&#8209;17 | [installer-check-dotnet](https://github.com/CSProDevelopment/cspro/commit/6aafd4bca1f5a85be68e0a29d843a9c8a0862744) | Check for .NET framework in installer |
| 2021&#8209;02&#8209;10 | [extended-controls-to-capture-type](https://github.com/CSProDevelopment/cspro/commit/fae584e687012bb81e8f567eb51a23d9b471f47b) | changed places in the forms designer using "Capture Type" to instead use "Extended Controls" + updated the release date |
| 2021&#8209;02&#8209;08 | [html-frequencies](https://github.com/CSProDevelopment/cspro/commit/c57017b6d55ef17354a96c5d3ce09e73febaf650) | Html frequencies |
| 2021&#8209;02&#8209;08 | [bug-html-lister](https://github.com/CSProDevelopment/cspro/commit/83192eeb5a899f92ea4acf309b64e3a5a2bc132c) | secondary attribute is no longer an independent column. instead prepe… |
| 2021&#8209;02&#8209;04 | [bug-html-lister](https://github.com/CSProDevelopment/cspro/commit/83cfe8a2b7a8800983280debf09d3991b43a954e) | left justify text and right justify numerics for html lister |
| 2021&#8209;02&#8209;04 | [freq-in-file-associations-dialog-only-when-applicable](https://github.com/CSProDevelopment/cspro/commit/c4d85ffa57ea4174a3402822598f75ac9ef749d9) | suppressed showing "Freq File" in the File Associations dialog unless using unnamed frequencies or freq_name.save() without an argument |
| 2021&#8209;01&#8209;29 | [1961/webview-iframe-src](https://github.com/CSProDevelopment/cspro/commit/08ab4e9b94683f318d757baf3c7a0e147630d9ce) | Don't force iframe src to open outside webview |
| 2021&#8209;01&#8209;29 | [startup-lang-crash](https://github.com/CSProDevelopment/cspro/commit/361e5abd9ecc469fda6a811ba48595729c7d6abe) | Fix initial current lang index in qsf |
| 2021&#8209;01&#8209;28 | [viewhtml-modeless2](https://github.com/CSProDevelopment/cspro/commit/eca9f5b95729d7f0482bbdcd1f4f2734a9d35c22) | Fix view(<htmlfile>) not showing content |
| 2021&#8209;01&#8209;28 | [1922/reconcile-qsf-dict-name](https://github.com/CSProDevelopment/cspro/commit/7d42e53f6934359daa89cea9f77d345a8c517035) | Reconcile dict name change in QSF file |
| 2021&#8209;01&#8209;27 | [1943/capi-err-lang](https://github.com/CSProDevelopment/cspro/commit/4006123734aa833543319640c97d74c5faeb1267) | Double click on CAPI text error in other language |
| 2021&#8209;01&#8209;27 | [block-qsf-text-warning-bug](https://github.com/CSProDevelopment/cspro/commit/1997fc6cffc4596f36cb362a6ad69b78bfe2efe1) | fixed a bug whereby the warning about missing parentheses in function names in question text didn't show up properly for blocks |
| 2021&#8209;01&#8209;27 | [block-bug](https://github.com/CSProDevelopment/cspro/commit/6464fe3d06062b50dce7e6bebe7887731a44431f) | fix dictionary name change crash related to  blocks |
| 2021&#8209;01&#8209;27 | [frequency-modifications-during-documentation](https://github.com/CSProDevelopment/cspro/commit/71abf55be5f5a3728aa7ea63b2e95194a8e6115d) | fixed some frequency issues discovered while documenting the new frequency functionality |
| 2021&#8209;01&#8209;27 | [1942/only-del-web-stuff-on-uninstall](https://github.com/CSProDevelopment/cspro/commit/2dcebc7d424418a1938407bb6b9344a6063c222f) | only delete webview cache on uninstall not all of appdata |
| 2021&#8209;01&#8209;27 | [hang-designer-close-entry](https://github.com/CSProDevelopment/cspro/commit/691fb69461d81f045081c8eee50cdccbe0881218) | Separate webview2 data path for cspro & csentry |
| 2021&#8209;01&#8209;26 | [1939/dup-css-classname](https://github.com/CSProDevelopment/cspro/commit/81d9f52af50a845124b5b33e21a3236ee87d34cf) | Prevent dup class names in edit styles |
| 2021&#8209;01&#8209;26 | [capi-fills-evaluation-post-movement-request-bug](https://github.com/CSProDevelopment/cspro/commit/3ea81d83b1d13c63e08acbd08ac178417d8b456c) | fixed a bug whereby CAPI fills weren't evaluated properly following a movement statement |
| 2021&#8209;01&#8209;26 | [fully-evaluate-path-when-using-signature](https://github.com/CSProDevelopment/cspro/commit/2e8788bc3b91fa3762cdd958e6cace0cb66d5a5f) | modified execsystem/signature to fully evaluate the path (as occurs for camera/view) + fixed a bug with the signature not being saved properly when using an overlay (due to the filename being clipped) |
| 2021&#8209;01&#8209;26 | [csentry-closing-case-listing-lock](https://github.com/CSProDevelopment/cspro/commit/cabc125798774b33a4544aea13f5845cb867282a) | fixed bug whereby CSEntry didn't close properly when the Lock=CaseListing PFF flag was set |
| 2021&#8209;01&#8209;26 | [qsf-style-dlg-fixes](https://github.com/CSProDevelopment/cspro/commit/57bcf3c14bb54cdbcdfdf3bfed25cb1d802a6595) | Fix issues with edit styles dialog |
| 2021&#8209;01&#8209;26 | [capi-options-menu](https://github.com/CSProDevelopment/cspro/commit/aab7ce0a0668b814ffcadc3a8e4f47c93c293977) | renamed the "CAPI Options" menu to "CAPI" + reverted the "d7617bb" change (related to the Help Text menu item), instead removing that menu item from the CAPI menu |
| 2021&#8209;01&#8209;26 | [1921/reconcile-capi-text-rename-lang](https://github.com/CSProDevelopment/cspro/commit/13716d8e66d7638b3db966e1f84da573379440ba) | Update questions when language is renamed |
| 2021&#8209;01&#8209;25 | [always-check-capi-logic-for-syntax-errors-on-run](https://github.com/CSProDevelopment/cspro/commit/e908453ab55a67eaa19ed67249a29fce431dc3bd) | fixed bug whereby CAPI question text wasn't syntax checked when launching a data entry application from the designer (when not in logic view) |
| 2021&#8209;01&#8209;25 | [hide-image-resource-checkbox-if-no-application-open](https://github.com/CSProDevelopment/cspro/commit/7a7e3d24a92407329f2369c76b1968493c7a4e45) | hid the "include in pen file as image resource" checkbox if no application is open |
| 2021&#8209;01&#8209;25 | [ellipses-in-menus](https://github.com/CSProDevelopment/cspro/commit/ba5e2a7746ae78ce63cab164aebf27e24b7c1eca) | fixed menu item ellipses issues + the check state of the CAPI Options -> Help Text menu item |
| 2021&#8209;01&#8209;25 | [qsf-editor-misc](https://github.com/CSProDevelopment/cspro/commit/e114877a7297479d7e155351833cb1749201ce06) | QSF editor fixes |
| 2021&#8209;01&#8209;25 | [suppress-case-construction-reporter-when-not-ouputting-cases](https://github.com/CSProDevelopment/cspro/commit/52452e576f2e7ee51ad9e607872c114d6ee23401) | fixed bug whereby the case construction reporter improperly issued errors in the output case even when the output case wasn't being saved |
| 2021&#8209;01&#8209;25 | [csfreq-refactor](https://github.com/CSProDevelopment/cspro/commit/74126cc1a3c239b0dd46d989a435ba59d0ac4682) | reworked CSFreq to use the new frequency generators + added some additional options (percentiles, sorting, and output type) |
| 2021&#8209;01&#8209;25 | [1912/intermittent-barcode-failure](https://github.com/CSProDevelopment/cspro/commit/56ee72e53d2bfca6ebadeca4b290b34879587b64) | Fix intermittent barcode failures |
| 2021&#8209;01&#8209;22 | [freq-review-fixes](https://github.com/CSProDevelopment/cspro/commit/c9cb5d263aae01308bc2ed923dde33dee0c7e57f) | fixed frequencies on multiply-occurring items to match the 7.5 behavior + modified the text frequencies to match the PFF's listing width |
| 2021&#8209;01&#8209;22 | [create-pen-hang](https://github.com/CSProDevelopment/cspro/commit/bac4cec71453fe7fe99fb2eeaedd6eb0d77cedae) | Fix hang creating pen files from designer |
| 2021&#8209;01&#8209;21 | [biswa/barcode_crash_76](https://github.com/CSProDevelopment/cspro/commit/452f88a223c6cbf54f188bfac645a3e7e6d1d106) | Crash fix for IllegalStateException: |
| 2021&#8209;01&#8209;21 | [pre74-case-reset-bug](https://github.com/CSProDevelopment/cspro/commit/54aa164bb4a88a1c9d608ba1a754ab33fba842c7) | fixed a bug in which the 7.4 case object wasn't reset as part of Case::Reset(), which led to bad behavior when using special output dictionaries |
| 2021&#8209;01&#8209;21 | [review-76](https://github.com/CSProDevelopment/cspro/commit/3a7ffd63a82e8c4d76835a772646796f16171a6f) | readied the code for a 7.6.0 release + made some small modifications upon 7.6 testing |
| 2021&#8209;01&#8209;19 | [csfreq-csexport-standardize](https://github.com/CSProDevelopment/cspro/commit/b1c53da172f190c34e32b91e012a743101ef3c3e) | added the View Batch Logic option to CSFreq and moved the Save Excluded Items option to the menu + standardized some behavior in CSExport/CSFreq (used the logic control for the CSFreq/CSExport universe editors + used the dictionary icons located in zInterfaceF) |
| 2021&#8209;01&#8209;18 | [batch-logic-viewer-dialog](https://github.com/CSProDevelopment/cspro/commit/c5e796376ef557fc92a8ad631151069333539a6c) | created a dialog for viewing batch logic with options to copy the logic to the clipboard or create a new batch application based on the logic (and implemented this on CSExport) |
| 2021&#8209;01&#8209;18 | [remove-xml-metadata-features](https://github.com/CSProDevelopment/cspro/commit/ebaad46a0111e100d0d2e84a93af4c26bcadf67b) | removed the ExportXMLMetadata tool and the XML Metadata export features |
| 2021&#8209;01&#8209;15 | [1898/map-case-listing](https://github.com/CSProDevelopment/cspro/commit/02e4f03875757ae7e49befb692e30bc2796e771a) | Fix map case listing empty case bug |
| 2021&#8209;01&#8209;14 | [capi-text-unicode](https://github.com/CSProDevelopment/cspro/commit/00841fec2f88a7f9949d01ed050dfceb4858efaa) | support non-ascii code page in RTF2html |
| 2021&#8209;01&#8209;14 | [centralize-message-editor](https://github.com/CSProDevelopment/cspro/commit/6284ac3b291e8f4bd2fe767aada5f9a69b1b7bc9) | created a common message editor class to use in the form/batch/table editors |
| 2021&#8209;01&#8209;14 | [1569/opt-comma-gen-vset](https://github.com/CSProDevelopment/cspro/commit/a1674e979921569f5de82d8f135422f778c890b0) | Make thousands separator optional in generate value set |
| 2021&#8209;01&#8209;14 | [1121/cssort-listing-warnings](https://github.com/CSProDevelopment/cspro/commit/5bdc2b0d70a37fa9e9f829488002ff7e7565f36e) | Show listing file in CSSort if there are warnings like too many record bccs |
| 2021&#8209;01&#8209;13 | [js-qsf-editor](https://github.com/CSProDevelopment/cspro/commit/1f1b59d0a217127ae4f271c60700208795d8b594) | Remove filter in QSF editor that deletes Javascript |
| 2021&#8209;01&#8209;13 | [clear-qsf-text-on-hide](https://github.com/CSProDevelopment/cspro/commit/a12d87e217081a95c0eece2904e23b909d0f48be) | Clear QSF webviews when hiding |
| 2021&#8209;01&#8209;13 | [trace-handler-to-zEngineF](https://github.com/CSProDevelopment/cspro/commit/5e6b23b40805588bba55cb49080f67942d4cbf30) | moved the trace handler to the zEngineF DLL |
| 2021&#8209;01&#8209;13 | [camera-with-msg](https://github.com/CSProDevelopment/cspro/commit/d87f8642627a4c0d7fee2e60a757f3a1241b4630) | Fix off by 1 error in execsystem camera |
| 2021&#8209;01&#8209;13 | [1615/update-case-tree-lang-change](https://github.com/CSProDevelopment/cspro/commit/c56e01126a172e3b041e60be653237a6afe8d5c5) | Update case tree on change language |
| 2021&#8209;01&#8209;13 | [windows-messaging-cleanup](https://github.com/CSProDevelopment/cspro/commit/096baff12e944dbd037d4a9f15a41a422d7986a9) | cleaned up most of the solution's Windows messaging |
| 2021&#8209;01&#8209;12 | [1729/include-csds-in-folder-deploy](https://github.com/CSProDevelopment/cspro/commit/2aae84c0270e3d5217545e105a009808cf7dbd1a) | include package spec in local folder deploy |
| 2021&#8209;01&#8209;12 | [fix-CSProRuntime-Messages-Processor](https://github.com/CSProDevelopment/cspro/commit/a6ce86faaedb1f45c477b89b229b51637b25fe02) | modified the CSProRuntime Messages Processor to properly use a TextSource when loading the message files |
| 2021&#8209;01&#8209;12 | [fix-qsf-edit-release](https://github.com/CSProDevelopment/cspro/commit/f173cdbe1bca2d57ec89500eb3c688b137d4270e) | Fix qsf editor in release build |
| 2021&#8209;01&#8209;12 | [centralize-compiler-output](https://github.com/CSProDevelopment/cspro/commit/8db5015678d612c86fc24e0f931745d164402549) | created a common compiler output class to use in the form/batch/table editors |
| 2021&#8209;01&#8209;12 | [warnings-to-errors](https://github.com/CSProDevelopment/cspro/commit/aac9b06c3bb3194a6dc8f943a3ba0f7842e32f02) | turned warnings (4005, 4100, 4189, 4702, 4715, 4840) into errors in many of the projects |
| 2021&#8209;01&#8209;12 | [warning-C4005](https://github.com/CSProDevelopment/cspro/commit/fabafd09afa5ba0c2bbb2a484e19e58a32a1dbcb) | turned warning 4005 (macro redefinition) into an error + removed all inclusions of resource.h from any header file other than stdafx.h + made sure that all dialogs build with IDDs moved out of their header files |
| 2021&#8209;01&#8209;11 | [new-help-context-ids](https://github.com/CSProDevelopment/cspro/commit/592201885db4c7bbfd2c8ecffe0c407ff27f1b6c) | stopped having projects use makehm to generate HTMLDefines.h files (now that the Help Generator reads resource.h files directly) |
| 2021&#8209;01&#8209;11 | [update-app-to-apps-list](https://github.com/CSProDevelopment/cspro/commit/f830561736b45b7671bbe943a113690e52b7a854) | After update app return to apps list |
| 2021&#8209;01&#8209;11 | [zFormF-resources](https://github.com/CSProDevelopment/cspro/commit/1069d19d878818b10f6bb6018faddf55e8169c45) | cleaned up zFormF's resources |
| 2021&#8209;01&#8209;11 | [centralize-css](https://github.com/CSProDevelopment/cspro/commit/ff07f406d845beb96b69bbd38b2737b168e9e811) | Centralize css |
| 2021&#8209;01&#8209;11 | [fix-android-build](https://github.com/CSProDevelopment/cspro/commit/2b745c7cb78cc913f75f7032a002c8f98e5619de) | Fix Android build |
| 2021&#8209;01&#8209;11 | [missing-capi-text-behavior](https://github.com/CSProDevelopment/cspro/commit/bbba893f0f63539330d43e944322a8d2c64bdf2f) | Missing capi text behavior |
| 2021&#8209;01&#8209;08 | [remove-auto-compile](https://github.com/CSProDevelopment/cspro/commit/58517f53c6dc49b994f5e74cfec80a224ea8e14f) | removed the "Auto Compile" setting (which was never fully implemented) |
| 2021&#8209;01&#8209;08 | [zTableF-resources](https://github.com/CSProDevelopment/cspro/commit/e30c2a173c39e24ae6a2e19e544f4b622cfe0d83) | cleaned up zTableF's resources |
| 2021&#8209;01&#8209;07 | [fix-capi-text-margin](https://github.com/CSProDevelopment/cspro/commit/cc6bc5161159bfd6f65581b63a82e2651b634923) | Remove margin in html in qsf webview |
| 2021&#8209;01&#8209;07 | [increase-default-capi-font-size](https://github.com/CSProDevelopment/cspro/commit/18cd2a2206ee44bbefec66c3ae73e182a0b8c5cd) | Increase default CAPI font size |
| 2021&#8209;01&#8209;07 | [biswa/JavaToKotlinPhase2b](https://github.com/CSProDevelopment/cspro/commit/496ce7f182f529f2c3910fc2f7b9f48d3618f79e) | Biswa/java to kotlin phase2b |
| 2021&#8209;01&#8209;07 | [biswa/1822-casetreeTablet](https://github.com/CSProDevelopment/cspro/commit/16082f0466289c5fc9409432a4a5677c40c54ee0) | Biswa/ Running Android for the first time in a while on  tablet |
| 2021&#8209;01&#8209;07 | [portable-occurrence-labels](https://github.com/CSProDevelopment/cspro/commit/8a30033204980127fa8e5f80ad75b27adefdcaf1) | fixed the new occurrence label evaluation to work in the portable environment |
| 2021&#8209;01&#8209;07 | [fix-fill-fix](https://github.com/CSProDevelopment/cspro/commit/a6f024ddf0eeb9b3653a932f3984e23ded9f1ecc) | Fix fill fix that would convert all to lower |
| 2021&#8209;01&#8209;07 | [pre76-fill-fixes](https://github.com/CSProDevelopment/cspro/commit/30798ad77c108e0af8ed2cda220388e1e8cbe44d) | Pre76 fill fixes |
| 2021&#8209;01&#8209;07 | [getocclabel-fixes](https://github.com/CSProDevelopment/cspro/commit/15377440c5d4254e83b7dd7f9fcb07ddc42998ed) | allowed getocclabel to be called without an argument (to give the occurrence label for the current group) |
| 2021&#8209;01&#8209;06 | [comp-capi-when-compiling](https://github.com/CSProDevelopment/cspro/commit/0a18e9dc69bb0d104c5d7fc1fcba7bec479a5037) | Compile capi fills/conditions when compiling and other CAPI text fixes |
| 2021&#8209;01&#8209;06 | [html-frequencies](https://github.com/CSProDevelopment/cspro/commit/36d237e132dac454e3a9132cbe26bc66c17714ad) | use first value to determine justification |
| 2021&#8209;01&#8209;06 | [excel-frequencies](https://github.com/CSProDevelopment/cspro/commit/e0d6b9f97a862fe00a4f08d060260c3fc1bb17d9) | dynamically justify value column |
| 2021&#8209;01&#8209;05 | [remove-logic-control](https://github.com/CSProDevelopment/cspro/commit/9d99eb2ccf7187a22d37a92df25c5dfdd14c4a1c) | removed CDEGroup's "Logic Control" property |
| 2021&#8209;01&#8209;05 | [zOrderF-resources](https://github.com/CSProDevelopment/cspro/commit/4dbc7bbd3d8844e1654865a8f2f2075b7f732acd) | reworked some of zOrderF's resources + added the zDesignerF DLL to centralize classes defined in triplicate (zFormF/zOrderF/zTableF) + added the logic reference window and dialog bar to that DLL |
| 2021&#8209;01&#8209;05 | [add-block-reconcile-crash](https://github.com/CSProDevelopment/cspro/commit/8d39c2533671252e10ac0719d2eaf439e67998ed) | Fix crash adding block in designer |
| 2021&#8209;01&#8209;04 | [qsf-table-style-android](https://github.com/CSProDevelopment/cspro/commit/e2d8c537d36dcf794a496db8968042049819ea10) | Fix qsf table style on Android |
| 2021&#8209;01&#8209;04 | [fix-cvt-old-capi-text-android](https://github.com/CSProDevelopment/cspro/commit/3512c2a3ca2fecc5ac627304e119adace7863f71) | Fix convert pre76 CAPI text on Android |
| 2021&#8209;01&#8209;04 | [more-capi-edit-fix](https://github.com/CSProDevelopment/cspro/commit/d147cbad9f87b5687953c347dfb04f958da27668) | More capi edit fixes |
| 2021&#8209;01&#8209;04 | [biswa/JavaToKotlinPhase2aUI](https://github.com/CSProDevelopment/cspro/commit/db265596b327c60b678de2fdbd1f36edb4afe487) | Biswa/Java to kotlin phase2a UI |
| 2021&#8209;01&#8209;04 | [linked-value-set-accelerator-key](https://github.com/CSProDevelopment/cspro/commit/50444f3ffd73f7e58a39567aa7bcff4583cabc31) | added Ctrl+Alt+V as a shortcut to paste value set links |
| 2021&#8209;01&#8209;04 | [wrap-qsf-edit-codeview](https://github.com/CSProDevelopment/cspro/commit/043eee9d948971b2ef2b758ad1c5db37e42c1b6f) | QSF Editor fixes |
| 2020&#8209;12&#8209;29 | [system-message-override-bug](https://github.com/CSProDevelopment/cspro/commit/8ed0f7471cfa6bdea10cc78334624cd3694bc45e) | fixed a bug whereby overridden system messages (not located in the CSPro installation directory) were not used when running a .pen file |
| 2020&#8209;12&#8209;24 | [arrow-keys-qsf-editor](https://github.com/CSProDevelopment/cspro/commit/159c130334ade83e9e09c48dc944e321d285af26) | Allow ctrl+arrow shortcuts in qsf editor |
| 2020&#8209;12&#8209;23 | [resource-id-numberer](https://github.com/CSProDevelopment/cspro/commit/5d4790c7f89b1ed52f30f824086fa8808ae266d6) | implemented the Resource ID Numberer build tool |
| 2020&#8209;12&#8209;23 | [zDictF-resources](https://github.com/CSProDevelopment/cspro/commit/172f2d912425f5b71287946ade33b665a55c5063) | reworked zDictF's resources and messages |
| 2020&#8209;12&#8209;22 | [rtf-convert-font-size](https://github.com/CSProDevelopment/cspro/commit/b8b9cb47c54cb0da08ae1f62e15051241a0f8811) | use px as font size unit in rtf2html |
| 2020&#8209;12&#8209;22 | [designer-resources-centralization](https://github.com/CSProDevelopment/cspro/commit/4060beca5646ada532301f4aa4b45433982d6c14) | centralized into the designer the resources defined in the zDictF/zFormF/zOrderF/zTableF projects |
| 2020&#8209;12&#8209;22 | [qsf-paste-without-format](https://github.com/CSProDevelopment/cspro/commit/1a0498aa976eb05ffa93619de532c3df5ca04d0d) | Paste without format in qsf editor |
| 2020&#8209;12&#8209;22 | [htmlview-right-click-accelerators](https://github.com/CSProDevelopment/cspro/commit/d975ae4482fe38ec756cff5148cb5cbe5ca2854c) | Disable browser context menu in html views and pass most accelerator keys to host program |
| 2020&#8209;12&#8209;21 | [qsf-accents](https://github.com/CSProDevelopment/cspro/commit/7a8c4ae9c7978ea963216e8d130097c549cdf6e5) | Correct encoding conversion when converting RTF qsf |
| 2020&#8209;12&#8209;21 | [csentry-resource-file-and-WM_IMSA_EXE-cleanup](https://github.com/CSProDevelopment/cspro/commit/5feeea932c24fb811bef41627852896d42b20eac) | Csentry resource file and wm imsa exe cleanup |
| 2020&#8209;12&#8209;21 | [codemirror](https://github.com/CSProDevelopment/cspro/commit/15faa99b3f26f86d3a9c8f57f9589a031345a80b) | Add codemirror to html editor for html syntax highlighting |
| 2020&#8209;12&#8209;19 | [freq-tab-bad-ext](https://github.com/CSProDevelopment/cspro/commit/14e510e6401721abc182303215a3020b610ce408) | Use correct extension for tab file in freq tool |
| 2020&#8209;12&#8209;18 | [capi-fill-syntax-check](https://github.com/CSProDevelopment/cspro/commit/0ef73d9d4ae23cc33ec50687ac39eaf88024326b) | Capi fill syntax check |
| 2020&#8209;12&#8209;18 | [biswa/JavaToKotlinPhase2](https://github.com/CSProDevelopment/cspro/commit/09e9374f3cdeafb5716bbd5bd6e9ef3904f88616) | Biswa/Java to kotlin phase2 |
| 2020&#8209;12&#8209;18 | [message-evaluation-caching-bug](https://github.com/CSProDevelopment/cspro/commit/55200da8d52beb9cd7690e2dfc315969882ea4fe) | fixed a bug where cached message formats were incorrectly used to evaluate messages in other languages |
| 2020&#8209;12&#8209;16 | [userbar-to-zEngineF](https://github.com/CSProDevelopment/cspro/commit/043b4631955a7e8456fb0564a51136f10cab9413) | refactored the userbar into zEngineF (and removed its dependency on resource IDs) and had the console and Android applications use a shared implementation |
| 2020&#8209;12&#8209;14 | [biswa/JavaToKotlin](https://github.com/CSProDevelopment/cspro/commit/aae4ce3b818600e56dd2efba5f94fd39d25592f6) | Porting of Java to Kotlin Conversion Phase 1 #1787 |
| 2020&#8209;12&#8209;14 | [biswa/gps_background_location_keep](https://github.com/CSProDevelopment/cspro/commit/3d83ae0f24afbfc0d26c5d633a2459ee9c1cbcf7) | Remove Background  Location while Supporting location reading from new Android build #1813 |
| 2020&#8209;12&#8209;11 | [resource-file-cleanup](https://github.com/CSProDevelopment/cspro/commit/d0c709157e19c19e2358a85595c088952e8edcc7) | cleaned up the resource files for all projects other than CSEntry/CSPro/zDictF/zFormF/zOrderF/zTableF |
| 2020&#8209;12&#8209;08 | [symbol-analysis](https://github.com/CSProDevelopment/cspro/commit/45b3e070a1e4008ea264fb7c9d929e017e7f9c41) | added Symbol Analysis, a feature to show where dictionary/form symbols and user-defined functions are used in logic |
| 2020&#8209;12&#8209;08 | [html-lister](https://github.com/CSProDevelopment/cspro/commit/15bd67d4698dedcd7456b0d545bbbb1d448d7ac4) | refactor multi-level application implementation using number of level… |
| 2020&#8209;12&#8209;07 | [html-lister](https://github.com/CSProDevelopment/cspro/commit/b07819485862c673462c5eec8c808e8d5098fcaf) | Html lister |
| 2020&#8209;12&#8209;07 | [clickable-dictionary-tree-control](https://github.com/CSProDevelopment/cspro/commit/3f4187568c3e4b4a8a0010a600b9089535646ab1) | added a new control to zInterfaceF, ClickableDictionaryTreeCtrl, to centralize dictionary tree interactions occurring in the tools + refactored CSDiff to use this control |
| 2020&#8209;12&#8209;03 | [biswa/slider_start_value](https://github.com/CSProDevelopment/cspro/commit/704ca21dbec6274862d6b524db548e20352e7169) | Change slider starting value to nothing. #1805 |
| 2020&#8209;12&#8209;02 | [backup-old-qsf](https://github.com/CSProDevelopment/cspro/commit/a5defff4db2338dff935fe1f1e7c003b8e8c160f) | Various HTML CAPI text fixes/improvements |
| 2020&#8209;11&#8209;30 | [universe-dialog-modernize](https://github.com/CSProDevelopment/cspro/commit/77f24baa982f860be507cc5660095c329903991d) | modified the Edit Universe dialog to use the Scintilla logic control |
| 2020&#8209;11&#8209;24 | [frequencies-excel-printer](https://github.com/CSProDevelopment/cspro/commit/3ae085acabc24801ff28195e795e05b8b65f5875) | added frequency printing to Excel files |
| 2020&#8209;11&#8209;24 | [overlapping-second-level-ids-bug](https://github.com/CSProDevelopment/cspro/commit/e65d26724670e90150ba2b9f7ac6d51cb61ed46b) | fixed bug whereby cases where the level 2+ ID overlaps with the first level ID were not properly read |
| 2020&#8209;11&#8209;23 | [frequencies-tbw-printer](https://github.com/CSProDevelopment/cspro/commit/b73104bec778d308c3b5ce7badbde33b50d024cd) | added the ability to print frequencies to table (.tbw) format |
| 2020&#8209;11&#8209;23 | [fix-qsf-modified](https://github.com/CSProDevelopment/cspro/commit/462bc270832c8a5b904b88d937437adaef8879a9) | Fix modified handling in qsf editor |
| 2020&#8209;11&#8209;23 | [qsf-insert-link](https://github.com/CSProDevelopment/cspro/commit/0bdcaca87c6d3859e26d48d95b8f5b82c238a7eb) | Add insert/edit link to qsf editor |
| 2020&#8209;11&#8209;23 | [installer-generator-zip-output](https://github.com/CSProDevelopment/cspro/commit/519536ad78515a0c088ae23eee9cd1d95e362a08) | added an option to the Installer Generator to create a zip file |
| 2020&#8209;11&#8209;20 | [create-pen-hang](https://github.com/CSProDevelopment/cspro/commit/fe06fdd25040ab8c0bb24f439a70471d7a64128e) | Second attempt to fix hang creating pen file from designer |
| 2020&#8209;11&#8209;20 | [qsf-table-toolbar](https://github.com/CSProDevelopment/cspro/commit/7367a780903d68743b8ec144d52fa41786dda278) | Add insert table to qsf editor |
| 2020&#8209;11&#8209;19 | [biswa/widget_slider](https://github.com/CSProDevelopment/cspro/commit/71503ab14ea6f7f4407b23c38307db73025703df) | Biswa/widget slider |
| 2020&#8209;11&#8209;19 | [biswa/barcode_npe_error](https://github.com/CSProDevelopment/cspro/commit/d396f8a2309d525d12486fd9ca359ad77a7c8f22) | Barcode crash fix |
| 2020&#8209;11&#8209;18 | [frequencies-imputations-and-distinct-and-valueset-flags](https://github.com/CSProDevelopment/cspro/commit/9b7543a8361d12f44dadcaa0173428d581baf142) | added "distinct" and "valueset" flags to the frequency command + refactored imputation frequency printing |
| 2020&#8209;11&#8209;18 | [qsf-delimiters](https://github.com/CSProDevelopment/cspro/commit/c38e4a9aef5bbfe711f19e338024a8fa0cd73096) | Qsf delimiters |
| 2020&#8209;11&#8209;17 | [frequency-printing](https://github.com/CSProDevelopment/cspro/commit/86002bd0bec3d0084f5da738fda5056dab36fc89) | refactored the creation of frequency tables (and statistics) and printing the tables to text format |
| 2020&#8209;11&#8209;17 | [install-webview2](https://github.com/CSProDevelopment/cspro/commit/16212dfeff116b8f31027ec1711d5aba1adf6074) | Added webview2 runtime to installer |
| 2020&#8209;11&#8209;16 | [qsf-cust-style](https://github.com/CSProDevelopment/cspro/commit/23578819024a872eabc8e53a5377c8f127f59e26) | Customize styles in CAPI text editor |
| 2020&#8209;11&#8209;13 | [biswa/camera_capture_new](https://github.com/CSProDevelopment/cspro/commit/ebded7e68e62f7069ed9d8e07d5f1d89ae5a3880) | Biswa/camera capture |
| 2020&#8209;11&#8209;09 | [html-qsf](https://github.com/CSProDevelopment/cspro/commit/c51627c4824a141cb1779af795f26abe57f7a564) | HTML CAPI question text |
| 2020&#8209;11&#8209;02 | [json-big-int](https://github.com/CSProDevelopment/cspro/commit/61eb91cab559f70590ca0794117024c8ba295205) | Fix bug syncing data with integers > 32 bits |
| 2020&#8209;11&#8209;02 | [slider-capture-type](https://github.com/CSProDevelopment/cspro/commit/ab0d6c75375594cc5484616a26326415869b2d28) | added the slider capture type (designer and JNI work) |
| 2020&#8209;10&#8209;30 | [text-source](https://github.com/CSProDevelopment/cspro/commit/95f928d8d3125de17e13f73be96fbdef947b59f8) | reworked how the logic and message file text buffers are processed by introducing a new TextSource series of objects |
| 2020&#8209;10&#8209;27 | [excel-lister](https://github.com/CSProDevelopment/cspro/commit/d9a834a10d753e25a3accd1950f8667d8a3d4aa2) | added the ability to output listing information to an Excel file |
| 2020&#8209;10&#8209;23 | [string-alpha-symbol-type](https://github.com/CSProDevelopment/cspro/commit/043a675a67539fe60a132ed89c973595dd19e5a1) | refactor logic string/alpha objects to use new WorkString/WorkAlpha objects rather than work SECTs/VARTs |
| 2020&#8209;10&#8209;19 | [biswa/1759_signature_optional_text](https://github.com/CSProDevelopment/cspro/commit/30335dbcd3655034680a5b5d78f54fdebd6055b7) | Biswa/1759 signature optional text |
