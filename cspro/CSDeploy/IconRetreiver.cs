﻿using System;
using System.Drawing;
using System.Runtime.InteropServices;

/// <summary>
/// Get icon for file based on file extension
/// </summary>
public static class IconRetreiver
{

    [StructLayout(LayoutKind.Sequential)]
    private struct SHFILEINFO
    {
        public IntPtr hIcon;
        public IntPtr iIcon;
        public uint dwAttributes;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string szDisplayName;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 80)]
        public string szTypeName;
    }

    private const Int32 MAX_PATH = 260;

    [StructLayoutAttribute(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct SHSTOCKICONINFO
    {
        public UInt32 cbSize;
        public IntPtr hIcon;
        public Int32 iSysIconIndex;
        public Int32 iIcon;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = MAX_PATH)]
        public string szPath;
    }

    [Flags]
    private enum SHGSI : uint
    {
        SHGSI_ICONLOCATION = 0,
        SHGSI_ICON = 0x000000100,
        SHGSI_SYSICONINDEX = 0x000004000,
        SHGSI_LINKOVERLAY = 0x000008000,
        SHGSI_SELECTED = 0x000010000,
        SHGSI_LARGEICON = 0x000000000,
        SHGSI_SMALLICON = 0x000000001,
        SHGSI_SHELLICONSIZE = 0x000000004
    }

    public enum SHSTOCKICONID : uint
    {
        SIID_DOCNOASSOC = 0,
        SIID_DOCASSOC = 1,
        SIID_APPLICATION = 2,
        SIID_FOLDER = 3,
        SIID_FOLDEROPEN = 4,
        SIID_DRIVE525 = 5,
        SIID_DRIVE35 = 6,
        SIID_DRIVEREMOVE = 7,
        SIID_DRIVEFIXED = 8,
        SIID_DRIVENET = 9,
        SIID_DRIVENETDISABLED = 10,
        SIID_DRIVECD = 11,
        SIID_DRIVERAM = 12,
        SIID_WORLD = 13,
        SIID_SERVER = 15,
        SIID_PRINTER = 16,
        SIID_MYNETWORK = 17,
        SIID_FIND = 22,
        SIID_HELP = 23,
        SIID_SHARE = 28,
        SIID_LINK = 29,
        SIID_SLOWFILE = 30,
        SIID_RECYCLER = 31,
        SIID_RECYCLERFULL = 32,
        SIID_MEDIACDAUDIO = 40,
        SIID_LOCK = 47,
        SIID_AUTOLIST = 49,
        SIID_PRINTERNET = 50,
        SIID_SERVERSHARE = 51,
        SIID_PRINTERFAX = 52,
        SIID_PRINTERFAXNET = 53,
        SIID_PRINTERFILE = 54,
        SIID_STACK = 55,
        SIID_MEDIASVCD = 56,
        SIID_STUFFEDFOLDER = 57,
        SIID_DRIVEUNKNOWN = 58,
        SIID_DRIVEDVD = 59,
        SIID_MEDIADVD = 60,
        SIID_MEDIADVDRAM = 61,
        SIID_MEDIADVDRW = 62,
        SIID_MEDIADVDR = 63,
        SIID_MEDIADVDROM = 64,
        SIID_MEDIACDAUDIOPLUS = 65,
        SIID_MEDIACDRW = 66,
        SIID_MEDIACDR = 67,
        SIID_MEDIACDBURN = 68,
        SIID_MEDIABLANKCD = 69,
        SIID_MEDIACDROM = 70,
        SIID_AUDIOFILES = 71,
        SIID_IMAGEFILES = 72,
        SIID_VIDEOFILES = 73,
        SIID_MIXEDFILES = 74,
        SIID_FOLDERBACK = 75,
        SIID_FOLDERFRONT = 76,
        SIID_SHIELD = 77,
        SIID_WARNING = 78,
        SIID_INFO = 79,
        SIID_ERROR = 80,
        SIID_KEY = 81,
        SIID_SOFTWARE = 82,
        SIID_RENAME = 83,
        SIID_DELETE = 84,
        SIID_MEDIAAUDIODVD = 85,
        SIID_MEDIAMOVIEDVD = 86,
        SIID_MEDIAENHANCEDCD = 87,
        SIID_MEDIAENHANCEDDVD = 88,
        SIID_MEDIAHDDVD = 89,
        SIID_MEDIABLURAY = 90,
        SIID_MEDIAVCD = 91,
        SIID_MEDIADVDPLUSR = 92,
        SIID_MEDIADVDPLUSRW = 93,
        SIID_DESKTOPPC = 94,
        SIID_MOBILEPC = 95,
        SIID_USERS = 96,
        SIID_MEDIASMARTMEDIA = 97,
        SIID_MEDIACOMPACTFLASH = 98,
        SIID_DEVICECELLPHONE = 99,
        SIID_DEVICECAMERA = 100,
        SIID_DEVICEVIDEOCAMERA = 101,
        SIID_DEVICEAUDIOPLAYER = 102,
        SIID_NETWORKCONNECT = 103,
        SIID_INTERNET = 104,
        SIID_ZIPFILE = 105,
        SIID_SETTINGS = 106,
        SIID_DRIVEHDDVD = 132,
        SIID_DRIVEBD = 133,
        SIID_MEDIAHDDVDROM = 134,
        SIID_MEDIAHDDVDR = 135,
        SIID_MEDIAHDDVDRAM = 136,
        SIID_MEDIABDROM = 137,
        SIID_MEDIABDR = 138,
        SIID_MEDIABDRE = 139,
        SIID_CLUSTEREDDRIVE = 140,
        SIID_MAX_ICONS = 175
    };

    private const uint FILE_ATTRIBUTE_NORMAL = 0x80;
    private const uint FILE_ATTRIBUTE_DIRECTORY = 0x10;

    [Flags]
    private enum SHGFI
    {
        SHGFI_ICON = 0x000000100,
        SHGFI_DISPLAYNAME = 0x000000200,
        SHGFI_TYPENAME = 0x000000400,
        SHGFI_ATTRIBUTES = 0x000000800,
        SHGFI_ICONLOCATION = 0x000001000,
        SHGFI_EXETYPE = 0x000002000,
        SHGFI_SYSICONINDEX = 0x000004000,
        SHGFI_LINKOVERLAY = 0x000008000,
        SHGFI_SELECTED = 0x000010000,
        SHGFI_ATTR_SPECIFIED = 0x000020000,
        SHGFI_LARGEICON = 0x000000000,
        SHGFI_SMALLICON = 0x000000001,
        SHGFI_OPENICON = 0x000000002,
        SHGFI_SHELLICONSIZE = 0x000000004,
        SHGFI_PIDL = 0x000000008,
        SHGFI_USEFILEATTRIBUTES = 0x000000010,
        SHGFI_ADDOVERLAYS = 0x000000020,
        SHGFI_OVERLAYINDEX = 0x000000040
    }

    [DllImport("shell32.dll")]
    private static extern IntPtr SHGetFileInfo(string pszPath, uint dwFileAttributes, ref SHFILEINFO psfi, uint cbSizeFileInfo, uint uFlags);

    [DllImport("Shell32.dll")]
    private static extern Int32 SHGetStockIconInfo(SHSTOCKICONID siid, SHGSI uFlags, ref SHSTOCKICONINFO psii);

    [DllImport("User32.dll")]
    private static extern int DestroyIcon(IntPtr hIcon);

    /// <summary>
    /// Get shell icon for a file based on file extension
    /// </summary>
    /// <param name="filePath">Full path of file to get icon for</param>
    /// <returns>Icon for file</returns>
    public static Icon GetIconForFile(string filePath)
    {
        SHFILEINFO shellFileInfo = new SHFILEINFO();
        if ((long) SHGetFileInfo(filePath, FILE_ATTRIBUTE_NORMAL, ref shellFileInfo, (uint)Marshal.SizeOf(shellFileInfo), (uint)(SHGFI.SHGFI_ICON | SHGFI.SHGFI_SMALLICON)) != 0)
        {
            Icon icon = (Icon)System.Drawing.Icon.FromHandle(shellFileInfo.hIcon).Clone();
            DestroyIcon(shellFileInfo.hIcon);
            return icon;
        }
        return null;
    }

    public static Icon GetStockIcon(SHSTOCKICONID siid)
    {
        SHSTOCKICONINFO iconInfo = new SHSTOCKICONINFO();
        iconInfo.cbSize = Convert.ToUInt32(Marshal.SizeOf(typeof(SHSTOCKICONINFO)));

        if (SHGetStockIconInfo(siid, (SHGSI)(SHGSI.SHGSI_ICON | SHGSI.SHGSI_SMALLICON), ref iconInfo) == 0)
        {
            Icon icon = (Icon)System.Drawing.Icon.FromHandle(iconInfo.hIcon).Clone();
            DestroyIcon(iconInfo.hIcon);
            return icon;
        }

        return null;
    }
};