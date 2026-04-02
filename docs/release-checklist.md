# CSPro Release Checklist

This document lists the steps necessary to create a CSPro release.


## Naming Conventions

1. If creating a release, files will be named as follows:
    - cspro-8.0.1-windows-x86.exe
    - cspro-8.0.1-release-notes.txt
    - csentry-8.0.1.apk
    - csweb-8.0.1.tar.gz
    - csweb-8.0.1.zip

2. If creating a prerelease, files will be named as follows:
    - cspro-8.1.0-alpha-20260227-windows-x86.exe
    - cspro-8.1.0-alpha-20260227-release-notes.txt
    - csentry-8.1.0-alpha-20260227.apk
    - csweb-8.1.0-alpha-20260227.tar.gz
    - csweb-8.1.0-alpha-20260227.zip


## Code

1. When creating a branch for this preparation work, name it using the following convention:
    - release/v8.0.0-20240319
    - prerelease/v8.1.0-alpha-20260227

2. Update the release type and version numbers and dates in the following files:
    - build-tools/Installer Inputs/readme.txt
    - cspro/CSEntryDroid/app/src/main/AndroidManifest.xml
        - Use the branch name, following the "/v", as the versionName.
    - cspro/zUtilO/Versioning.cpp
    - cspro/zUtilO/Versioning.h

3. Update the version numbers in the \*.rc and AssemblyInfo.\* files by running:
    - build-tools/Run Update Version.bat

4. Ensure that code has been properly synced and standardized by running:
    - build-tools/Images - Create Toolbars.bat
    - build-tools/Run Generate Combined License.bat
    - build-tools/Run Project File Manager.bat
    - build-tools/Run Resource ID Numberer.bat
    - cspro/html/Update Android HTML Assets.bat

5. The following are sample commit message for some of the changes above:
    - updated the versioning information for a CSPro 8.1.0 release (alpha: 2026-02-27)
    - updated the resource IDs using the Resource ID Numberer
    - updated the Android assets (as of 2026-02-27)

6. Tag the repository using a variant of the branch name. For example:
    - v8.0.1-2024-03-19
    - v8.1.0-2026-02-27-alpha


## Messages

1. Format the message files by running:
    - build-tools/Messages - Format Files.bat

2. Check for missing runtime messages, or messages with invalid formatting, by running:
    - build-tools/Messages - Runtime Messages Audit

3. Optionally, check for missing Android strings:
    - Open the English version of strings.xml in Android Studio:
        - cspro/CSEntryDroid/app/src/main/res/values/strings.xml
    - Look for a red squiggle under messages. Hovering over the messages tells you which languages are missing.


## Helps

1. Update the release type and version numbers and dates in the following files:
    - CSPro/topics/release_history.csdoc
    - Shared/definitions.json


## Examples

1. Resave the CSPro application files using the current version by running:
    - build-tools/Run Update Example Files.bat


## Release - Windows

1. Open the CSPro Installer Generator by running:
    - build-tools/Create Installer.bat

2. Select *Prepare and Analyze Inputs* and verify that the release type and version numbers and dates are correct.

3. Select all *Action* checkboxes.

4. Select *Create Installer*.

5. Upload to the CSPro Users website the following files, renamed as indicated above in *Naming Conventions*:
    - build-tools/Installer Inputs/readme.txt
    - build-tools/Installer Inputs/Installer/cspro[version].exe

6. Create a GitHub release using the following steps:

    1. Run the Open Source Syncer by running:
        - build-tools/Sync Open Source Repository.bat

    2. Sync the public and private repositories using *Commits* -> *Sync Feature Branches*.

    3. Sync the release tag using *Releases* -> *Sync Tags* and push the tag to GitHub.

    4. Using the *Releases* -> *Manage Releases* interface, select *Create Release From Tag* using the new tag.

    5. Select the installer created in step #4, review the release notes, and select *Create Release*.


## Release - Android (Google Play)

1. Open the CSPro Installer Generator by running:
    - build-tools/Create Installer.bat

2. Select *Regenerate Assets*.

3. Open Android Studio.

4. Select *Build* -> *Generate Signed App Bundle or APK*.

5. Select *Android App Bundle*.

6. Specify:
    - *Key store path* -> cspro/CSEntryDroid/.csentrydroidkeystore
    - *Key store password* -> (the password)
    - *Key alias* -> gov.census.cspro.csentrydroid
    - *Key password* -> (the same password as above)

7. On the next screen, for *build variant*, select *release*.

8. Upload to Google Play the following files:
    - cspro/CSEntryDroid/app/release/app-release.aab
    - cspro/CSEntryDroid/app/build/outputs/native-debug-symbols/release/native-debug-symbols.zip


## Release - Android (CSPro Users Website APK for Sideloading)

1. Open the CSPro Installer Generator by running:
    - build-tools/Create Installer.bat

2. Select *Regenerate Assets*.

3. Open Android Studio.

4. Uncomment out the code around QUERY_ALL_PACKAGES in:
    - cspro/CSEntryDroid/app/src/main/AndroidManifest.xml

5. Select *Build* -> *Generate Signed App Bundle or APK*.

6. Select *APK*.

7. Specify:
    - *Key store path* -> cspro/CSEntryDroid/.csentrydroidkeystore
    - *Key store password* -> (the password)
    - *Key alias* -> gov.census.cspro.csentrydroid
    - *Key password* -> (the same password as above)

8. On the next screen, for *build variant*, select *release*.

9. Upload to the CSPro Users website the following file, renamed as indicated above in *Naming Conventions*:
    - cspro/CSEntryDroid/app/release/app-release.apk


## Release - Android (APK Size Reduction)

If creating a special build where minimizing the size of the APK is important, consider making the following changes in cspro/CSEntryDroid/app/build.gradle:

1. Set *useLegacyPackaging* to *true*. This will compress the .so files in the APK.

2. If only using x64 devices, modify *release* -> *ndkBuild* -> *abiFilters* to only *'arm64-v8a'*. The APK will not contain a x86 version with this change.


## Publicity

1. Consider adding blog posts highlighting new features.

2. Consider adding a [feature showcase](https://github.com/csprousers/feature-showcase) demonstrating new features.

3. Announce the release on the CSPro Users website [blog](https://csprousers.org) and [forum](https://csprousers.org/forum).

4. Announce the release on social media accounts.
