#include <engine/StandardSystemIncludes.h>
#include "JNIHelpers.h"
#include <zAction/PortableRunner.h>


SharableString ActionInvoker::PortableRunner::Clipboard_GetText()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jText(pEnv, (jstring)pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
                                                                                               JNIReferences::methodApplicationInterfaceClipboardGetText));
    return JavaString::ToSharableString(*pEnv, jText.get());
}


void ActionInvoker::PortableRunner::Clipboard_PutText(const std::string_view text_sv)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    auto jText = JNIReferences::make_local_ref(pEnv, JavaString::ToJava(*pEnv, std::string(text_sv)));

    pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,
                               JNIReferences::methodApplicationInterfaceClipboardPutText,
                               jText.get());

    ThrowJavaExceptionAsCSProException(pEnv);
}


void ActionInvoker::PortableRunner::System_CreateShortcut(const std::string& shortcut_id, const std::string& target_file_path,
                                                          const std::optional<std::string>& icon_file_path,
                                                          const std::string& label, const std::optional<std::string>& long_label)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jShortcutId(pEnv, JavaString::ToJava(*pEnv, shortcut_id));
    JNIReferences::scoped_local_ref<jstring> jTargetFilePath(pEnv, JavaString::ToJava(*pEnv, target_file_path));
    JNIReferences::scoped_local_ref<jstring> jIconFilePath(pEnv, JavaString::ToJava(*pEnv, icon_file_path));
    JNIReferences::scoped_local_ref<jstring> jLabel(pEnv, JavaString::ToJava(*pEnv, label));
    JNIReferences::scoped_local_ref<jstring> jLongLabel(pEnv, JavaString::ToJava(*pEnv, long_label));

    pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,
                               JNIReferences::methodApplicationInterfaceCreatePinShortcut,
                               jShortcutId.get(), jTargetFilePath.get(), jIconFilePath.get(), jLabel.get(), jLongLabel.get());

    ThrowJavaExceptionAsCSProException(pEnv);
}


std::vector<std::tuple<std::string, std::string>> ActionInvoker::PortableRunner::System_ShowSelectDocumentDialog(const std::vector<std::string>& mime_types,
                                                                                                                 const bool multiple)
{
    ASSERT(!mime_types.empty());

    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jobjectArray> jMimeTypes(pEnv, pEnv->NewObjectArray(mime_types.size(), JNIReferences::classString, pEnv->NewStringUTF("")));

    for( int i = 0; i < mime_types.size(); ++i )
    {
        JNIReferences::scoped_local_ref<jstring> jMimeType(pEnv, JavaString::ToJava(*pEnv, mime_types[i]));
        pEnv->SetObjectArrayElement(jMimeTypes.get(), i, jMimeType.get());
    }

    jobjectArray jPathsAndNames = (jobjectArray)pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
                                                                             JNIReferences::methodApplicationInterfaceShowSelectDocumentDialog,
                                                                             jMimeTypes.get(),
                                                                             multiple);
    ThrowJavaExceptionAsCSProException(pEnv);

    std::vector<std::tuple<std::string, std::string>> paths_and_names;

    if( jPathsAndNames != nullptr )
    {
        const int array_size = pEnv->GetArrayLength(jPathsAndNames);
        ASSERT(array_size % 2 == 0);

        for( int i = 0; i < array_size; i += 2 )
        {
            JNIReferences::scoped_local_ref<jstring> jPath(pEnv, (jstring)pEnv->GetObjectArrayElement(jPathsAndNames, i));
            JNIReferences::scoped_local_ref<jstring> jName(pEnv, (jstring)pEnv->GetObjectArrayElement(jPathsAndNames, i + 1));

            paths_and_names.emplace_back(JavaString::ToUtf8(*pEnv, jPath.get()),
                                         JavaString::ToUtf8(*pEnv, jName.get()));
        }
    }

    return paths_and_names;
}
