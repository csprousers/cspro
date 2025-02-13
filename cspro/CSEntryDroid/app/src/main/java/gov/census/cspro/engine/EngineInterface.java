package gov.census.cspro.engine;


import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.media.MediaScannerConnection;
import android.os.Build;
import android.os.Environment;

import androidx.core.content.ContextCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.util.ArrayList;

import gov.census.cspro.ActionInvokerActivityResult;
import gov.census.cspro.bridge.CNPifFile;
import gov.census.cspro.csentry.CSEntryDirectory;
import gov.census.cspro.csentry.ui.UserbarHandler;
import gov.census.cspro.data.CaseSummary;
import gov.census.cspro.form.CaseTreeNode;
import gov.census.cspro.form.CaseTreeUpdate;
import gov.census.cspro.form.EntryPage;
import gov.census.cspro.form.FieldNote;
import gov.census.cspro.html.HtmlDirectoryPathHandler;
import gov.census.cspro.html.VirtualFile;
import gov.census.cspro.util.CredentialStore;
import timber.log.Timber;

// Project Imports

public class EngineInterface
{
    private static EngineInterface instance                         = null;
    private long m_nativeEngineInterfaceReference;
    private boolean                     m_bApplicationOpen          = false;
    private String                      m_windowTitle               = null;
    private UserbarHandler              m_userbarHandler            = null;
    private ParadataDriver              m_paradataDriver            = null;
    private static String               m_execpffParameter;
    private ArrayList<String>           m_dataFilesCreatedFromPff   = null;
    private ArrayList<String>           m_otherFilesCreatedFromPff  = null;
    private CredentialStore             m_credentialStore;
    private String m_applicationFilename;

    public CSEntryDirectory csEntryDirectory;

    protected EngineInterface(Application application)
    {
        m_nativeEngineInterfaceReference = InitNativeEngineInterface();

        csEntryDirectory = new CSEntryDirectory(application.getApplicationContext());

        // Start messenger used to manage communication between engine and UI
        Messenger.CreateMessengerInstance();

        // Hook up messenger so it can track current activity
        application.registerActivityLifecycleCallbacks(Messenger.getInstance());

        Thread messengerThread = new Thread(Messenger.getInstance());
        messengerThread.setName("Messenger");
        messengerThread.start();

        initializeAndroidEnvironmentVariables(application.getApplicationContext());
        m_credentialStore = new CredentialStore(application.getApplicationContext());
    }

    public static void CreateEngineInterfaceInstance(Application application)
    {
        if (instance == null)
            instance = new EngineInterface(application);
    }

    public static EngineInterface getInstance()
    {
        return instance;
    }

    public UserbarHandler getUserbarHandler()
    {
        return m_userbarHandler;
    }

    public ParadataDriver getParadataDriver()
    {
        return m_paradataDriver;
    }

    void setParadataDriver(ParadataDriver paradataDriver)
    {
        m_paradataDriver = paradataDriver;
    }

    CredentialStore getCredentialStore() { return m_credentialStore; }

    @Override
    protected void finalize() throws Throwable
    {
        super.finalize();

        // TODO: Should really implement a JNI method that calls the native
        // application interface destructor
    }

    public boolean openApplication(String pffFilename)
    {
        boolean result = false;

        try
        {
            m_userbarHandler = new UserbarHandler();
            m_paradataDriver = null;
            m_execpffParameter = null;

            result = InitApplication(m_nativeEngineInterfaceReference, pffFilename);
            if (result)
                m_bApplicationOpen = true;

            CNPifFile pifFile = new CNPifFile(pffFilename);

            if( pifFile.IsValid() )
            {
                m_windowTitle = getApplicationDescription();

                m_applicationFilename = pifFile.GetAppFilename();

                // Save off data, external, write files referenced in pff so that we do a
                // a media scan when the app ends.
                saveFilesCreatedByPff(pifFile);

                // if an OnExit filename is specified, queue that as the next PFF to be run
                if( pifFile.GetOnExitFilename().length() > 0 )
                    m_execpffParameter = pifFile.GetOnExitFilename();
            }

        }
        catch(Exception ex)
        {
            Timber.e(ex, "An Error Occurred While Attempting to Open: %s", pffFilename);
        }

        return result;
    }

    /**
     * Extract data, external, write and user files from pff
     * and send so that we can later send them to Android
     * media scanner so that they will show up when the
     * device is connected to a PC. Without forcing the
     * scan they won't show up until the device is rebooted
     * (at least on certain devices).
     */
    private void saveFilesCreatedByPff(CNPifFile pifFile)
    {
        m_dataFilesCreatedFromPff = new ArrayList<>();
        m_otherFilesCreatedFromPff = new ArrayList<>();

        // For data file add the data file itself plus anything
        // that starts with the data file so that we pick up
        // the .dat.not, .dat.sts etc...
        String dataFile = pifFile.GetInputFilename();
        if( !dataFile.trim().isEmpty() )
            m_dataFilesCreatedFromPff.add(dataFile);

        String[] externalFiles = pifFile.GetExternalFilenames();
        for (String filename : externalFiles) {
            if( !filename.trim().isEmpty() )
                m_dataFilesCreatedFromPff.add(filename);
        }

        String[] userFiles = pifFile.GetUserFilenames();
        for (String filename : userFiles) {
            if( !filename.trim().isEmpty() )
                m_otherFilesCreatedFromPff.add(filename);
        }

        String writeFile = pifFile.GetWriteFilename();
        if( !writeFile.trim().isEmpty() )
            m_otherFilesCreatedFromPff.add(writeFile);
    }

    /**
     * Extract data, external, write and user files from pff
     * and send them to Android media scanner so that they
     * will show up when the device is connected to a PC.
     * Without forcing the scan they won't show up until
     * the device rebooted (at least on certain devices).
     * @param context Context required by media scanner
     */
    private void scanFilesCreatedInPff(Context context)
    {
        ArrayList<String> toScan = new ArrayList<>(m_dataFilesCreatedFromPff);
        for (String dataFilename : m_dataFilesCreatedFromPff)
            toScan.addAll(GetFilesThatStartWith(dataFilename));
        toScan.addAll(m_otherFilesCreatedFromPff);
        MediaScannerConnection.scanFile(context,
                toScan.toArray(new String[] {}),
                null,
                null);
    }

    /**
     * Return all files in same directory that start with same name
     * as file passed in.
     */
    private ArrayList<String> GetFilesThatStartWith(String fileFullPath)
    {
        ArrayList<String> results = new ArrayList<>();
        String name = Util.removeDirectory(fileFullPath);
        File dir = new File(Util.removeFilename(fileFullPath));
        if (dir.exists()) {
            File[] files = dir.listFiles();
            if (files != null) {
                for (File f : files) {
                    if (f.isFile() && f.getName().startsWith(name)) {
                        results.add(f.getAbsolutePath());
                    }
                }
            }
        }
        return results;
    }

    private void initializeAndroidEnvironmentVariables(Context context)
    {
        // GHM 20131209 pass down variables used in the application, and values that can be queried by the getusername and pathname functions

        String email = "";
        AccountManager accountManager = AccountManager.get(context);
        Account[] accounts = accountManager.getAccountsByType("com.google");
        if( accounts.length > 0 )
            email = accounts[0].name;

        String tempFolder = context.getCacheDir().getAbsolutePath();

        String applicationFolder = "", versionNumber = "", assetsDirectory = "";
        String downloadsDirectory = "";
        try
        {
            PackageManager pm = context.getPackageManager();
            PackageInfo pi = pm.getPackageInfo(context.getPackageName(), 0);
            applicationFolder = Util.removeFilename(pi.applicationInfo.sourceDir);
            versionNumber = Build.VERSION.RELEASE;
            assetsDirectory = ensureAssetsExist(context);
            downloadsDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
        }
        catch( Exception ignored )
        {
        }

        String externalMemoryCardDirectory = null;
        File [] externalDirs = ContextCompat.getExternalFilesDirs(context, null);
        if (externalDirs.length > 1 && externalDirs[1] != null && externalDirs[1].exists()) {
            externalMemoryCardDirectory = externalDirs[1].getAbsolutePath();
        }

        String internalStorageDirectory = context.getDir("InternalData",ContextWrapper.MODE_PRIVATE).getAbsolutePath();

        SetAndroidEnvironmentVariables(m_nativeEngineInterfaceReference,
                email,tempFolder,applicationFolder,
                versionNumber,assetsDirectory,
                csEntryDirectory.getPath(),
                externalMemoryCardDirectory,
                internalStorageDirectory,
                downloadsDirectory);
    }

    public static String getVersionDetailedString()
    {
        return GetInformationForAbout(true);
    }

    public static String getReleaseDateString()
    {
        return GetInformationForAbout(false);
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    private String ensureAssetsExist(Context context) throws Exception
    {
        // use the version number from the manifest as a directory name for the asset files;
        // this way the assets will get updated when the app is upgraded
        String assetsDirectoryName = Util.combinePath(Util.combinePath(context.getApplicationInfo().dataDir,"assets"),
                Integer.toString(context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionCode));
        File assetsDirectory = new File(assetsDirectoryName);

        if( !assetsDirectory.exists() )
        {
            AssetManager assetManager = context.getAssets();

            //noinspection ResultOfMethodCallIgnored
            assetsDirectory.mkdirs();

            // the serialized system messages is stored in the .apk assets and is saved to the data directory
            // if it hasn't already been; this will significantly reduce the size of .pen files
            final String messageFilename = "system.mgf";
            Util.copyAndCloseStreams(assetManager.open(messageFilename),new FileOutputStream(new File(assetsDirectoryName,messageFilename)));

            // write out the html and report assets
            copyAssetsDirectory(assetManager, assetsDirectoryName, "html");
            copyAssetsDirectory(assetManager, assetsDirectoryName, "Reports");
        }

        HtmlDirectoryPathHandler.Companion.setAssetsDirectory(assetsDirectory.getAbsolutePath());

        return assetsDirectory.getAbsolutePath();
    }

    private void copyAssetsDirectory(AssetManager assetManager, String assetsDirectoryName, String directoryName) throws Exception
    {
        String outputDirectoryName = Util.combinePath(assetsDirectoryName, directoryName);
        new File(outputDirectoryName).mkdir();

        String[] paths = assetManager.list(directoryName);
        if (paths != null) {
            for(String path : paths) {
                String fullPath = directoryName + File.separator + path;
                // the path can be either a directory or a file
                String[] directoryPaths = assetManager.list(fullPath);
                if (directoryPaths != null && directoryPaths.length > 0 ) {
                    copyAssetsDirectory(assetManager, assetsDirectoryName, fullPath);
                } else {
                    Util.copyAndCloseStreams(assetManager.open(fullPath), new FileOutputStream(new File(Util.combinePath(outputDirectoryName, path))));
                }
            }
        }
    }

    public void getSequentialCaseIds(ArrayList<CaseSummary> caseSummaries)
    {
        GetSequentialCaseIds(m_nativeEngineInterfaceReference, caseSummaries);
    }

    public boolean getStartInEntry()
    {
        return queryPffStartMode().action != PffStartModeParameter.NO_ACTION;
    }

    // Get the key of the case to start with if one was specified in pff
    // Could come from either "startMode" or "key" entry in pff file.
    public String getStartCaseKey()
    {
        String startKey = GetStartKeyString(m_nativeEngineInterfaceReference);
        if (Util.stringIsNullOrEmpty(startKey))
            startKey = GetStartPffKey(m_nativeEngineInterfaceReference);
        return startKey;
    }

    public PffStartModeParameter queryPffStartMode()
    {
        return QueryPffStartMode(m_nativeEngineInterfaceReference);
    }

    public boolean getAskOpIDFlag()
    {
        return GetAskOpIDFlag(m_nativeEngineInterfaceReference);
    }

    public String getOpIDFromPff()
    {
        return GetOpIDFromPff(m_nativeEngineInterfaceReference);
    }

    public void setOperatorId(String operatorID)
    {
        SetOperatorId(m_nativeEngineInterfaceReference,operatorID);
    }

    public boolean getAddLockFlag()
    {
        return GetAddLockFlag(m_nativeEngineInterfaceReference);
    }

    public boolean getModifyLockFlag()
    {
        return GetModifyLockFlag(m_nativeEngineInterfaceReference);
    }

    public boolean getDeleteLockFlag()
    {
        return GetDeleteLockFlag(m_nativeEngineInterfaceReference);
    }

    public boolean getViewLockFlag()
    {
        return GetViewLockFlag(m_nativeEngineInterfaceReference);
    }

    public boolean getCaseListingLockFlag()
    {
        return GetCaseListingLockFlag(m_nativeEngineInterfaceReference);
    }

    public boolean doNotShowCaseListing()
    {
        return DoNotShowCaseListing(m_nativeEngineInterfaceReference);
    }

    public boolean isInitialized()
    {
        return (m_nativeEngineInterfaceReference != 0);
    }

    public boolean deleteCase(double casePosition)
    {
        return DeleteCase(m_nativeEngineInterfaceReference, casePosition);
    }

    public void changeLanguage()
    {
        ChangeLanguage(m_nativeEngineInterfaceReference);
    }

    public boolean isSystemControlled()
    {
        return GetSystemControlled(m_nativeEngineInterfaceReference);
    }

    public boolean containsMultipleLanguages()
    {
        return ContainsMultipleLanguages(m_nativeEngineInterfaceReference);
    }

    public void runUserBarFunction(int userbar_index)
    {
        RunUserbarFunction(m_nativeEngineInterfaceReference, userbar_index);
    }

    public static void setExecPffParameter(String filename)
    {
        m_execpffParameter = filename;
    }

    public static String getExecPffParameter()
    {
        return m_execpffParameter;
    }

    public boolean showsRefusalsAutomatically()
    {
        return ShowsRefusalsAutomatically(m_nativeEngineInterfaceReference);
    }

    public boolean showRefusals()
    {
        return ShowRefusals(m_nativeEngineInterfaceReference);
    }

    public void savePartial()
    {
        SavePartial(m_nativeEngineInterfaceReference);
    }

    public boolean allowsPartialSave()
    {
        return AllowsPartialSave(m_nativeEngineInterfaceReference);
    }

    public boolean showCaseTree()
    {
        return GetShowCaseTreeFlag(m_nativeEngineInterfaceReference);
    }

    public boolean getAutoAdvanceOnSelectionFlag()
    {
        return GetAutoAdvanceOnSelectionFlag(m_nativeEngineInterfaceReference);
    }

    public boolean getDisplayCodesAlongsideLabelsFlag()
    {
        return GetDisplayCodesAlongsideLabelsFlag(m_nativeEngineInterfaceReference);
    }

    void getParadataCachedEvents()
    {
        GetParadataCachedEvents(m_nativeEngineInterfaceReference);
    }

    public String getApplicationDirectory() {
        if (m_applicationFilename != null) {
            return Util.removeFilename(m_applicationFilename);
        } else {
            // use the csentry directory if no application has been loaded
            return csEntryDirectory.getPath();
        }
    }

    // field iteration
    public native void      NextField(long applicationReference);
    public native void      PrevField(long applicationReference);
    public native boolean   HasPersistentFields(long applicationReference);
    public native void      PreviousPersistentField(long applicationReference);

    public native boolean   InsertOcc(long applicationReference);
    public native boolean   DeleteOcc(long applicationReference);
    public native boolean   InsertOccAfter(long applicationReference);

    public native void      GoToField(long applicationReference, int fieldSymbol, int index1, int index2, int index3);
    public native void      EndGroup(long applicationReference);
    public native void      AdvanceToEnd(long applicationReference);
    public native void      EndLevel(long applicationReference);
    public native void      EndLevelOcc(long applicationReference);

    // application creation and initialization
    private native long     InitNativeEngineInterface();
    public native boolean   InitApplication(long applicationReference, String pffFilename);
    public native boolean   Start(long applicationReference);
    public native void      Stop(long applicationReference);
    public native int       StopCode(long applicationReference);
    public native void      RunUserTriggedStop(long applicationReference);
    public native void      EndApplication(long applicationReference);
    public native boolean   ModifyCase(long applicationReference, double casePosition);
    public native boolean   InsertCase(long applicationReference, double casePosition);

    // id retrieval
    public native boolean   GetSequentialCaseIds(long applicationReference, ArrayList<CaseSummary> caseSummaries);

    // case manipulation
    public native boolean   DeleteCase(long applicationReference, double casePosition);


    // operator id functions
    public native boolean   GetAskOpIDFlag(long applicationReference);
    public native String    GetOpIDFromPff(long applicationReference);
    public native void      SetOperatorId(long applicationReference,String operatorID);

    // misc. functions
    public native void      SetAndroidEnvironmentVariables(long applicationReference, String email,
                                                             String tempFolder,String applicationFolder,
                                                             String versionNumber,String assetsDirectory,
                                                             String csEntryDirectory,
                                                             String externalMemoryCardDirectory,
                                                             String internalStorageDirectory,
                                                             String downloadsDirectory);
    private native static String GetInformationForAbout(boolean versionInformation);
    public native boolean   GetSystemControlled(long applicationReference);
    public native boolean   ContainsMultipleLanguages(long applicationReference);
    public native String    GetStartKeyString(long applicationReference);
    public native String    GetStartPffKey(long applicationReference);
    public native PffStartModeParameter QueryPffStartMode(long applicationReference);
    public native boolean   GetAddLockFlag(long applicationReference);
    public native boolean   GetModifyLockFlag(long applicationReference);
    public native boolean   GetDeleteLockFlag(long applicationReference);
    public native boolean   GetViewLockFlag(long applicationReference);
    public native boolean   GetCaseListingLockFlag(long applicationReference);
    public native boolean   DoNotShowCaseListing(long applicationReference);
    public native void      RunUserbarFunction(long applicationReference, int userbar_index);
    public native boolean   ShowsRefusalsAutomatically(long applicationReference);
    public native boolean   ShowRefusals(long applicationReference);
    public native void      SavePartial(long applicationReference);
    public native boolean   AllowsPartialSave(long applicationReference);
    public native boolean   GetShowCaseTreeFlag(long applicationReference);
    public native boolean   GetAutoAdvanceOnSelectionFlag(long applicationReference);
    public native boolean   GetDisplayCodesAlongsideLabelsFlag(long applicationReference);
    public native void      GetParadataCachedEvents(long applicationReference);
    public native String    GetApplicationDescription(long applicationReference);

    // language functions
    public native void      ChangeLanguage(long applicationReference);

    // caseTree
    public native CaseTreeUpdate[] UpdateCaseTree(long applicationReference);
    public native CaseTreeNode GetCaseTree(long applicationReference);

    // notes
    public void editCaseNote()
    {
        EditCaseNote(m_nativeEngineInterfaceReference);
    }
    public native void EditCaseNote(long applicationReference);

    public void reviewNotes()
    {
        ReviewNotes(m_nativeEngineInterfaceReference);
    }
    public native void ReviewNotes(long applicationReference);

    public ArrayList<FieldNote> getAllNotes()
    {
        ArrayList<FieldNote> notes = new ArrayList<>();
        GetAllNotes(m_nativeEngineInterfaceReference,notes);
        return notes;
    }
    public native void GetAllNotes(long applicationReference, ArrayList<FieldNote> fieldNotes);

    public void deleteNote(long note_index)
    {
        DeleteNote(m_nativeEngineInterfaceReference, note_index);
    }
    public native void DeleteNote(long applicationReference, long note_index);

    public void goToNoteField(long note_index)
    {
        GoToNoteField(m_nativeEngineInterfaceReference, note_index);
    }
    public native void GoToNoteField(long applicationReference, long note_index);

    public native void OnProgressDialogCancel(long applicationReference);

    private native boolean HasSync(long applicationReference);
    private native boolean SyncApp(long applicationReference);
    private native long GetCurrentPage(long applicationReference, boolean processPossibleRequests);

    public void onProgressDialogCancel()
    {
        OnProgressDialogCancel(m_nativeEngineInterfaceReference);
    }

    public EntryPage getCurrentPage(boolean processPossibleRequests)
    {
        long nativePage = GetCurrentPage(m_nativeEngineInterfaceReference, processPossibleRequests);
        return nativePage != 0 ? new EntryPage(nativePage) : null;
    }

    public void NextField()
    {
        // advance to the next field
        NextField(m_nativeEngineInterfaceReference);
    }

    public void PreviousField()
    {
        // move backwards a field
        PrevField(m_nativeEngineInterfaceReference);
    }

    public boolean hasPersistentFields()
    {
        return HasPersistentFields(m_nativeEngineInterfaceReference);
    }

    public void PreviousPersistentField()
    {
        // move backwards a previous persistent field
        PreviousPersistentField(m_nativeEngineInterfaceReference);
    }

    public void insertOcc()
    {
        InsertOcc(m_nativeEngineInterfaceReference);
    }

    public void insertOccAfter()
    {
        // advance to the next field
        InsertOccAfter(m_nativeEngineInterfaceReference);
    }

    public void deleteOcc()
    {
        DeleteOcc(m_nativeEngineInterfaceReference);
    }

    public String getApplicationDescription()
    {
        return GetApplicationDescription(m_nativeEngineInterfaceReference);
    }

    public String getWindowTitle()
    {
        return m_windowTitle;
    }

    public void setWindowTitle(String desc)
    {
        m_windowTitle = desc;
    }

    public boolean start()
    {
        return Start(m_nativeEngineInterfaceReference);
    }

    public boolean modifyCase(double casePosition)
    {
        return ModifyCase(m_nativeEngineInterfaceReference,casePosition);
    }

    public boolean insertCase(double casePosition)
    {
        return InsertCase(m_nativeEngineInterfaceReference, casePosition);
    }

    public boolean isApplicationOpen()
    {
        return m_bApplicationOpen;
    }

    public void endApplication()
    {
        EndApplication(m_nativeEngineInterfaceReference);
        m_bApplicationOpen = false;
    }

    public void stopApplication(Activity activity)
    {
        scanFilesCreatedInPff(activity);
        Stop(m_nativeEngineInterfaceReference);
    }

    public int getStopCode()
    {
        return StopCode(m_nativeEngineInterfaceReference);
    }

    public void runUserTriggedStop()
    {
        RunUserTriggedStop(m_nativeEngineInterfaceReference);
    }

    public void AdvanceToEnd()
    {
        AdvanceToEnd(m_nativeEngineInterfaceReference);
    }
    public void EndGroup()
    {
        EndGroup(m_nativeEngineInterfaceReference);
    }
    public void EndLevel()
    {
        EndLevel(m_nativeEngineInterfaceReference);
    }
    public void EndLevelOcc()
    {
        EndLevelOcc(m_nativeEngineInterfaceReference);
    }

    public void goToField(int fieldSymbol, int index1, int index2, int index3)
    {
        GoToField(m_nativeEngineInterfaceReference,fieldSymbol,index1,index2,index3);
    }

    public CaseTreeNode getCaseTree()
    {
        return GetCaseTree(m_nativeEngineInterfaceReference);
    }

    public CaseTreeUpdate[] updateCaseTree()
    {
        return UpdateCaseTree(m_nativeEngineInterfaceReference);
    }

    public boolean HasSync()
    {
        return HasSync(m_nativeEngineInterfaceReference);
    }

    public boolean SyncApp()
    {
        return SyncApp(m_nativeEngineInterfaceReference);
    }

    // system settings functions
    public native static String GetSystemSettingString(String setting_name, String default_value);
    public native static boolean GetSystemSettingBoolean(String setting_name, boolean default_value);

    // message functions
    public native static String GetRuntimeString(int message_number, String text);

    // mapping functions
    public AppMappingOptions getMappingOptions() { return GetMappingOptions(m_nativeEngineInterfaceReference); }
    private native AppMappingOptions GetMappingOptions(long applicationReference);

    public BaseMapSelection getBaseMapSelection()
    {
        return GetBaseMapSelection(m_nativeEngineInterfaceReference);
    }
    private native BaseMapSelection GetBaseMapSelection(long applicationReference);

    public String formatCoordinates(double latitude, double longitude)
    {
        return FormatCoordinates(m_nativeEngineInterfaceReference, latitude, longitude);
    }
    private native String FormatCoordinates(long applicationReference, double latitude, double longitude);

    public void runNonEntryApplication(String pffFilename)
    {
        RunNonEntryApplication(m_nativeEngineInterfaceReference, pffFilename);
    }
    private native void RunNonEntryApplication(long applicationReference, String pffFilename);

    public ActionInvokerActivityResult runActionInvoker(String callingPackage, String action, String accessToken, String refreshToken, boolean abortOnException)
    {
        return RunActionInvoker(m_nativeEngineInterfaceReference, callingPackage, action, accessToken, refreshToken, abortOnException);
    }
    private native ActionInvokerActivityResult RunActionInvoker(long applicationReference, String callingPackage, String action, String accessToken, String refreshToken, boolean abortOnException);

    public native static String CreateRegularExpressionFromFileSpec(String fileSpec);

    public long getThreadWaitId()
    {
        return GetThreadWaitId(m_nativeEngineInterfaceReference);
    }
    public native long GetThreadWaitId(long applicationReference);

    public void setThreadWaitComplete(long threadWaitId, String response)
    {
        SetThreadWaitComplete(m_nativeEngineInterfaceReference, threadWaitId, response);
    }
    public native void SetThreadWaitComplete(long applicationReference, long threadWaitId, String response);

    public boolean useHtmlDialogs()
    {
        return UseHtmlDialogs(m_nativeEngineInterfaceReference);
    }
    private native boolean UseHtmlDialogs(long applicationReference);

    public int parseDimensionText(String dimensionText, boolean isWidth)
    {
        return ParseDimensionText(m_nativeEngineInterfaceReference, dimensionText, isWidth);
    }
    private native int ParseDimensionText(long applicationReference, String dimensionText, boolean isWidth);

    public int actionInvokerCreateWebController(String actionInvokerAccessTokenOverride)
    {
        return ActionInvokerCreateWebController(m_nativeEngineInterfaceReference, actionInvokerAccessTokenOverride);
    }
    private native int ActionInvokerCreateWebController(long applicationReference, String actionInvokerAccessTokenOverride);

    public void actionInvokerCancelAndWaitOnActionsInProgress(int webControllerKey)
    {
        ActionInvokerCancelAndWaitOnActionsInProgress(m_nativeEngineInterfaceReference, webControllerKey);
    }
    private native void ActionInvokerCancelAndWaitOnActionsInProgress(long applicationReference, int webControllerKey);

    public String actionInvokerProcessMessage(int webControllerKey, ActionInvokerListener listener, String message, boolean async, boolean calledByOldCSProObject)
    {
        return ActionInvokerProcessMessage(m_nativeEngineInterfaceReference, webControllerKey, listener, message, async, calledByOldCSProObject);
    }
    private native String ActionInvokerProcessMessage(long applicationReference, int webControllerKey, ActionInvokerListener listener, String message, boolean async, boolean calledByOldCSProObject);

    public String oldCSProJavaScriptInterfaceGetAccessToken() { return oldCSProJavaScriptInterfaceGetAccessToken(m_nativeEngineInterfaceReference); }
    private native String oldCSProJavaScriptInterfaceGetAccessToken(long applicationReference);

    public VirtualFile getVirtualFile(String path) { return GetVirtualFile(m_nativeEngineInterfaceReference, path); }
    private native VirtualFile GetVirtualFile(long applicationReference, String path);

    public void viewCurrentCase() { ViewCurrentCase(m_nativeEngineInterfaceReference); }
    private native void ViewCurrentCase(long applicationReference);

    public void viewCase(double positionInRepository) { ViewCase(m_nativeEngineInterfaceReference, positionInRepository); }
    private native void ViewCase(long applicationReference, double positionInRepository);
}
