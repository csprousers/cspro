#include <engine/StandardSystemIncludes.h>
#include "JNIHelpers.h"
#include <zAction/PortableRunner.h>


std::optional<std::wstring> ActionInvoker::PortableRunner::ClipboardGetText()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jText(pEnv, (jstring)pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
                                                                                               JNIReferences::methodApplicationInterfaceClipboardGetText));
    return JavaToOptionalWSZ(pEnv, jText.get());
}


void ActionInvoker::PortableRunner::ClipboardPutText(const std::wstring& text)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    auto jText = JNIReferences::make_local_ref(pEnv, WideToJava(pEnv, text));

    pEnv->CallStaticVoidMethod(JNIReferences::classApplicationInterface,
                               JNIReferences::methodApplicationInterfaceClipboardPutText,
                               jText.get());

    ThrowJavaExceptionAsCSProException(pEnv);
}


std::vector<std::tuple<std::wstring, std::wstring>> ActionInvoker::PortableRunner::SystemShowSelectDocumentDialog(const std::vector<std::wstring>& mime_types, const bool multiple)
{
    ASSERT(!mime_types.empty());

    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jobjectArray> jMimeTypes(pEnv, pEnv->NewObjectArray(mime_types.size(), JNIReferences::classString, pEnv->NewStringUTF("")));

    for( int i = 0; i < mime_types.size(); ++i )
    {
        JNIReferences::scoped_local_ref<jstring> jMimeType(pEnv, WideToJava(pEnv, mime_types[i]));
        pEnv->SetObjectArrayElement(jMimeTypes.get(), i, jMimeType.get());
    }

    jobjectArray jPathsAndNames = (jobjectArray)pEnv->CallStaticObjectMethod(JNIReferences::classApplicationInterface,
                                                                             JNIReferences::methodApplicationInterfaceShowSelectDocumentDialog,
                                                                             jMimeTypes.get(),
                                                                             multiple);
    ThrowJavaExceptionAsCSProException(pEnv);

    std::vector<std::tuple<std::wstring, std::wstring>> paths_and_names;

    if( jPathsAndNames != nullptr )
    {
        const int array_size = pEnv->GetArrayLength(jPathsAndNames);
        ASSERT(array_size % 2 == 0);

        for( int i = 0; i < array_size; i += 2 )
        {
            JNIReferences::scoped_local_ref<jstring> jPath(pEnv, (jstring)pEnv->GetObjectArrayElement(jPathsAndNames, i));
            JNIReferences::scoped_local_ref<jstring> jName(pEnv, (jstring)pEnv->GetObjectArrayElement(jPathsAndNames, i + 1));

            paths_and_names.emplace_back(JavaToWSZ(pEnv, jPath.get()), JavaToWSZ(pEnv, jName.get()));
        }
    }

    return paths_and_names;
}
