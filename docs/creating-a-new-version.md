# Creating a New Version of CSPro

This document lists some steps to follow when creating a new version. This is for changes that will result in a major and minor release, not a patch release.


## Code

1. Consider updating the external libraries by following the steps in the document:
    - docs/external-libraries.md

2. Consider updating the NuGet packages:
    - Open the solution in Visual Studio.
    - Right-click on the solution in the *Solution Explorer*.
    - Select *Manage NuGet Packages for Solution*.
    - Select *Updates* -> *Select all packages* ->  *Update*.


## Helps

1. Update the version numbers in:
    - Shared/definitions.json

2. Add a new entry for the version in:
    - CSPro/topics/release_history.csdoc

3. Add a new *What's New* topic and link to it from the main topic:
    - CSPro/topics/what_is_new_in_cspro.csdoc
    - CSPro/topics/what_is_new_in_cspro_[major]_[minor].csdoc

4. Remove the *ID_HELP_WHAT_IS_NEW* context ID from the previous *What's New* topic, moving it to this new topic.

5. The following is a sample commit message for some of the changes above:
    - set up the helps for CSPro 8.1
