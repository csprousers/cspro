#pragma once

#if defined(WIN_DESKTOP) || defined(_CONSOLE)

#include <zToolsO/zToolsO.h>
#include <zToolsO/WinRegistry.h>


// a simple way to load and store settings from the Windows registry
class CLASS_DECL_ZTOOLSO WinSettings
{
public:
    enum class Type
    {
        ViewNamesInTree,
        AppendLabelsToNamesInTree,

        LastApplicationDirectory,

        ListingFilenameExtension,
        FrequencyFilenameExtension,

        AddImageToResourceFolder,

        TraceWindowAlwaysOnTop,

        LogicSettings,
        LogicEditorZoomLevel,

        CodeFolding,
        DeprecationWarnings,
        WordWrap,

        CodeLogicSplitNewlines,
        CodeLogicUseVerbatimStringLiterals,
        CodeEscapeJsonForwardSlashes,

        LocalhostStartAutomatically,
        LocalhostPreferredPort,
        LocalhostAutomaticallyMappedDrives,
    };

    using KeyType = std::variant<Type, std::wstring>;

private:
    WinSettings();

    static WinSettings& GetInstance();

public:
    ~WinSettings();    

    template<typename T>
    static T Read(KeyType key) { return ReadWorker<T>(std::move(key), nullptr); }

    template<typename T>
    static T Read(KeyType key, T default_value) { return ReadWorker<T>(std::move(key), &default_value); }

    template<typename T>
    static void Write(KeyType key, const T& value);

private:
    static const TCHAR* GetKeyText(const KeyType& key);
    
private:
    template<typename T>
    static T ReadWorker(KeyType key, T* default_value);

private:
    WinRegistry m_winRegistry;
    std::map<KeyType, std::variant<std::wstring, DWORD>> m_loadedSettings;
    std::set<KeyType> m_savedSettings;
};

#endif
