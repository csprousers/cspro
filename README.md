
## CSPro

This repository contains the code for the [Census and Survey Processing System](https://www.census.gov/data/software/cspro.html) (CSPro) and [CSEntry Android](https://play.google.com/store/apps/details?id=gov.census.cspro.csentry).

The CSPro suite of programs that runs on Windows is coded in C++ using the [Microsoft Foundation Class](https://learn.microsoft.com/en-us/cpp/mfc/framework-mfc) (MFC) framework. This code is built using [Microsoft Visual Studio](https://visualstudio.microsoft.com).

CSEntry, the Android application, is coded in Java and Kotlin and built using [Android Studio](https://developer.android.com/studio).

The shared runtime is coded in C++, with the [NDK](https://developer.android.com/ndk) toolset building this for Android.

----

Released versions of the software are available at the following sites:

- [Census Bureau](https://www.census.gov/data/software/cspro.html) (CSPro and CSWeb)
- [CSPro Users](https://www.csprousers.org/downloads) (CSPro and CSWeb)
- [Google Play](https://play.google.com/store/apps/details?id=gov.census.cspro.csentry) (CSEntry)


## Open-Source Software Licenses

CSPro is a work of the United States government and any CSPro-specific code is released to the public domain within the United States. CSPro uses several [open-source software](https://en.wikipedia.org/wiki/Open-source_software) packages, and it is essential to review each [license](https://github.com/csprousers/cspro/tree/dev/build-tools/Licenses/Licenses) to ensure that any derived use of this code follows all licenses.


## Private Development, Public Repository

Most development on CSPro occurs on a [private repository](https://github.com/CSProDevelopment/cspro). The commits in this public repository are snapshots of the development that occurs on the private repository.

If you submit a pull request that is accepted by the development team, the pull request will be processed in this public repository and/or the private repository. Regardless, the changes will be incorporated as part of the next code snapshot.


## Limitations

This public code snapshot differs from the code on the private repository that is used to create the released version of CSPro:

- The released version of CSPro uses the [SQLite Encryption Extension](https://sqlite.org/com/see.html) (SEE) to support working with encrypted SQLite databases. Because SEE requires a license, it cannot be released in this public repository. The SQLite compilation units included here are from the public SQLite release: [sqlite3.c](https://github.com/csprousers/cspro/blob/dev/cspro/external/SQLite/sqlite3.c) and [sqlite3.h](https://github.com/csprousers/cspro/blob/dev/cspro/external/SQLite/sqlite3.h). Any access to encrypted SQLite databases using code built from this public repository will result in an exception.

- The API keys used to access or use CSWeb, Dropbox, and Google Maps have been removed (from [ApiKeys.h](https://github.com/csprousers/cspro/blob/dev/cspro/zToolsO/ApiKeys.h) and [api_keys.xml](https://github.com/csprousers/cspro/blob/dev/cspro/CSEntryDroid/app/src/main/res/values/api_keys.xml)). Those choosing to build CSPro from this public snapshot will have to provide their own API keys if they want to use these services.


## Contributing

We welcome contributions from everyone! If you'd like to contribute, please follow these guidelines:

- All contributions must be submitted via a [pull request](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/about-pull-requests) to the [dev](https://github.com/csprousers/csweb/tree/dev) branch.
- Ensure that the pull request is well-documented and includes a clear description of the changes.
- Follow the project's coding style and best practices.

Thank you for helping improve this project!
