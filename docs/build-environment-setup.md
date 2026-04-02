# Setting up the CSPro Build Environment


## Repositories and Directories

CSPro, CSWeb, and other repositories can be installed in any directory, but these are the locations used by some developers on Windows machines:

- [CSPro](https://github.com/csprousers/cspro): C:\\cspro
- [CSWeb](https://github.com/csprousers/csweb): C:\\code\\csweb

For those working with CSPro documentation and resources:

- [Helps](https://github.com/csprousers/helps): C:\\code\\helps
- [Examples](https://github.com/csprousers/examples): C:\\code\\examples
- [Feature Showcases](https://github.com/csprousers/feature-showcase): C:\\code\\feature-showcase
- [Mobile Workshop](https://github.com/csprousers/workshop-mobile): C:\\code\\workshop

Additional CSPro-related repositories:

- [Prebuilt Libraries](https://github.com/csprousers/cspro-libraries): C:\\code\\cspro-open-source-libraries
- [Action Invoker (Android) Demo](https://github.com/csprousers/android-action-invoker-demo): C:\\code\\android-action-invoker-demo
- [CSEntry Launcher (Android)](https://github.com/csprousers/android-csentry-launcher): C:\\code\\android-csentry-launcher


## Building Windows

Building the CSPro solution requires:

- [Microsoft Visual Studio](https://visualstudio.microsoft.com)

Creating the installer requires:

- [Nullsoft Scriptable Install System (NSIS)](https://sourceforge.net/projects/nsis/)


## Building Android

Building the CSEntry application requires:

- [Android Studio](https://developer.android.com/studio) with the [NDK](https://developer.android.com/ndk) toolset

To speed up builds, the Android build process will use [ccache](https://ccache.dev) if its executable is part of the path environment available.


## Building Help Documentation

The help documentation is created using [CSDocument](https://www.csprousers.org/help/CSDocument), which is part of the CSPro installation. CSDocument creates HTML documentation directly, but the tool depends on tools to create alternative formats of documentation:

- [HTML Help Workshop](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/htmlhelp/microsoft-html-help-downloads) (for CHMs)
- [wkhtmltopdf](https://wkhtmltopdf.org) (for PDFs)
