// ============================================================================
//  Redstone Server Panel - Minecraft Server Manager (Windows, Pure Win32 API)
//  System tray support, dark/light themes, bilingual UI (TR/EN),
//  live player skinheads, and DPI-aware modern interface.
//
//  Build (MSVC):
//    rc resource.rc
//    cl /utf-8 /EHsc /O2 redstonesp.cpp resource.res /Fe:RedstoneServerPanel.exe ^
//       /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib shell32.lib ^
//       advapi32.lib psapi.lib winhttp.lib gdiplus.lib shlwapi.lib
//
//  Build (MinGW-w64):
//    windres resource.rc -O coff -o resource.o
//    g++ -O2 -s -std=c++17 redstonesp.cpp resource.o -o RedstoneServerPanel.exe
//        -mwindows -static -lcomdlg32 -lpsapi -lshell32 -ladvapi32 -lgdi32
//        -luser32 -lwinhttp -lgdiplus -lshlwapi
// ============================================================================
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winhttp.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <richedit.h>
#ifndef MSFTEDIT_CLASS
#define MSFTEDIT_CLASS L"RichEdit50W"
#endif
#include <string>
#include <vector>
#include <map>
#include <set>
#include <atomic>
#include <memory>
#include <new>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cwchar>
#include <cwctype>
#include <cctype>

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#endif

// ----------------------------------------------------------------------------
//  Constants & Identifiers
// ----------------------------------------------------------------------------
#define WM_TRAYICON  (WM_USER + 1)
#define WM_SC_EVENT  (WM_USER + 2)   // worker thread -> UI thread
#define WM_SC_SHOW   (WM_USER + 3)   // second instance -> primary instance

enum {
    ID_BTN_BROWSE = 1001, ID_BTN_START, ID_BTN_STOP, ID_BTN_KICK, ID_BTN_BAN,
    ID_BTN_UNBAN, ID_BTN_SEND, ID_BTN_JVMSAVE, ID_CHK_AUTOSTART,
    ID_BTN_PROPSAVE, ID_BTN_PROPRELOAD,
    ID_BTN_GM_SURV, ID_BTN_GM_CREA, ID_BTN_GM_ADV, ID_BTN_GM_SPEC,
    ID_BTN_DF_PEACE, ID_BTN_DF_EASY, ID_BTN_DF_NORM, ID_BTN_DF_HARD,
    ID_CHK_ONLINE, ID_CHK_PVP, ID_CHK_WHITELIST, ID_CHK_FLIGHT, ID_CHK_CMDBLOCK,
    ID_BTN_WL_ADD, ID_BTN_WL_REM, ID_BTN_WL_RELOAD,
    ID_BTN_OP_ADD, ID_BTN_OP_REM, ID_BTN_OP_RELOAD,
    ID_BTN_BK_CREATE, ID_BTN_BK_OPEN, ID_BTN_BK_RESTORE, ID_BTN_BK_DEL, ID_BTN_BK_RELOAD,
    ID_BTN_PROF_SAVE, ID_BTN_PROF_LOAD, ID_BTN_PROF_DEL,
    ID_BTN_PLUG_TOGGLE, ID_BTN_PLUG_ADD, ID_BTN_PLUG_DEL, ID_BTN_PLUG_OPEN, ID_BTN_PLUG_RELOAD,
    ID_BTN_MOD_TOGGLE, ID_BTN_MOD_ADD, ID_BTN_MOD_DEL, ID_BTN_MOD_OPEN, ID_BTN_MOD_RELOAD,
    ID_CHK_SCHED_BK, ID_BTN_SCHED_BK_SAVE,
    ID_CHK_SCHED_RESTART, ID_BTN_SCHED_RESTART_SAVE,
    ID_EDIT_JAR = 1101, ID_EDIT_IP, ID_EDIT_PORT, ID_EDIT_RAM, ID_EDIT_UNBAN,
    ID_EDIT_CMD, ID_EDIT_LOG, ID_LIST_PLAYERS, ID_EDIT_JVM,
    ID_EDIT_MOTD, ID_EDIT_MAXPL, ID_EDIT_VIEWDIST, ID_EDIT_SPAWNPROT,
    ID_EDIT_WL, ID_EDIT_OP, ID_LIST_WL, ID_LIST_OP,
    ID_EDIT_BK_TAG, ID_LIST_BK, ID_EDIT_PROF_NAME, ID_LIST_PROF,
    ID_LIST_PLUGINS, ID_LIST_MODS, ID_EDIT_BK_HOURS, ID_EDIT_RESTART_TIME,
    ID_TRAY_SHOW = 2001, ID_TRAY_START, ID_TRAY_STOP, ID_TRAY_AUTOSTART, ID_TRAY_EXIT,
    ID_MENU_LANG_TR = 2101, ID_MENU_LANG_EN, ID_MENU_THEME_DARK, ID_MENU_THEME_LIGHT,
    ID_MENU_THEME_REDSTONE, ID_MENU_THEME_MC,
    ID_MENU_AUTORESTART, ID_MENU_OPENLOGS
};
enum { TID_STATS = 1, TID_TIME = 2, TID_STOPWATCH = 3, TID_RESTART = 4, TID_ANIM = 5 };

enum class SrvState { Stopped, Starting, Running, Stopping };
enum class Lang { TR, EN };
enum class Theme { Dark, Light, Redstone, Minecraft };
enum EvType { EV_LOG, EV_JOIN, EV_LEAVE, EV_TIME, EV_RETRY_TIME, EV_READY, EV_STOPPING, EV_EXITED, EV_SKIN, EV_SYNC_LIST, EV_BACKUP_DONE };

struct SrvEvent {
    int type;
    int gen;
    std::wstring text;
    HBITMAP hbm = NULL;
    ~SrvEvent() { if (hbm) DeleteObject(hbm); }
};

// ----------------------------------------------------------------------------
//  Theme Palettes (Dark & Light/White)
// ----------------------------------------------------------------------------
struct Palette {
    COLORREF bg, card, field, logBg, border, text, logText, muted, accent;
    COLORREF green, red, amber, sel;
    COLORREF btnDef, btnDefHov, btnDis, btnDisTxt;
    COLORREF tabBar, tabIdle, subBar;
};

static const Palette PAL_DARK = {
    RGB(15, 20, 29),    // bg (main content surface)
    RGB(24, 31, 44),    // card
    RGB(33, 42, 59),    // field
    RGB(11, 15, 22),    // logBg
    RGB(48, 59, 82),    // border
    RGB(230, 236, 246), // text
    RGB(241, 245, 249), // logText (high-contrast crisp bright silver-white)
    RGB(148, 163, 184), // muted
    RGB(56, 189, 248),  // accent (electric cyan #38BDF8 - easily distinguished)
    RGB(74, 222, 128),  // green (vibrant spring green #4ADE80)
    RGB(255, 107, 107), // red (vibrant coral red #FF6B6B)
    RGB(251, 191, 36),  // amber (warm vivid gold #FBBF24)
    RGB(38, 62, 104),   // sel
    RGB(48, 58, 80),    // btnDef
    RGB(62, 74, 100),   // btnDefHov
    RGB(27, 33, 46),    // btnDis
    RGB(86, 96, 118),   // btnDisTxt
    RGB(7, 9, 14),      // tabBar (deep dark top strip, clearly distinct from bg/cards)
    RGB(13, 17, 25),    // tabIdle
    RGB(11, 15, 22)     // subBar
};

static const Palette PAL_LIGHT = {
    RGB(240, 244, 249), // bg
    RGB(255, 255, 255), // card
    RGB(247, 249, 253), // field
    RGB(243, 246, 251), // logBg
    RGB(206, 214, 226), // border
    RGB(15, 23, 42),    // text
    RGB(30, 41, 59),    // logText
    RGB(100, 116, 139), // muted
    RGB(37, 99, 235),   // accent
    RGB(22, 163, 74),   // green
    RGB(220, 38, 38),   // red
    RGB(217, 119, 6),   // amber
    RGB(219, 234, 254), // sel
    RGB(226, 232, 240), // btnDef
    RGB(203, 213, 225), // btnDefHov
    RGB(238, 242, 246), // btnDis
    RGB(148, 163, 184), // btnDisTxt
    RGB(216, 224, 236), // tabBar (distinct cool-gray top strip)
    RGB(226, 233, 243), // tabIdle
    RGB(231, 237, 246)  // subBar
};

static const Palette PAL_REDSTONE = {
    RGB(20, 11, 14),    // bg (deep netherite obsidian surface)
    RGB(30, 17, 22),    // card (crimson slate card)
    RGB(42, 24, 31),    // field (dark nether brick field)
    RGB(13, 7, 9),      // logBg (deep obsidian console)
    RGB(78, 32, 42),    // border (crimson border)
    RGB(248, 235, 238), // text (bright quartz/bone white)
    RGB(248, 235, 238), // logText (high-contrast crisp white)
    RGB(172, 130, 138), // muted (soft dusted rose)
    RGB(255, 68, 68),   // accent (glowing redstone red #FF4444)
    RGB(74, 222, 128),  // green (vibrant emerald green)
    RGB(255, 68, 68),   // red (redstone pulse)
    RGB(251, 191, 36),  // amber (blaze powder gold)
    RGB(76, 22, 30),    // sel (crimson selection)
    RGB(50, 26, 34),    // btnDef
    RGB(70, 36, 48),    // btnDefHov
    RGB(25, 14, 18),    // btnDis
    RGB(108, 76, 84),   // btnDisTxt
    RGB(11, 6, 8),      // tabBar (deep obsidian top strip)
    RGB(20, 11, 14),    // tabIdle
    RGB(15, 8, 11)      // subBar
};

static const Palette PAL_MINECRAFT = {
    RGB(24, 26, 28),    // bg (deepslate / bedrock slate)
    RGB(34, 38, 42),    // card (smooth deepslate card)
    RGB(46, 52, 58),    // field (chiseled stone field)
    RGB(15, 17, 19),    // logBg (dark bedrock console)
    RGB(64, 72, 78),    // border (stone edge border)
    RGB(240, 244, 248), // text (crisp iron/silver white)
    RGB(240, 244, 248), // logText (high-contrast crisp white)
    RGB(148, 158, 166), // muted (soft stone gray)
    RGB(74, 222, 128),  // accent (signature Minecraft grass green #4ADE80)
    RGB(74, 222, 128),  // green (grass block green)
    RGB(248, 113, 113), // red (redstone torch red)
    RGB(250, 204, 21),  // amber (gold ingot gold)
    RGB(38, 64, 46),    // sel (mossy cobblestone selection)
    RGB(48, 54, 60),    // btnDef
    RGB(64, 72, 80),    // btnDefHov
    RGB(26, 29, 32),    // btnDis
    RGB(92, 102, 110),  // btnDisTxt
    RGB(13, 14, 16),    // tabBar (deep bedrock top strip)
    RGB(22, 24, 26),    // tabIdle
    RGB(17, 19, 21)     // subBar
};

static Palette P = PAL_DARK;
static Theme g_theme = Theme::Dark;
static Lang  g_lang  = Lang::TR;
static bool  g_autoRestart = false;                    // restart automatically after a crash
static int   g_crashCount  = 0;                        // consecutive early crashes
static std::wstring g_jvmArgs = L"-XX:+UseG1GC";       // editable via settings.ini (JvmArgs)

// ----------------------------------------------------------------------------
//  Localization Strings (Turkish & English)
// ----------------------------------------------------------------------------
struct Strings {
    const wchar_t* wndTitle;
    const wchar_t* cardSettings;
    const wchar_t* cardPlayers;
    const wchar_t* cardConsole;
    const wchar_t* capJar;
    const wchar_t* capIp;
    const wchar_t* capPort;
    const wchar_t* capRam;
    const wchar_t* capUnban;
    const wchar_t* btnBrowse;
    const wchar_t* btnStart;
    const wchar_t* btnStop;
    const wchar_t* btnKick;
    const wchar_t* btnBan;
    const wchar_t* btnUnban;
    const wchar_t* btnSend;
    const wchar_t* stStatus;
    const wchar_t* stUptime;
    const wchar_t* stMemory;
    const wchar_t* stPlayers;
    const wchar_t* stWorldTime;
    const wchar_t* valStarting;
    const wchar_t* valRunning;
    const wchar_t* valStopping;
    const wchar_t* valStopped;
    const wchar_t* noPlayersOnline;
    const wchar_t* serverOffline;
    const wchar_t* tipStopped;
    const wchar_t* tipStarting;
    const wchar_t* tipRunningFmt; // e.g. "Aktif • %d oyuncu"
    const wchar_t* tipStopping;
    const wchar_t* balloonTrayBg;
    const wchar_t* balloonTrayStart;
    const wchar_t* dlgFilterJar;
    const wchar_t* dlgTitleJar;
    const wchar_t* msgAlreadyRunning;
    const wchar_t* msgSelectJar;
    const wchar_t* titleMissingInfo;
    const wchar_t* msgJarNotFound;
    const wchar_t* titleFileNotFound;
    const wchar_t* msgInvalidRam;
    const wchar_t* msgInvalidPort;
    const wchar_t* msgInvalidIp;
    const wchar_t* titleInvalidVal;
    const wchar_t* msgJavaNotFound;
    const wchar_t* titleJavaMissing;
    const wchar_t* msgEulaPrompt;
    const wchar_t* titleEula;
    const wchar_t* msgPipeErr;
    const wchar_t* msgStartErr;
    const wchar_t* msgThreadErr;
    const wchar_t* titleError;
    const wchar_t* logStarting;
    const wchar_t* logWorkDir;
    const wchar_t* balloonStarting;
    const wchar_t* logStopping;
    const wchar_t* logCrashed;
    const wchar_t* balloonCrashed;
    const wchar_t* logStopped;
    const wchar_t* balloonStopped;
    const wchar_t* msgExitConfirm;
    const wchar_t* titleExit;
    const wchar_t* logSelectPlayer;
    const wchar_t* msgBanConfirm;
    const wchar_t* titleBanConfirm;
    const wchar_t* logInvalidPlayer;
    const wchar_t* logUnbanReq;
    const wchar_t* logSrvNotRunning;
    const wchar_t* logAppReady;
    const wchar_t* trayShow;
    const wchar_t* trayHide;
    const wchar_t* trayStart;
    const wchar_t* trayStop;
    const wchar_t* menuAutoStart;
    const wchar_t* trayExit;
    const wchar_t* menuLang;
    const wchar_t* menuTheme;
    const wchar_t* menuThemeDark;
    const wchar_t* menuThemeLight;
    const wchar_t* menuThemeRedstone;
    const wchar_t* menuThemeMc;
    const wchar_t* balloonJoinTitle;
    const wchar_t* balloonJoinSuffix;
    const wchar_t* balloonLeaveTitle;
    const wchar_t* balloonLeaveSuffix;
    const wchar_t* logReady;
    const wchar_t* balloonReady;
    const wchar_t* msgTimeoutKill;
    const wchar_t* titleTimeout;
};

static const Strings STR_TR = {
    L"Redstone Server Panel — Minecraft Sunucu Yöneticisi",
    L"SUNUCU AYARLARI",
    L"OYUNCULAR",
    L"KONSOL",
    L"Sunucu .jar dosyası",
    L"IP adresi (isteğe bağlı)",
    L"Port",
    L"Bellek (ör: 4G, 2048M)",
    L"Banı kaldır (oyuncu adı)",
    L"Seç",
    L"Başlat",
    L"Güvenle Durdur",
    L"At (Kick)",
    L"Banla",
    L"Banı Kaldır",
    L"Gönder",
    L"DURUM",
    L"ÇALIŞMA SÜRESİ",
    L"BELLEK",
    L"OYUNCU",
    L"OYUN SAATİ",
    L"BAŞLIYOR",
    L"AKTİF",
    L"KAPANIYOR",
    L"KAPALI",
    L"Şu an çevrimiçi oyuncu yok",
    L"Sunucu kapalı",
    L"Kapalı",
    L"Başlatılıyor…",
    L"Aktif • ",
    L"Kapatılıyor…",
    L"Arka planda çalışmaya devam ediyor. Simgeye tıklayarak geri dönebilirsiniz.",
    L"Sistem tepsisinde çalışıyor.",
    L"Java arşivi (*.jar)\0*.jar\0Tüm dosyalar (*.*)\0*.*\0\0",
    L"Sunucu .jar dosyasını seçin",
    L"Sunucu zaten çalışıyor.",
    L"Lütfen sunucu .jar dosyasını seçin.",
    L"Eksik bilgi",
    L"Seçilen .jar dosyası bulunamadı:\n",
    L"Dosya yok",
    L"RAM değeri geçersiz. Örnek: 4G veya 2048M",
    L"Port 1-65535 arasında bir sayı olmalı.",
    L"IP adresi geçersiz karakterler içeriyor.",
    L"Geçersiz değer",
    L"Java bulunamadı. Lütfen Java 17 veya üstünü kurun ve PATH / JAVA_HOME ayarlayın.",
    L"Java yok",
    L"Minecraft sunucusunu çalıştırmak için EULA'yı kabul etmelisiniz:\nhttps://aka.ms/MinecraftEULA\n\nKabul ediyor musunuz?",
    L"Minecraft EULA",
    L"Boru (pipe) oluşturulamadı.",
    L"Sunucu başlatılamadı:\n",
    L"Okuyucu thread başlatılamadı.",
    L"Hata",
    L"Sunucu başlatılıyor… (",
    L" RAM, çalışma dizini: ",
    L"Sunucu başlatılıyor…",
    L"Güvenli kapatma isteği gönderildi (dünya kaydediliyor)…",
    L"Sunucu beklenmedik şekilde kapandı (çıkış kodu ",
    L"Sunucu beklenmedik şekilde kapandı!",
    L"Sunucu kapandı.",
    L"Sunucu güvenle kapatıldı.",
    L"Sunucu hâlâ çalışıyor. Güvenle kapatılıp program sonlandırılsın mı?",
    L"Çıkış",
    L"Önce listeden bir oyuncu seçin.",
    L" kalıcı olarak banlansın mı?",
    L"Ban onayı",
    L"Geçersiz oyuncu adı.",
    L"Ban kaldırma isteği: ",
    L"Sunucu çalışmıyor; komut gönderilemedi.",
    L"Redstone Server Panel hazır. .jar dosyasını seçip “Başlat”a basın.",
    L"Pencereyi Göster",
    L"Pencereyi Gizle",
    L"Sunucuyu Başlat",
    L"Sunucuyu Güvenle Durdur",
    L"Windows ile başlat",
    L"Çıkış",
    L"Dil / Language",
    L"Tema",
    L"Koyu Tema (Varsayılan)",
    L"Açık Tema (Beyaz)",
    L"Redstone Teması",
    L"Minecraft Teması",
    L"Oyuncu katıldı",
    L" sunucuya girdi.",
    L"Oyuncu ayrıldı",
    L" sunucudan çıktı.",
    L"Sunucu hazır, oyuncular bağlanabilir.",
    L"Sunucu hazır — oyuncular bağlanabilir!",
    L"Sunucu 60 saniyedir kapanmadı. Zorla sonlandırılsın mı?\n(Kaydedilmemiş dünya verileri kaybolabilir.)",
    L"Kapanma zaman aşımı"
};

static const Strings STR_EN = {
    L"Redstone Server Panel — Minecraft Server Manager",
    L"SERVER SETTINGS",
    L"PLAYERS",
    L"CONSOLE",
    L"Server .jar file",
    L"IP address (optional)",
    L"Port",
    L"Memory (e.g. 4G, 2048M)",
    L"Unban (player name)",
    L"Browse",
    L"Start",
    L"Stop Safely",
    L"Kick",
    L"Ban",
    L"Unban",
    L"Send",
    L"STATUS",
    L"UPTIME",
    L"MEMORY",
    L"PLAYERS",
    L"WORLD TIME",
    L"STARTING",
    L"ONLINE",
    L"STOPPING",
    L"OFFLINE",
    L"No players currently online",
    L"Server is offline",
    L"Offline",
    L"Starting…",
    L"Online • ",
    L"Stopping…",
    L"Still running in the background. Click the tray icon to restore.",
    L"Running in system tray.",
    L"Java Archive (*.jar)\0*.jar\0All Files (*.*)\0*.*\0\0",
    L"Select server .jar file",
    L"Server is already running.",
    L"Please select the server .jar file.",
    L"Missing Information",
    L"Selected .jar file was not found:\n",
    L"File Not Found",
    L"Invalid RAM value. Example: 4G or 2048M",
    L"Port must be a number between 1 and 65535.",
    L"IP address contains invalid characters.",
    L"Invalid Value",
    L"Java was not found. Please install Java 17+ and configure PATH / JAVA_HOME.",
    L"Java Missing",
    L"You must accept the Minecraft EULA to run the server:\nhttps://aka.ms/MinecraftEULA\n\nDo you accept?",
    L"Minecraft EULA",
    L"Failed to create pipe.",
    L"Failed to start server:\n",
    L"Failed to start reader thread.",
    L"Error",
    L"Starting server… (",
    L" RAM, working directory: ",
    L"Starting server…",
    L"Safe shutdown requested (saving world)…",
    L"Server exited unexpectedly (exit code ",
    L"Server stopped unexpectedly!",
    L"Server stopped.",
    L"Server stopped safely.",
    L"The server is still running. Safely stop the server and exit?",
    L"Exit",
    L"Please select a player from the list first.",
    L" — permanently ban this player?",
    L"Confirm Ban",
    L"Invalid player name.",
    L"Unban request sent: ",
    L"Server is not running; command could not be sent.",
    L"Redstone Server Panel is ready. Select a .jar file and click “Start”.",
    L"Show Window",
    L"Hide Window",
    L"Start Server",
    L"Stop Server Safely",
    L"Start with Windows",
    L"Exit",
    L"Language / Dil",
    L"Theme",
    L"Dark Theme (Default)",
    L"Light Theme (White)",
    L"Redstone Theme",
    L"Minecraft Theme",
    L"Player Joined",
    L" joined the server.",
    L"Player Left",
    L" left the server.",
    L"Server is ready, players can now connect.",
    L"Server is ready — players can now join!",
    L"The server has not stopped after 60 seconds. Force terminate?\n(Unsaved world data may be lost.)",
    L"Shutdown Timeout"
};

static const Strings& L() { return g_lang == Lang::TR ? STR_TR : STR_EN; }

// ----------------------------------------------------------------------------
//  Global State
// ----------------------------------------------------------------------------
static HINSTANCE g_hInst;
static HWND g_hwnd;
static int  g_dpi = 96;
static ULONG_PTR g_gdiplusToken = 0;

static HWND g_hJar, g_hIp, g_hPort, g_hRam, g_hUnban, g_hCmd, g_hLog, g_hList;
static HWND g_hBtnBrowse, g_hBtnStart, g_hBtnStop, g_hBtnKick, g_hBtnBan, g_hBtnUnban, g_hBtnSend;
static HWND g_hHover = NULL;

static HFONT g_fUI, g_fSemi, g_fCap, g_fVal, g_fMono;
static HBRUSH g_bBg = NULL, g_bField = NULL, g_bLog = NULL;
static int g_textH = 16;

static WNDPROC g_oldEdit = NULL, g_oldBtn = NULL;

static SrvState g_state = SrvState::Stopped;
static HANDLE g_hProc = NULL, g_hStdinWr = NULL;
static int g_gen = 0;
static std::atomic<int> g_pendingTime{0};
static std::atomic<bool> g_useTimelineCmd{false};
static std::atomic<int> g_suppressErrLines{0};
static ULONGLONG g_startTick = 0;
static bool g_quitPending = false;
static bool g_trayHintShown = false;

static std::wstring g_igTime = L"--:--", g_ramText = L"0 MB", g_upText = L"00:00:00";
static std::vector<std::wstring> g_hist;
static int g_histPos = 0;

// Skinhead cache & pending downloads
static std::map<std::wstring, HBITMAP> g_skinCache;
static std::map<std::wstring, HBITMAP> g_fallbackCache;
static std::set<std::wstring> g_skinPending;
static std::map<std::wstring, std::wstring> g_playerUuids;
static CRITICAL_SECTION g_csUuid;

// Tray
static NOTIFYICONDATAW g_nid = {};
static HICON g_icoBig, g_icoSmall, g_trayIco[4];
static UINT g_msgTaskbarCreated = 0;
static std::wstring g_lastTip;
static int g_lastTrayIcon = -1;

// Layout
enum { F_JAR, F_IP, F_PORT, F_RAM, F_UNBAN, F_CMD, F_JVM, F_MOTD, F_MAXPL, F_VIEWDIST, F_SPAWNPROT, F_WL, F_OP, F_BK_TAG, F_PROF_NAME, F_BK_HOURS, F_RESTART_TIME, F_COUNT };
struct Field { HWND h; RECT rc; const wchar_t* caption; };
static Field g_f[F_COUNT];
static RECT g_rcSet, g_rcPl, g_rcCon, g_rcStats, g_rcTile[5], g_rcListBg, g_rcLogBg;
static RECT g_rcWlBg, g_rcOpBg, g_rcBkBg, g_rcProfBg, g_rcPlugBg, g_rcModBg;

// Window / page navigation (tab bar + title bar + sub-tabs + card pages)
enum { PG_PANEL, PG_PROPS, PG_ACCESS, PG_BACKUPS, PG_PLUGINS, PG_PROFILES, PG_SETTINGS, PG_COUNT };
struct TxtBlock { RECT rc; int id; };
static int g_page = PG_PANEL, g_subTab = 0;
static RECT g_rcTab[PG_COUNT], g_rcSub[2];
static float g_tabCurX = -1.0f, g_tabCurW = -1.0f;
static float g_tabTargetX = 0.0f, g_tabTargetW = 0.0f;
static bool  g_animatingTab = false;
static int   g_tabHover = -1;
static int   g_subTabHover = -1;
static bool  g_logThemeDirty = false;
static LARGE_INTEGER g_animPerfFreq = {};
static LARGE_INTEGER g_animLastTime = {};
static void  ShowPage();
static std::vector<TxtBlock> g_scards, g_txts;       // active page cards (title id) and description blocks
static bool g_autoStartCache = false;
static HWND g_hJvm, g_hBtnJvmSave, g_hChkAutoStart, g_hChkAutoRestart;
static HWND g_hBtnLangTR, g_hBtnLangEN, g_hBtnThDark, g_hBtnThLight, g_hBtnThRedstone, g_hBtnThMc, g_hBtnLogs;

// Automation state & controls (Settings -> Server)
static bool g_schedBkEnabled = false;
static int  g_schedBkHours = 2;
static bool g_schedRestartEnabled = false;
static std::wstring g_schedRestartTime = L"04:00";
static time_t g_lastAutoBackupTime = 0;
static int  g_lastRestartMinute = -1;
static int  g_restartWarningStep = 0;
static bool g_schedRestartPending = false;
static bool g_scheduledRestartTrigger = false;
static HWND g_hChkSchedBk, g_hEditBkHours, g_hBtnSchedBkSave;
static HWND g_hChkSchedRestart, g_hEditRestartTime, g_hBtnSchedRestartSave;

// Server Properties page state & controls
static HWND g_hMotd, g_hMaxPl, g_hViewDist, g_hSpawnProt;
static HWND g_hBtnPropSave, g_hBtnPropReload;
static HWND g_hBtnGm[4], g_hBtnDf[4];
static HWND g_hChkOnline, g_hChkPvp, g_hChkWhitelist, g_hChkFlight, g_hChkCmdBlock;
static int  g_propGm = 0; // 0=survival, 1=creative, 2=adventure, 3=spectator
static int  g_propDf = 2; // 0=peaceful, 1=easy, 2=normal, 3=hard
static bool g_propOnline = true, g_propPvp = true, g_propWhitelist = false, g_propFlight = false, g_propCmdBlock = false;

// Access (Whitelist & OP) page controls
static HWND g_hListWl, g_hEditWl, g_hBtnWlAdd, g_hBtnWlRem, g_hBtnWlReload;
static HWND g_hListOp, g_hEditOp, g_hBtnOpAdd, g_hBtnOpRem, g_hBtnOpReload;

// Backups page controls & state
static HWND g_hEditBkTag, g_hBtnBkCreate, g_hBtnBkOpen;
static HWND g_hListBk, g_hBtnBkRestore, g_hBtnBkDel, g_hBtnBkReload;
static std::vector<std::wstring> g_backupFiles; // filenames inside <serverDir>\backups
static std::atomic<bool> g_backupBusy{false};

// Plugins & Mods page controls & state
struct ModItem {
    std::wstring filename;
    std::wstring displayName;
    std::wstring fullPath;
    bool enabled;
    std::wstring sizeStr;
};
static std::vector<ModItem> g_pluginsList;
static std::vector<ModItem> g_modsList;
static HWND g_hListPlugins, g_hBtnPlugToggle, g_hBtnPlugAdd, g_hBtnPlugDel, g_hBtnPlugOpen, g_hBtnPlugReload;
static HWND g_hListMods,    g_hBtnModToggle,  g_hBtnModAdd,  g_hBtnModDel,  g_hBtnModOpen,  g_hBtnModReload;

// Profiles page controls & state
struct ServerProfile { std::wstring name, jar, ip, port, ram, jvm; };
static std::vector<ServerProfile> g_profiles;   // parallel to g_hListProf items
static HWND g_hEditProfName, g_hBtnProfSave, g_hListProf, g_hBtnProfLoad, g_hBtnProfDel;

static int S(int v) { return MulDiv(v, g_dpi, 96); }
static int HeaderH() { return S(46) + S(38) + 1; }
static const wchar_t* Tx(const wchar_t* tr, const wchar_t* en) { return g_lang == Lang::TR ? tr : en; }
static const wchar_t* PageName(int pg) {
    switch (pg) {
        case PG_PANEL:    return Tx(L"Panel", L"Dashboard");
        case PG_PROPS:    return Tx(L"Özellikler", L"Properties");
        case PG_ACCESS:   return Tx(L"Yetkiler", L"Access");
        case PG_BACKUPS:  return Tx(L"Yedekler", L"Backups");
        case PG_PLUGINS:  return Tx(L"Eklenti & Mod", L"Plugins & Mods");
        case PG_PROFILES: return Tx(L"Profiller", L"Profiles");
        default:          return Tx(L"Ayarlar", L"Settings");
    }
}
static const wchar_t* CardTitle(int id) {
    switch (id) {
        case 0: return Tx(L"BAŞLANGIÇ", L"STARTUP");
        case 1: return Tx(L"DİL", L"LANGUAGE");
        case 2: return Tx(L"TEMA", L"THEME");
        case 3: return Tx(L"OTOMATİK YENİDEN BAŞLATMA", L"AUTO-RESTART");
        case 4: return Tx(L"JVM ARGÜMANLARI", L"JVM ARGUMENTS");
        case 5: return Tx(L"LOGLAR", L"LOGS");
        case 6: return Tx(L"SUNUCU BİLGİLERİ (SERVER.PROPERTIES)", L"SERVER INFO (SERVER.PROPERTIES)");
        case 7: return Tx(L"OYUN MODU & ZORLUK", L"GAMEMODE & DIFFICULTY");
        case 8: return Tx(L"KURALLAR & GÜVENLİK", L"RULES & SECURITY");
        case 9: return Tx(L"WHITELIST LİSTESİ (WHITELIST.JSON)", L"WHITELIST (WHITELIST.JSON)");
        case 10: return Tx(L"OPERATÖRLER / OP LİSTESİ (OPS.JSON)", L"OPERATORS / OPS (OPS.JSON)");
        case 11: return Tx(L"DÜNYA YEDEKLEME", L"WORLD BACKUP");
        case 12: return Tx(L"KAYITLI YEDEKLER (BACKUPS\\)", L"SAVED BACKUPS (BACKUPS\\)");
        case 13: return Tx(L"YENİ PROFİL KAYDET", L"SAVE NEW PROFILE");
        case 14: return Tx(L"KAYITLI SUNUCU PROFİLLERİ", L"SAVED SERVER PROFILES");
        case 15: return Tx(L"OTOMATİK ZAMANLANMIŞ YEDEKLEME", L"SCHEDULED AUTO-BACKUP");
        case 16: return Tx(L"GÜNLÜK YENİDEN BAŞLATMA", L"DAILY SCHEDULED RESTART");
        case 17: return Tx(L"EKLENTİLER (PLUGINS\\)", L"PLUGINS (PLUGINS\\)");
        case 18: return Tx(L"MODLAR (MODS\\)", L"MODS (MODS\\)");
        default: return L"";
    }
}
static const wchar_t* DescText(int id) {
    switch (id) {
        case 0: return Tx(L"Sunucu beklenmedik şekilde kapanırsa 10 saniye sonra yeniden başlatılır. Art arda 3 çökmeden sonra vazgeçilir.",
                          L"If the server stops unexpectedly it is restarted after 10 seconds. Gives up after 3 consecutive crashes.");
        case 1: return Tx(L"Sunucu başlatılırken kullanılacak ek Java argümanları. Değişiklik bir sonraki başlatmada geçerli olur.",
                          L"Extra Java arguments used when the server starts. Changes apply on the next start.");
        case 2: return Tx(L"Konsol çıktısı console.log dosyasına kaydedilir.",
                          L"Console output is saved to console.log.");
        case 3: return Tx(L"Varsayılan oyun modu (gamemode)", L"Default game mode (gamemode)");
        case 4: return Tx(L"Oyun zorluğu (difficulty)", L"World difficulty (difficulty)");
        case 5: return Tx(L"Not: Bazı ayarların tam geçerli olması için sunucuyu yeniden başlatmanız gerekir.",
                          L"Note: Some settings require a server restart to take full effect.");
        case 6: return Tx(L"Dünya klasörlerini (world, nether, end) .zip olarak sunucu klasöründeki 'backups' dizinine yedekler. Sunucu açıksa önce otomatik 'save-all' gönderilir.",
                          L"Compresses world folders (world, nether, end) into a .zip inside the server's 'backups' folder. Sends 'save-all' first if the server is running.");
        case 7: return Tx(L"Panel'deki .jar yolu, IP, port, RAM ve JVM argümanlarını bir profil olarak saklar. Farklı sunucular arasında tek tıkla geçiş yapabilirsiniz.",
                          L"Stores the jar path, IP, port, RAM and JVM arguments from the Dashboard as a profile so you can switch between servers with one click.");
        case 8: return Tx(L"Sunucu çalışırken belirlenen saat aralığında (saat cinsinden) arka planda otomatik olarak dünya yedeği alır.",
                          L"Automatically takes world backups at the specified hour interval while the server is running.");
        case 9: return Tx(L"Her gün belirtilen saatte (SS:DD) oyunculara 5 ve 1 dakika kala duyuru yaparak sunucuyu güvenle yeniden başlatır.",
                          L"Safely restarts the server daily at the specified time (HH:MM) with 5-minute and 1-minute countdown announcements.");
        case 10: return Tx(L"plugins\\ klasöründeki eklentiler. Devre dışı bırakılanlar .disabled uzantısı alır, ayarları silinmez.",
                           L"Plugins in plugins\\ folder. Disabled plugins get .disabled extension and configurations are kept.");
        case 11: return Tx(L"mods\\ klasöründeki modlar. Devre dışı bırakılan modlar .disabled uzantısı alır.",
                           L"Mods in mods\\ folder. Disabled mods get .disabled extension.");
        default: return L"";
    }
}

// ----------------------------------------------------------------------------
//  Helpers
// ----------------------------------------------------------------------------
static std::wstring GetText(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, L'\0');
    GetWindowTextW(h, &s[0], n + 1);
    s.resize(wcslen(s.c_str()));
    return s;
}
static std::wstring Trim(const std::wstring& s) {
    size_t a = s.find_first_not_of(L" \t\r\n");
    if (a == std::wstring::npos) return L"";
    size_t b = s.find_last_not_of(L" \t\r\n");
    return s.substr(a, b - a + 1);
}
static std::string TrimA(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
static std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, NULL, NULL);
    return s;
}
static std::wstring FromUtf8(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), (int)s.size(), NULL, 0);
    UINT cp = CP_UTF8;
    if (n == 0) { cp = CP_ACP; n = MultiByteToWideChar(cp, 0, s.c_str(), (int)s.size(), NULL, 0); }
    if (n == 0) return L"";
    std::wstring w(n, L'\0');
    MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}
static std::wstring ToLowerW(std::wstring s) {
    for (wchar_t& c : s) c = (wchar_t)towlower(c);
    return s;
}
static std::string ToLowerA(std::string s) {
    for (char& c : s) c = (char)tolower((unsigned char)c);
    return s;
}
static std::wstring Pad2(int v) { return (v < 10 ? L"0" : L"") + std::to_wstring(v); }

static std::wstring SysError(DWORD e) {
    wchar_t* buf = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, e, 0, (LPWSTR)&buf, 0, NULL);
    std::wstring s = buf ? Trim(buf) : L"";
    if (buf) LocalFree(buf);
    return s;
}

static int Msg(const std::wstring& text, const wchar_t* title, UINT flags = MB_OK | MB_ICONINFORMATION) {
    bool visible = g_hwnd && IsWindowVisible(g_hwnd);
    HWND owner = visible ? g_hwnd : NULL;
    // When the panel is hidden in the tray there is no owner window, so make the
    // dialog topmost to ensure it is not lost behind other windows.
    return MessageBoxW(owner, text.c_str(), title, flags | MB_SETFOREGROUND | (visible ? 0 : MB_TOPMOST));
}

static bool ReadAll(const std::wstring& path, std::string& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    char b[8192]; DWORD n = 0; out.clear();
    BOOL ok = FALSE;
    while ((ok = ReadFile(h, b, sizeof(b), &n, NULL)) && n > 0) out.append(b, n);
    CloseHandle(h);
    return ok != FALSE;
}
static bool WriteAll(const std::wstring& path, const std::string& data) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    size_t off = 0; bool ok = true;
    while (off < data.size()) {
        DWORD w = 0;
        if (!WriteFile(h, data.data() + off, (DWORD)(data.size() - off), &w, NULL) || w == 0) { ok = false; break; }
        off += w;
    }
    CloseHandle(h);
    return ok;
}

// ----------------------------------------------------------------------------
//  Settings (%APPDATA%\RedstoneServerPanel\settings.ini, UTF-16)
// ----------------------------------------------------------------------------
static Lang DetectSystemLanguage() {
    LANGID id = GetUserDefaultUILanguage();
    return (PRIMARYLANGID(id) == LANG_TURKISH) ? Lang::TR : Lang::EN;
}

static std::wstring SettingsPath() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"APPDATA", buf, MAX_PATH);
    std::wstring base = (n > 0 && n < MAX_PATH) ? std::wstring(buf) : L".";
    std::wstring dir = base + L"\\RedstoneServerPanel";
    CreateDirectoryW(dir.c_str(), NULL);
    std::wstring path = dir + L"\\settings.ini";
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        WriteAll(path, std::string("\xFF\xFE", 2));      // UTF-16 LE BOM
    }
    return path;
}
static std::wstring IniGet(const wchar_t* key, const wchar_t* def) {
    wchar_t b[2048];
    GetPrivateProfileStringW(L"Server", key, def, b, 2048, SettingsPath().c_str());
    return b;
}
static void LoadAppPrefs() {
    std::wstring langStr = ToLowerW(Trim(IniGet(L"Lang", L"")));
    if (langStr == L"tr") g_lang = Lang::TR;
    else if (langStr == L"en") g_lang = Lang::EN;
    else g_lang = DetectSystemLanguage();

    std::wstring themeStr = ToLowerW(Trim(IniGet(L"Theme", L"dark")));
    if (themeStr == L"light") g_theme = Theme::Light;
    else if (themeStr == L"redstone") g_theme = Theme::Redstone;
    else if (themeStr == L"minecraft") g_theme = Theme::Minecraft;
    else g_theme = Theme::Dark;

    switch (g_theme) {
        case Theme::Light:     P = PAL_LIGHT; break;
        case Theme::Redstone:  P = PAL_REDSTONE; break;
        case Theme::Minecraft: P = PAL_MINECRAFT; break;
        case Theme::Dark:
        default:               P = PAL_DARK; break;
    }

    g_autoRestart = Trim(IniGet(L"AutoRestart", L"0")) == L"1";
    std::wstring ja = Trim(IniGet(L"JvmArgs", L"-XX:+UseG1GC"));
    if (ja.find(L'"') == std::wstring::npos && ja.size() < 512) g_jvmArgs = ja;

    g_schedBkEnabled = Trim(IniGet(L"SchedBkEnabled", L"0")) == L"1";
    g_schedBkHours = _wtoi(IniGet(L"SchedBkHours", L"2").c_str());
    if (g_schedBkHours <= 0) g_schedBkHours = 2;

    g_schedRestartEnabled = Trim(IniGet(L"SchedRestartEnabled", L"0")) == L"1";
    g_schedRestartTime = Trim(IniGet(L"SchedRestartTime", L"04:00"));
    if (g_schedRestartTime.empty()) g_schedRestartTime = L"04:00";
}
static void LoadSettings() {
    SetWindowTextW(g_hJar,  IniGet(L"Jar", L"").c_str());
    SetWindowTextW(g_hIp,   IniGet(L"Ip", L"").c_str());
    SetWindowTextW(g_hPort, IniGet(L"Port", L"25565").c_str());
    SetWindowTextW(g_hRam,  IniGet(L"Ram", L"4G").c_str());
    if (g_hJvm) SetWindowTextW(g_hJvm, g_jvmArgs.c_str());
    if (g_hEditBkHours) SetWindowTextW(g_hEditBkHours, std::to_wstring(g_schedBkHours).c_str());
    if (g_hEditRestartTime) SetWindowTextW(g_hEditRestartTime, g_schedRestartTime.c_str());
}
static void SaveSettings() {
    std::wstring p = SettingsPath();
    if (g_hJar) {
        WritePrivateProfileStringW(L"Server", L"Jar",  Trim(GetText(g_hJar)).c_str(),  p.c_str());
        WritePrivateProfileStringW(L"Server", L"Ip",   Trim(GetText(g_hIp)).c_str(),   p.c_str());
        WritePrivateProfileStringW(L"Server", L"Port", Trim(GetText(g_hPort)).c_str(), p.c_str());
        WritePrivateProfileStringW(L"Server", L"Ram",  Trim(GetText(g_hRam)).c_str(),  p.c_str());
    }
    WritePrivateProfileStringW(L"Server", L"Lang",  g_lang == Lang::TR ? L"tr" : L"en", p.c_str());
    const wchar_t* thVal = L"dark";
    if (g_theme == Theme::Light) thVal = L"light";
    else if (g_theme == Theme::Redstone) thVal = L"redstone";
    else if (g_theme == Theme::Minecraft) thVal = L"minecraft";
    WritePrivateProfileStringW(L"Server", L"Theme", thVal, p.c_str());
    WritePrivateProfileStringW(L"Server", L"AutoRestart", g_autoRestart ? L"1" : L"0", p.c_str());
    WritePrivateProfileStringW(L"Server", L"JvmArgs", g_jvmArgs.c_str(), p.c_str());
    WritePrivateProfileStringW(L"Server", L"SchedBkEnabled", g_schedBkEnabled ? L"1" : L"0", p.c_str());
    WritePrivateProfileStringW(L"Server", L"SchedBkHours", std::to_wstring(g_schedBkHours).c_str(), p.c_str());
    WritePrivateProfileStringW(L"Server", L"SchedRestartEnabled", g_schedRestartEnabled ? L"1" : L"0", p.c_str());
    WritePrivateProfileStringW(L"Server", L"SchedRestartTime", g_schedRestartTime.c_str(), p.c_str());
}

// Directory holding settings.ini and console.log (%APPDATA%\RedstoneServerPanel)
static std::wstring AppDataDir() {
    std::wstring p = SettingsPath();
    size_t sl = p.find_last_of(L'\\');
    return sl == std::wstring::npos ? L"." : p.substr(0, sl);
}

// Appends console output to console.log (UTF-8). The file is rotated once at startup if > 5 MB.
static void LogToFile(const std::wstring& text) {
    static std::wstring path;
    if (path.empty()) {
        path = AppDataDir() + L"\\console.log";
        WIN32_FILE_ATTRIBUTE_DATA fa;
        if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fa) && fa.nFileSizeHigh == 0 && fa.nFileSizeLow > 5u * 1024 * 1024)
            MoveFileExW(path.c_str(), (path + L".old").c_str(), MOVEFILE_REPLACE_EXISTING);
    }
    HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    std::string s = ToUtf8(text); DWORD w = 0;
    WriteFile(h, s.data(), (DWORD)s.size(), &w, NULL);
    CloseHandle(h);
}

// Start with Windows (HKCU\...\Run)
static const wchar_t* RUN_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static bool IsAutoStart() {
    HKEY k; if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_READ, &k) != ERROR_SUCCESS) return false;
    bool r = RegQueryValueExW(k, L"RedstoneServerPanel", NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
    RegCloseKey(k); return r;
}
static void SetAutoStart(bool on) {
    HKEY k; if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &k) != ERROR_SUCCESS) return;
    if (on) {
        wchar_t exe[MAX_PATH];
        DWORD n = GetModuleFileNameW(NULL, exe, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            std::wstring v = L"\"" + std::wstring(exe, n) + L"\" --tray";
            RegSetValueExW(k, L"RedstoneServerPanel", 0, REG_SZ, (const BYTE*)v.c_str(), (DWORD)((v.size() + 1) * sizeof(wchar_t)));
        }
    } else RegDeleteValueW(k, L"RedstoneServerPanel");
    RegCloseKey(k);
}

// ----------------------------------------------------------------------------
//  Procedural Icons & Minecraft Skinheads
// ----------------------------------------------------------------------------
static HICON MakeIcon(int size, int dot /*0 none, 1 gray, 2 green, 3 amber*/) {
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = size; bi.bmiHeader.biHeight = -size;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    void* bits = NULL;
    HDC dc = GetDC(NULL);
    HBITMAP color = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (dc) ReleaseDC(NULL, dc);
    if (!color || !bits) {
        if (color) DeleteObject(color);
        return NULL;
    }
    DWORD* px = (DWORD*)bits;
    auto pack = [](int r, int g, int b) -> DWORD { return 0xFF000000u | (r << 16) | (g << 8) | b; };
    for (int y = 0; y < size; y++) for (int x = 0; x < size; x++) {
        int u = x * 16 / size, v = y * 16 / size;
        unsigned n = (unsigned)((u * 73856093) ^ (v * 19349663)) & 0xFF;
        DWORD c;
        bool corner = (u == 0 || u == 15) && (v == 0 || v == 15);
        if (corner) c = 0;
        else if (u == 0 || u == 15 || v == 0 || v == 15) c = pack(30, 24, 18);
        else if (v <= 4 || (v == 5 && n % 100 < 50)) {
            int d = (n % 3 == 0) ? -16 : (n % 3 == 1 ? 0 : 14);
            c = pack(86 + d, 160 + d, 58 + d);
        } else {
            int d = (n % 3 == 0) ? -12 : (n % 3 == 1 ? 0 : 10);
            c = (n % 23 == 0) ? pack(112, 112, 116) : pack(134 + d, 96 + d, 66 + d);
        }
        if (dot > 0) {
            double fx = x * 16.0 / size - 12.0, fy = y * 16.0 / size - 12.0;
            double dist = fx * fx + fy * fy;
            if (dist <= 3.6 * 3.6) {
                if (dist > 2.6 * 2.6) c = pack(16, 18, 24);
                else c = dot == 1 ? pack(150, 156, 168) : dot == 2 ? pack(60, 224, 112) : pack(250, 190, 40);
            }
        }
        px[y * size + x] = c;
    }
    std::vector<BYTE> maskBits(((size + 15) / 16) * 2 * size, 0);
    HBITMAP mask = CreateBitmap(size, size, 1, 1, maskBits.data());
    if (!mask) {
        DeleteObject(color);
        return NULL;
    }
    ICONINFO ii = { TRUE, 0, 0, mask, color };
    HICON ic = CreateIconIndirect(&ii);
    DeleteObject(mask); DeleteObject(color);
    return ic;
}

// Generates a crisp 32x32 pixel-art Minecraft skinhead (Steve/Alex style) as immediate fallback
static HBITMAP MakeDefaultSkinHead(const std::wstring& name) {
    unsigned h = 2166136261u;
    for (wchar_t c : ToLowerW(name)) h = (h ^ (unsigned)c) * 16777619u;

    struct SkinTone { DWORD hair, hairDark, skin, skinShade, eye, mouth; };
    static const SkinTone tones[] = {
        // Classic Steve
        { 0xFF4A3423, 0xFF3A281B, 0xFFB9856C, 0xFFA4725A, 0xFF493B7F, 0xFF6F4534 },
        // Classic Alex
        { 0xFFC57242, 0xFFA85C32, 0xFFD5A98C, 0xFFC29578, 0xFF3B6638, 0xFF9E6B55 },
        // Dark hair / warm skin
        { 0xFF242024, 0xFF171417, 0xFFC69076, 0xFFB07C63, 0xFF2E699E, 0xFF7B4E3B },
        // Golden hair
        { 0xFFD8AF4B, 0xFFB99235, 0xFFDAB196, 0xFFC59B80, 0xFF35748A, 0xFF94624E },
        // Auburn / red hair
        { 0xFF8C3B2B, 0xFF6E2B1E, 0xFFD0A187, 0xFFBA8B72, 0xFF3E6E48, 0xFF885643 }
    };
    const SkinTone& t = tones[h % ARRAYSIZE(tones)];
    DWORD H1 = t.hair, H2 = t.hairDark, S1 = t.skin, S2 = t.skinShade, EW = 0xFFFFFFFF, EP = t.eye, MO = t.mouth;

    // 8x8 Minecraft face grid
    const DWORD grid[8][8] = {
        { H2, H1, H1, H2, H1, H1, H2, H2 },
        { H2, H1, H2, H1, H1, H2, H1, H2 },
        { H2, S2, S1, S1, S1, S1, S2, H2 },
        { S2, S1, S1, S1, S1, S1, S1, S2 },
        { S1, EW, EP, S1, S1, EP, EW, S1 },
        { S2, S1, S1, S2, S2, S1, S1, S2 },
        { S2, S1, MO, MO, MO, MO, S1, S2 },
        { S2, S2, S1, MO, MO, S1, S2, S2 }
    };

    const int sz = 32;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = sz; bi.bmiHeader.biHeight = -sz;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    void* bits = NULL;
    HDC dc = GetDC(NULL);
    HBITMAP hbm = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (dc) ReleaseDC(NULL, dc);
    if (!hbm || !bits) {
        if (hbm) DeleteObject(hbm);
        return NULL;
    }

    DWORD* px = (DWORD*)bits;
    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            px[y * sz + x] = grid[y * 8 / sz][x * 8 / sz];
        }
    }
    return hbm;
}

static bool HttpFetchBytes(const wchar_t* host, const wchar_t* path, std::vector<BYTE>& out) {
    out.clear();
    HINTERNET hSess = WinHttpOpen(L"RedstoneServerPanel/2.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return false;
    WinHttpSetTimeouts(hSess, 3500, 3500, 4000, 4000);

    HINTERNET hConn = WinHttpConnect(hSess, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConn) { WinHttpCloseHandle(hSess); return false; }

    HINTERNET hReq = WinHttpOpenRequest(hConn, L"GET", path, NULL, WINHTTP_NO_REFERER,
                                        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return false; }

    bool ok = false;
    if (WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hReq, NULL)) {
        DWORD status = 0, sz = sizeof(status);
        WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);
        if (status == 200) {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(hReq, &avail) && avail > 0) {
                if (out.size() + avail > 256 * 1024) break;
                size_t old = out.size();
                out.resize(old + avail);
                DWORD rd = 0;
                if (!WinHttpReadData(hReq, out.data() + old, avail, &rd)) break;
                out.resize(old + rd);
            }
            ok = !out.empty();
        }
    }
    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSess);
    return ok;
}

static HBITMAP DecodePngToBitmap(const std::vector<BYTE>& png, int targetSize = 32) {
    if (png.empty()) return NULL;
    IStream* stream = SHCreateMemStream(png.data(), (UINT)png.size());
    if (!stream) return NULL;

    HBITMAP result = NULL;
    {
        Gdiplus::Bitmap src(stream);
        if (src.GetLastStatus() == Gdiplus::Ok && src.GetWidth() > 0 && src.GetHeight() > 0) {
            BITMAPINFO bi = {};
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = targetSize;
            bi.bmiHeader.biHeight = -targetSize;
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            void* bits = NULL;
            HDC sdc = GetDC(NULL);
            result = CreateDIBSection(sdc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
            if (result && bits) {
                HDC mdc = CreateCompatibleDC(sdc);
                if (mdc) {
                    HGDIOBJ old = SelectObject(mdc, result);
                    {
                        Gdiplus::Graphics g(mdc);
                        g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
                        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
                        g.DrawImage(&src, Gdiplus::Rect(0, 0, targetSize, targetSize));
                    }
                    SelectObject(mdc, old);
                    DeleteDC(mdc);
                } else {
                    DeleteObject(result);
                    result = NULL;
                }
            } else if (result) {
                DeleteObject(result);
                result = NULL;
            }
            if (sdc) ReleaseDC(NULL, sdc);
        }
    }
    stream->Release();
    return result;
}

struct SkinFetchArgs {
    HWND hwnd;
    int gen;
    std::wstring name;
    std::wstring uuid;
};

static DWORD WINAPI SkinFetchThread(LPVOID p) {
    std::unique_ptr<SkinFetchArgs> a((SkinFetchArgs*)p);
    std::wstring id = !a->uuid.empty() ? a->uuid : a->name;
    // Strip leading Bedrock '.' or '*' prefix if present when querying skin APIs
    while (!id.empty() && (id[0] == L'.' || id[0] == L'*')) id.erase(0, 1);

    HBITMAP hbm = NULL;
    if (!id.empty()) {
        std::vector<BYTE> png;
        std::wstring p1 = L"/avatar/" + id + L"/32.png";
        std::wstring p2 = L"/helm/" + id + L"/32.png";

        if (!HttpFetchBytes(L"mc-heads.net", p1.c_str(), png)) {
            HttpFetchBytes(L"minotar.net", p2.c_str(), png);
        }
        hbm = DecodePngToBitmap(png, 32);
    }

    SrvEvent* ev = new (std::nothrow) SrvEvent{ EV_SKIN, a->gen, a->name, hbm };
    if (!ev) {
        if (hbm) DeleteObject(hbm);
        return 0;
    }
    if (!PostMessageW(a->hwnd, WM_SC_EVENT, 0, (LPARAM)ev)) {
        if (hbm) DeleteObject(hbm);
        delete ev;
    }
    return 0;
}

static void RequestPlayerSkin(const std::wstring& name) {
    std::wstring key = ToLowerW(name);
    if (g_skinCache.count(key) || g_skinPending.count(key)) return;
    g_skinPending.insert(key);

    if (!g_fallbackCache.count(key)) {
        g_fallbackCache[key] = MakeDefaultSkinHead(name);
    }

    std::wstring uuid;
    EnterCriticalSection(&g_csUuid);
    auto it = g_playerUuids.find(key);
    if (it != g_playerUuids.end()) uuid = it->second;
    LeaveCriticalSection(&g_csUuid);

    SkinFetchArgs* args = new (std::nothrow) SkinFetchArgs{ g_hwnd, g_gen, name, uuid };
    if (!args) {
        g_skinPending.erase(key);
        return;
    }
    HANDLE h = CreateThread(NULL, 0, SkinFetchThread, args, 0, NULL);
    if (h) CloseHandle(h);
    else {
        g_skinPending.erase(key);
        delete args;
    }
}

static HBITMAP GetPlayerSkinBitmap(const std::wstring& name) {
    std::wstring key = ToLowerW(name);
    auto it = g_skinCache.find(key);
    if (it != g_skinCache.end() && it->second) return it->second;
    auto fit = g_fallbackCache.find(key);
    if (fit != g_fallbackCache.end() && fit->second) return fit->second;
    HBITMAP fb = MakeDefaultSkinHead(name);
    g_fallbackCache[key] = fb;
    return fb;
}

// ----------------------------------------------------------------------------
//  Status Texts & Telemetry (RAM Working Set + Uptime)
// ----------------------------------------------------------------------------
static const wchar_t* StateText() {
    switch (g_state) {
        case SrvState::Starting: return L().valStarting;
        case SrvState::Running:  return L().valRunning;
        case SrvState::Stopping: return L().valStopping;
        default:                 return L().valStopped;
    }
}
static COLORREF StateColor() {
    switch (g_state) {
        case SrvState::Running:  return P.green;
        case SrvState::Stopped:  return P.red;
        default:                 return P.amber;
    }
}
static int PlayerCount() { return g_hList ? (int)SendMessageW(g_hList, LB_GETCOUNT, 0, 0) : 0; }

// Safely reads the Working Set Size (in bytes) of a given process handle
static SIZE_T GetProcessWorkingSetBytes(HANDLE hProcess) {
    if (!hProcess) return 0;
    PROCESS_MEMORY_COUNTERS pmc = {};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}

// Calculates total Working Set Size (bytes) across:
// 1. The panel application itself (GetCurrentProcess)
// 2. The active Minecraft/Java child process (g_hProc) and any descendant processes it spawned
static SIZE_T GetTotalTelemetryWorkingSetBytes() {
    SIZE_T totalBytes = GetProcessWorkingSetBytes(GetCurrentProcess());

    DWORD selfPid = GetCurrentProcessId();
    std::set<DWORD> measuredPids;
    if (selfPid != 0) measuredPids.insert(selfPid);

    if (g_hProc && g_state != SrvState::Stopped) {
        if (WaitForSingleObject(g_hProc, 0) == WAIT_TIMEOUT) {
            totalBytes += GetProcessWorkingSetBytes(g_hProc);
            DWORD srvPid = GetProcessId(g_hProc);
            if (srvPid != 0) {
                measuredPids.insert(srvPid);
            }
        }
    }

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        std::vector<std::pair<DWORD, DWORD>> procList;
        PROCESSENTRY32W pe = {};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (pe.th32ProcessID != 0) {
                    procList.push_back({ pe.th32ProcessID, pe.th32ParentProcessID });
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);

        // Iteratively collect all child/descendant PIDs rooted at selfPid or srvPid
        std::set<DWORD> treePids = measuredPids;
        bool expanded = true;
        while (expanded) {
            expanded = false;
            for (const auto& pr : procList) {
                DWORD pid = pr.first;
                DWORD ppid = pr.second;
                if (treePids.find(pid) == treePids.end() && treePids.find(ppid) != treePids.end()) {
                    treePids.insert(pid);
                    expanded = true;
                }
            }
        }

        // Query WorkingSetSize for each child/descendant process not yet measured
        for (DWORD pid : treePids) {
            if (measuredPids.find(pid) != measuredPids.end()) continue;
            measuredPids.insert(pid);

            HANDLE hChild = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (!hChild) {
                hChild = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            }
            if (!hChild) {
                hChild = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            }
            if (hChild) {
                totalBytes += GetProcessWorkingSetBytes(hChild);
                CloseHandle(hChild);
            }
        }
    }

    return totalBytes;
}

static void UpdateTelemetryStats() {
    SIZE_T totalBytes = GetTotalTelemetryWorkingSetBytes();
    unsigned long long mb = (unsigned long long)(totalBytes / (1024ULL * 1024ULL));
    if (mb == 0 && totalBytes > 0) mb = 1;
    g_ramText = std::to_wstring(mb) + L" MB";

    if (g_state != SrvState::Stopped && g_hProc) {
        ULONGLONG s = (GetTickCount64() - g_startTick) / 1000;
        g_upText = Pad2((int)(s / 3600)) + L":" + Pad2((int)(s % 3600 / 60)) + L":" + Pad2((int)(s % 60));
    } else {
        g_upText = L"00:00:00";
    }
}

// ----------------------------------------------------------------------------
//  System Tray
// ----------------------------------------------------------------------------
static void TrayAdd() {
    g_nid = {};
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwnd; g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_trayIco[1];
    lstrcpynW(g_nid.szTip, L"Redstone Server Panel", ARRAYSIZE(g_nid.szTip));
    Shell_NotifyIconW(NIM_ADD, &g_nid);
    g_lastTip.clear(); g_lastTrayIcon = -1;
}
static void TrayUpdate() {
    int ico = g_state == SrvState::Running ? 2 : g_state == SrvState::Stopped ? 1 :
              (g_state == SrvState::Starting || g_state == SrvState::Stopping) ? 3 : 1;
    std::wstring tip = L"Redstone Server Panel — ";
    switch (g_state) {
        case SrvState::Stopped:  tip += L().tipStopped; break;
        case SrvState::Starting: tip += L().tipStarting; break;
        case SrvState::Running:
            tip += std::wstring(L().tipRunningFmt) + std::to_wstring(PlayerCount()) +
                   (g_lang == Lang::TR ? L" oyuncu" : L" player(s)");
            break;
        case SrvState::Stopping: tip += L().tipStopping; break;
    }
    if (tip == g_lastTip && ico == g_lastTrayIcon) return;
    g_lastTip = tip; g_lastTrayIcon = ico;
    g_nid.uFlags = NIF_ICON | NIF_TIP;
    g_nid.hIcon = g_trayIco[ico];
    lstrcpynW(g_nid.szTip, tip.c_str(), ARRAYSIZE(g_nid.szTip));
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}
static void Balloon(const std::wstring& title, const std::wstring& text) {
    if (IsWindowVisible(g_hwnd)) return;
    NOTIFYICONDATAW n = {};
    n.cbSize = sizeof(n); n.hWnd = g_hwnd; n.uID = 1; n.uFlags = NIF_INFO;
    lstrcpynW(n.szInfoTitle, title.c_str(), ARRAYSIZE(n.szInfoTitle));
    lstrcpynW(n.szInfo, text.c_str(), ARRAYSIZE(n.szInfo));
    n.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
    Shell_NotifyIconW(NIM_MODIFY, &n);
}
static void ShowMain() {
    ShowWindow(g_hwnd, SW_SHOW);
    if (IsIconic(g_hwnd)) ShowWindow(g_hwnd, SW_RESTORE);
    SetForegroundWindow(g_hwnd);
}
static void HideMain() {
    ShowWindow(g_hwnd, SW_HIDE);
    if (!g_trayHintShown) {
        g_trayHintShown = true;
        Balloon(L"Redstone Server Panel", L().balloonTrayBg);
    }
}

// ----------------------------------------------------------------------------
//  Console Log (RichEdit Colored Output)
// ----------------------------------------------------------------------------
static const wchar_t* GetRichEditClass() {
    static const wchar_t* s_class = NULL;
    if (s_class) return s_class;
    if (LoadLibraryW(L"msftedit.dll")) {
        s_class = MSFTEDIT_CLASS;
    } else if (LoadLibraryW(L"riched20.dll")) {
        s_class = L"RichEdit20W";
    } else {
        s_class = L"EDIT";
    }
    return s_class;
}

static bool IsBlankLineW(const std::wstring& s) {
    for (wchar_t c : s) {
        if (c > 32 && c != 0x7F && c != 0x85 && c != 160 && c != 0x1680 &&
            !(c >= 0x2000 && c <= 0x200F) && c != 0x2028 && c != 0x2029 &&
            c != 0x202F && c != 0x205F && c != 0x2060 && c != 0x3000 && c != 0xFEFF)
            return false;
    }
    return true;
}

static COLORREF GetConsoleColorForLine(const std::wstring& line, bool isDark) {
    // 1. Outgoing console command typed by user: e.g. "> help"
    if (line.size() >= 2 && line[0] == L'>' && line[1] == L' ') {
        if (!isDark) return RGB(37, 99, 235);
        if (g_theme == Theme::Redstone) return RGB(255, 120, 120);
        if (g_theme == Theme::Minecraft) return RGB(110, 245, 160);
        return RGB(56, 189, 248); // Electric cyan (dark)
    }
    // 2. Internal panel log / event notification: e.g. "» Sunucu başlatılıyor..."
    if (line.size() >= 2 && (line[0] == L'»' || line[0] == 0x00BB)) {
        return isDark ? RGB(216, 180, 254) : RGB(124, 58, 237); // Bright orchid (dark) / rich purple (light)
    }

    std::wstring lower = ToLowerW(line);

    // 3. Stack traces & Exceptions
    if (lower.find(L"\tat ") != std::wstring::npos ||
        lower.find(L"caused by:") != std::wstring::npos) {
        return isDark ? RGB(255, 107, 107) : RGB(220, 38, 38);
    }

    // 4. Server Errors / Fatal crashes
    if (lower.find(L"error") != std::wstring::npos ||
        lower.find(L"fatal") != std::wstring::npos ||
        lower.find(L"exception") != std::wstring::npos ||
        lower.find(L"critical") != std::wstring::npos ||
        lower.find(L"failed") != std::wstring::npos) {
        return isDark ? RGB(255, 107, 107) : RGB(220, 38, 38);
    }

    // 5. Server Warnings
    if (lower.find(L"warn") != std::wstring::npos ||
        lower.find(L"warning") != std::wstring::npos) {
        return isDark ? RGB(251, 191, 36) : RGB(217, 119, 6); // Warm gold (dark) / deep amber (light)
    }

    // 6. Player in-game chat messages: e.g. "<qu3zel> hello world"
    size_t lt = line.find(L'<');
    size_t gt = line.find(L'>', lt == std::wstring::npos ? 0 : lt);
    if (lt != std::wstring::npos && gt != std::wstring::npos && gt > lt + 1) {
        return isDark ? RGB(74, 222, 128) : RGB(22, 163, 74); // Vibrant spring green
    }

    // 7. Player join, leave, login events & Server Ready
    if (lower.find(L"joined the game") != std::wstring::npos ||
        lower.find(L"left the game") != std::wstring::npos ||
        lower.find(L"logged in with entity id") != std::wstring::npos ||
        lower.find(L"for help, type") != std::wstring::npos) {
        return isDark ? RGB(52, 211, 153) : RGB(16, 149, 94); // Mint emerald
    }

    // 8. Debug / Trace lines
    if (lower.find(L"/debug") != std::wstring::npos ||
        lower.find(L"[debug]") != std::wstring::npos ||
        lower.find(L"/trace") != std::wstring::npos ||
        lower.find(L"[trace]") != std::wstring::npos) {
        return isDark ? RGB(175, 190, 215) : RGB(100, 116, 139); // Clean bright slate (high contrast)
    }

    // 9. Standard log text
    return isDark ? RGB(245, 248, 252) : P.logText;
}

static void LockConsoleHorizontalScroll() {
    if (!g_hLog) return;
    SendMessageW(g_hLog, WM_HSCROLL, SB_LEFT, 0);
    POINT pt = { 0, 0 };
    SendMessageW(g_hLog, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
    if (pt.x != 0) {
        pt.x = 0;
        SendMessageW(g_hLog, EM_SETSCROLLPOS, 0, (LPARAM)&pt);
    }
}

static void AppendLogInternal(const std::wstring& text, bool logToFile) {
    if (!g_hLog || text.empty()) return;
    if (logToFile) LogToFile(text);

    bool isVisiblePage = (g_page == PG_PANEL);

    int len = GetWindowTextLengthW(g_hLog);
    if (len > 400000) {
        if (isVisiblePage) SendMessageW(g_hLog, WM_SETREDRAW, FALSE, 0);
        CHARRANGE crDel = { 0, 150000 };
        SendMessageW(g_hLog, EM_EXSETSEL, 0, (LPARAM)&crDel);
        SendMessageW(g_hLog, EM_REPLACESEL, FALSE, (LPARAM)L"");
        if (isVisiblePage) SendMessageW(g_hLog, WM_SETREDRAW, TRUE, 0);
    }

    bool isDark = (g_theme != Theme::Light);

    struct ColorSpan {
        COLORREF color;
        std::wstring content;
    };
    std::vector<ColorSpan> batches;

    size_t pos = 0;
    while (pos < text.size()) {
        size_t nextN = text.find(L'\n', pos);
        std::wstring rawLine;
        if (nextN == std::wstring::npos) {
            rawLine = text.substr(pos);
            pos = text.size();
        } else {
            rawLine = text.substr(pos, nextN - pos);
            pos = nextN + 1;
        }

        // Strip any trailing \r or \n
        while (!rawLine.empty() && (rawLine.back() == L'\r' || rawLine.back() == L'\n')) {
            rawLine.pop_back();
        }

        // Drop empty or pure-whitespace lines to eliminate blank lines in console
        if (IsBlankLineW(rawLine)) continue;

        // RichEdit treats these as hard/soft line breaks; they would create phantom blank lines
        for (wchar_t& ch : rawLine) {
            if (ch == L'\r' || ch == 0x0B || ch == 0x0C || ch == 0x85 || ch == 0x2028 || ch == 0x2029) ch = L' ';
        }

        COLORREF col = GetConsoleColorForLine(rawLine, isDark);

        // RichEdit expects a single '\r' as a clean paragraph break
        std::wstring richLine = rawLine + L"\r";

        if (!batches.empty() && batches.back().color == col) {
            batches.back().content += richLine;
        } else {
            batches.push_back({ col, richLine });
        }
    }

    // Check if scrollbar is currently at or near the bottom before appending
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(g_hLog, SB_VERT, &si);
    bool wasAtBottom = (si.nMax <= 0 || (si.nPos + (int)si.nPage >= si.nMax - 14));

    if (isVisiblePage) {
        SendMessageW(g_hLog, WM_SETREDRAW, FALSE, 0);
    }

    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;

    for (const auto& b : batches) {
        CHARRANGE cr = { -1, -1 };
        SendMessageW(g_hLog, EM_EXSETSEL, 0, (LPARAM)&cr);
        // Force tight single spacing so paragraphs never render with extra gaps
        PARAFORMAT2 pf;
        ZeroMemory(&pf, sizeof(pf));
        pf.cbSize = sizeof(pf);
        pf.dwMask = PFM_SPACEBEFORE | PFM_SPACEAFTER | PFM_LINESPACING;
        pf.dySpaceBefore = 0;
        pf.dySpaceAfter = 0;
        pf.bLineSpacingRule = 0;
        SendMessageW(g_hLog, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
        cf.crTextColor = b.color;
        SendMessageW(g_hLog, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        SendMessageW(g_hLog, EM_REPLACESEL, FALSE, (LPARAM)b.content.c_str());
    }

    if (isVisiblePage) {
        SendMessageW(g_hLog, WM_SETREDRAW, TRUE, 0);
        if (wasAtBottom) {
            SendMessageW(g_hLog, WM_VSCROLL, SB_BOTTOM, 0);
        }
        // Lock horizontal scroll to 0 so the beginning of lines is NEVER hidden to the left
        LockConsoleHorizontalScroll();
        InvalidateRect(g_hLog, NULL, FALSE);
        UpdateWindow(g_hLog);
    } else {
        // Guarantee g_hLog stays completely hidden when not on dashboard
        ShowWindow(g_hLog, SW_HIDE);
    }
}

static void AppendLog(const std::wstring& text) {
    AppendLogInternal(text, true);
}

static void AppLog(const std::wstring& t) {
    AppendLog(L"» " + t + L"\r\n");
}

static void ReapplyLogColors() {
    if (!g_hLog) return;
    SendMessageW(g_hLog, EM_SETBKGNDCOLOR, 0, (LPARAM)P.logBg);

    // If we're not currently on the dashboard/console page, defer recoloring until navigated to
    if (!IsWindowVisible(g_hLog) || g_page != PG_PANEL) {
        g_logThemeDirty = true;
        return;
    }

    int len = GetWindowTextLengthW(g_hLog);
    if (len <= 0) return;

    std::wstring all(len + 1, L'\0');
    GetWindowTextW(g_hLog, &all[0], len + 1);
    all.resize(len);

    SendMessageW(g_hLog, WM_SETREDRAW, FALSE, 0);
    SetWindowTextW(g_hLog, L"");
    AppendLogInternal(all, false);
    SendMessageW(g_hLog, WM_SETREDRAW, TRUE, 0);
    if (IsWindowVisible(g_hLog)) {
        SendMessageW(g_hLog, WM_VSCROLL, SB_BOTTOM, 0);
        LockConsoleHorizontalScroll();
        InvalidateRect(g_hLog, NULL, TRUE);
        UpdateWindow(g_hLog);
    }
}

// ----------------------------------------------------------------------------
//  Server Stdin Writer
// ----------------------------------------------------------------------------
static bool WriteToConsole(std::wstring cmd) {
    if (g_state == SrvState::Stopped || !g_hStdinWr) return false;
    cmd.erase(std::remove(cmd.begin(), cmd.end(), L'\r'), cmd.end());
    cmd.erase(std::remove(cmd.begin(), cmd.end(), L'\n'), cmd.end());
    std::string s = ToUtf8(cmd) + "\n";
    DWORD w = 0;
    return WriteFile(g_hStdinWr, s.data(), (DWORD)s.size(), &w, NULL) && w == s.size();
}

// ----------------------------------------------------------------------------
//  Reader Thread: Parses Minecraft Server Output Accurately
// ----------------------------------------------------------------------------
struct ReaderArgs { HWND hwnd; HANDLE rd; int gen; };

static void PostEv(HWND h, int gen, int type, const std::wstring& t = L"") {
    SrvEvent* e = new (std::nothrow) SrvEvent{ type, gen, t, NULL };
    if (!e) return;
    if (!PostMessageW(h, WM_SC_EVENT, 0, (LPARAM)e)) delete e;
}

// Strips ANSI escape sequences (\x1b[...), OSC sequences, AND Minecraft section-sign formatting codes (§x)
static void StripFormatting(std::string& s) {
    std::string o; o.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == 0x1B) {
            if (i + 1 < s.size() && s[i + 1] == '[') {
                i += 2;
                while (i < s.size() && !((s[i] >= '@' && s[i] <= '~'))) i++;
            } else if (i + 1 < s.size() && s[i + 1] == ']') {
                // OSC sequence terminated by \x07 (BEL) or ST (\x1b\\)
                i += 2;
                while (i < s.size() && s[i] != 0x07 && s[i] != 0x1B) i++;
                if (i < s.size() && s[i] == 0x1B && i + 1 < s.size() && s[i + 1] == '\\') i++;
            } else if (i + 1 < s.size()) {
                i++; // 2-byte escape
            }
        } else if (c == 0xC2 && i + 1 < s.size() && (unsigned char)s[i + 1] == 0xA7) {
            // UTF-8 '§' (0xC2 0xA7) followed by color/format code
            i += 2;
        } else if (c == 0xA7 && i + 1 < s.size()) {
            // Single-byte '§' followed by color/format code
            i += 1;
        } else if (c >= 0x20 || c == '\t' || c == '\n') {
            o.push_back(s[i]);
        }
    }
    s.swap(o);
}

static bool EndsWith(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}
static bool StartsWith(const std::string& s, const std::string& pre) {
    return s.compare(0, pre.size(), pre) == 0;
}

static bool IsValidMcToken(const std::string& n) {
    if (n.empty() || n.size() > 32) return false;
    for (unsigned char c : n) {
        if (!(isalnum(c) || c == '_' || c == '.' || c == '-' || c == '*')) return false;
    }
    std::string low = ToLowerA(n);
    // Prevent logger tags like "System", "Server", "INFO", "CHAT" from ever being treated as player names
    if (low == "system" || low == "server" || low == "chat" || low == "info" || low == "warn" || low == "error")
        return false;
    return true;
}

// Strips logger/timestamp/thread prefixes such as "[12:34:56 INFO]: [System] [CHAT] "
static std::string ExtractLogPayload(const std::string& line) {
    std::string msg = line;
    // Advance past standard logger header(s) ending with "]: "
    size_t pos = 0;
    while (true) {
        size_t br = msg.find("]: ", pos);
        if (br == std::string::npos) break;
        // Only treat "]: " as header if there's a '[' before it and no '<' (chat) before it
        size_t openBr = msg.rfind('[', br);
        size_t chatLt = msg.find('<', pos);
        if (openBr != std::string::npos && (chatLt == std::string::npos || openBr < chatLt)) {
            pos = br + 3;
        } else {
            break;
        }
    }
    if (pos > 0) msg = msg.substr(pos);

    // Strip any remaining leading tags like "[System]", "[CHAT]", "[Not Secure]", "System chat:", "System:", "System "
    msg = TrimA(msg);
    bool changed = true;
    while (changed && !msg.empty()) {
        changed = false;
        if (msg[0] == '[') {
            size_t cl = msg.find(']');
            if (cl != std::string::npos) {
                // Do not strip if this is "PlayerName[/127.0.0.1]" (that starts with player name, not '[')
                msg = TrimA(msg.substr(cl + 1));
                if (!msg.empty() && msg[0] == ':') msg = TrimA(msg.substr(1));
                changed = true;
            }
        } else if (StartsWith(ToLowerA(msg), "system chat:")) {
            msg = TrimA(msg.substr(12));
            changed = true;
        } else if (StartsWith(ToLowerA(msg), "system:") || StartsWith(ToLowerA(msg), "system ")) {
            msg = TrimA(msg.substr(7));
            if (!msg.empty() && (msg[0] == ':' || msg[0] == '|' || msg[0] == '>'))
                msg = TrimA(msg.substr(1));
            changed = true;
        }
    }
    return msg;
}

// Extracts player name from join/leave suffix lines ("<Player> joined the game" / "<Player> (formerly <Old>) joined the game")
static bool ExtractJoinLeaveName(std::string prefixPart, std::string& out) {
    // Reject chat or colon-prefixed lines (e.g. "<Player> someone joined the game" or "[G] Steve: Alex joined the game")
    if (prefixPart.find('<') != std::string::npos || prefixPart.find('>') != std::string::npos ||
        prefixPart.find(':') != std::string::npos)
        return false;

    prefixPart = TrimA(prefixPart);
    // Handle "(formerly OldName)"
    size_t fpos = prefixPart.rfind(" (formerly ");
    if (fpos != std::string::npos && prefixPart.back() == ')') {
        prefixPart = TrimA(prefixPart.substr(0, fpos));
    }

    // Take the token immediately preceding "joined the game" / "left the game"
    size_t sp = prefixPart.rfind(' ');
    std::string candidate = (sp == std::string::npos) ? prefixPart : prefixPart.substr(sp + 1);
    if (!IsValidMcToken(candidate)) return false;
    out = candidate;
    return true;
}

static void HandleLine(const ReaderArgs& a, std::string line, std::wstring& chunk) {
    StripFormatting(line);
    if (TrimA(line).empty()) return;
    std::string msg = ExtractLogPayload(line);
    bool suppress = false;

    if (g_suppressErrLines.load() > 0 && EndsWith(msg, "<--[HERE]")) {
        g_suppressErrLines--;
        return;
    }

    static const std::string JOIN = " joined the game";
    static const std::string LEFT = " left the game";
    static const std::string UUID_PRE = "UUID of player ";
    static const std::string LOST_CONN = " lost connection:";

    std::string name;

    // 0. Detect Minecraft version ("Starting minecraft server version X")
    if (StartsWith(msg, "Starting minecraft server version ")) {
        std::string ver = TrimA(msg.substr(34));
        if (!ver.empty() && !StartsWith(ver, "1.")) {
            g_useTimelineCmd = true; // Minecraft 26.1+ uses timeline syntax ("time query day")
        }
    }
    // 1. "UUID of player <Name> is <UUID>"
    else if (StartsWith(msg, UUID_PRE) && msg.find('<') == std::string::npos) {
        size_t isPos = msg.find(" is ", UUID_PRE.size());
        if (isPos != std::string::npos) {
            std::string pName = TrimA(msg.substr(UUID_PRE.size(), isPos - UUID_PRE.size()));
            std::string uuid = TrimA(msg.substr(isPos + 4));
            if (IsValidMcToken(pName) && !uuid.empty()) {
                EnterCriticalSection(&g_csUuid);
                g_playerUuids[ToLowerW(FromUtf8(pName))] = FromUtf8(uuid);
                LeaveCriticalSection(&g_csUuid);
            }
        }
    }
    // 2. "<Name>[/<ip>:<port>] logged in with entity id ..."
    else if (msg.find("] logged in with entity id ") != std::string::npos && msg.find('<') == std::string::npos) {
        size_t br = msg.find('[');
        if (br != std::string::npos && br > 0) {
            std::string sub = TrimA(msg.substr(0, br));
            size_t sp = sub.rfind(' ');
            std::string pName = (sp == std::string::npos) ? sub : sub.substr(sp + 1);
            if (IsValidMcToken(pName)) {
                PostEv(a.hwnd, a.gen, EV_JOIN, FromUtf8(pName));
            }
        }
    }
    // 3. "<Name> joined the game"
    else if (EndsWith(msg, JOIN) && ExtractJoinLeaveName(msg.substr(0, msg.size() - JOIN.size()), name)) {
        PostEv(a.hwnd, a.gen, EV_JOIN, FromUtf8(name));
    }
    // 4. "<Name> left the game"
    else if (EndsWith(msg, LEFT) && ExtractJoinLeaveName(msg.substr(0, msg.size() - LEFT.size()), name)) {
        PostEv(a.hwnd, a.gen, EV_LEAVE, FromUtf8(name));
    }
    // 5. "<Name> lost connection: ..."
    else if (msg.find(LOST_CONN) != std::string::npos && msg.find('<') == std::string::npos) {
        size_t lc = msg.find(LOST_CONN);
        if (ExtractJoinLeaveName(msg.substr(0, lc), name)) {
            PostEv(a.hwnd, a.gen, EV_LEAVE, FromUtf8(name));
        }
    }
    // 6. "There are X of a max of Y players online: name1, name2"
    else if (StartsWith(msg, "There are ") && msg.find(" players online:") != std::string::npos) {
        size_t col = msg.find(" players online:");
        std::string listPart = TrimA(msg.substr(col + 16));
        PostEv(a.hwnd, a.gen, EV_SYNC_LIST, FromUtf8(listPart));
    }
    // 7. Time query response:
    //    - Legacy (<=1.21.4): "The time is <ticks>"
    //    - Modern (26.1+):    "Timeline minecraft:day is at <ticks> tick(s)" or "Clock ... is at <ticks> tick(s)"
    else if (StartsWith(msg, "The time is ") ||
             ((StartsWith(msg, "Timeline ") || StartsWith(msg, "Clock ")) && msg.find(" is at ") != std::string::npos)) {
        const char* start = NULL;
        if (StartsWith(msg, "The time is ")) {
            start = msg.c_str() + 12;
        } else {
            size_t p = msg.find(" is at ");
            if (p != std::string::npos) start = msg.c_str() + p + 7;
        }
        if (start) {
            char* end = NULL;
            long long ticks = strtoll(start, &end, 10);
            if (end != start) {
                ticks = ((ticks % 24000) + 24000) % 24000;
                int hour = (int)((ticks / 1000 + 6) % 24);
                int min = (int)((ticks % 1000) * 60 / 1000);
                PostEv(a.hwnd, a.gen, EV_TIME, Pad2(hour) + L":" + Pad2(min));
            }
        }
        if (g_pendingTime.load() > 0) { g_pendingTime--; suppress = true; }
    }
    // 7b. Fallback if "time query daytime" was sent to a 26.1+ server before version detection
    else if (msg.find("Can't find element 'minecraft:daytime'") != std::string::npos) {
        g_useTimelineCmd = true;
        if (g_pendingTime.load() > 0) g_pendingTime--;
        suppress = true;
        g_suppressErrLines = 1;
        PostEv(a.hwnd, a.gen, EV_RETRY_TIME);
    }
    // 8. Server Ready ("Done (...s)! For help, type ...")
    else if (StartsWith(msg, "Done (") && msg.find("For help") != std::string::npos) {
        PostEv(a.hwnd, a.gen, EV_READY);
    }
    // 9. Server Stopping
    else if (msg == "Stopping server" || msg == "Stopping the server") {
        PostEv(a.hwnd, a.gen, EV_STOPPING);
    }

    if (!suppress) { chunk += FromUtf8(line); chunk += L"\r\n"; }
}

static DWORD WINAPI ReaderThread(LPVOID p) {
    ReaderArgs a = *(ReaderArgs*)p; delete (ReaderArgs*)p;
    char buf[4096]; DWORD n; std::string line;
    while (ReadFile(a.rd, buf, sizeof(buf), &n, NULL) && n > 0) {
        std::wstring chunk;
        for (DWORD i = 0; i < n; i++) {
            char c = buf[i];
            if (c == '\n' || line.size() > 16384) {
                HandleLine(a, line, chunk);
                line.clear();
                if (c != '\n' && c != '\r') line.push_back(c);
            } else if (c != '\r') line.push_back(c);
        }
        if (!chunk.empty()) PostEv(a.hwnd, a.gen, EV_LOG, chunk);
    }
    if (!line.empty()) { std::wstring chunk; HandleLine(a, line, chunk); if (!chunk.empty()) PostEv(a.hwnd, a.gen, EV_LOG, chunk); }
    CloseHandle(a.rd);
    PostEv(a.hwnd, a.gen, EV_EXITED);
    return 0;
}

// ----------------------------------------------------------------------------
//  Validation / Java / server.properties / EULA
// ----------------------------------------------------------------------------
static bool ValidRam(const std::wstring& r) {
    if (r.size() < 2 || r.size() > 7) return false;
    wchar_t u = towupper(r.back());
    if (u != L'G' && u != L'M') return false;
    for (size_t i = 0; i + 1 < r.size(); i++) if (!iswdigit(r[i])) return false;
    return _wtoi(r.substr(0, r.size() - 1).c_str()) > 0;
}
static bool ValidPort(const std::wstring& p) {
    if (p.empty() || p.size() > 5) return false;
    for (wchar_t c : p) if (!iswdigit(c)) return false;
    int v = _wtoi(p.c_str()); return v >= 1 && v <= 65535;
}
static bool ValidIp(const std::wstring& ip) {
    if (ip.size() > 253) return false;
    for (wchar_t c : ip) if (!(iswalnum(c) && c < 128) && c != L'.' && c != L':' && c != L'-') return false;
    return true;
}
static bool ValidPlayerName(const std::wstring& n) {
    if (n.empty() || n.size() > 32) return false;
    for (wchar_t c : n) if (!((iswalnum(c) && c < 128) || c == L'_' || c == L'.' || c == L'-' || c == L'*')) return false;
    return true;
}
static bool FileExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}
// Looks for installed JDK/JRE locations in the registry (Oracle, Adoptium, Microsoft, Zulu...)
// when java.exe is neither on PATH nor referenced by JAVA_HOME.
static std::wstring FindJavaInRegistry() {
    struct Root { const wchar_t* key; const wchar_t* valueName; };
    static const Root roots[] = {
        { L"SOFTWARE\\JavaSoft\\JDK",                  L"JavaHome" },
        { L"SOFTWARE\\JavaSoft\\JRE",                  L"JavaHome" },
        { L"SOFTWARE\\JavaSoft\\Java Development Kit", L"JavaHome" },
        { L"SOFTWARE\\JavaSoft\\Java Runtime Environment", L"JavaHome" },
    };
    static const wchar_t* msiRoots[] = {
        L"SOFTWARE\\Eclipse Adoptium\\JDK", L"SOFTWARE\\Eclipse Adoptium\\JRE",
        L"SOFTWARE\\Microsoft\\JDK", L"SOFTWARE\\Azul Systems\\Zulu",
    };
    REGSAM views[] = { KEY_READ | KEY_WOW64_64KEY, KEY_READ | KEY_WOW64_32KEY };
    std::wstring best;

    auto readStr = [](HKEY k, const wchar_t* name) -> std::wstring {
        wchar_t buf[MAX_PATH]; DWORD sz = sizeof(buf), type = 0;
        if (RegQueryValueExW(k, name, NULL, &type, (BYTE*)buf, &sz) != ERROR_SUCCESS || type != REG_SZ) return L"";
        buf[MAX_PATH - 1] = 0;
        return buf;
    };
    auto tryHome = [&](std::wstring home) {
        while (!home.empty() && (home.back() == L'\\' || home.back() == L'/')) home.pop_back();
        std::wstring p = home + L"\\bin\\java.exe";
        if (FileExists(p) && p > best) best = p;   // later version strings sort higher
    };

    for (REGSAM view : views) {
        for (const Root& r : roots) {
            HKEY k;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, r.key, 0, view, &k) != ERROR_SUCCESS) continue;
            std::wstring cur = readStr(k, L"CurrentVersion");
            if (!cur.empty()) {
                HKEY vk;
                if (RegOpenKeyExW(k, cur.c_str(), 0, view, &vk) == ERROR_SUCCESS) {
                    tryHome(readStr(vk, r.valueName)); RegCloseKey(vk);
                }
            }
            RegCloseKey(k);
        }
        for (const wchar_t* root : msiRoots) {
            HKEY k;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, root, 0, view, &k) != ERROR_SUCCESS) continue;
            wchar_t sub[256];
            for (DWORD i = 0; ; i++) {
                DWORD sl = ARRAYSIZE(sub);
                if (RegEnumKeyExW(k, i, sub, &sl, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
                for (const wchar_t* tail : { L"\\hotspot\\MSI", L"\\MSI" }) {
                    HKEY mk;
                    std::wstring path = std::wstring(sub) + tail;
                    if (RegOpenKeyExW(k, path.c_str(), 0, view, &mk) == ERROR_SUCCESS) {
                        tryHome(readStr(mk, L"Path")); RegCloseKey(mk);
                    }
                }
            }
            RegCloseKey(k);
        }
    }
    return best;
}
static std::wstring FindJava() {
    wchar_t buf[MAX_PATH];
    DWORD n = SearchPathW(NULL, L"java.exe", NULL, MAX_PATH, buf, NULL);
    if (n > 0 && n < MAX_PATH) return buf;
    wchar_t jh[MAX_PATH];
    n = GetEnvironmentVariableW(L"JAVA_HOME", jh, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        std::wstring p = std::wstring(jh) + L"\\bin\\java.exe";
        if (FileExists(p)) return p;
    }
    return FindJavaInRegistry();
}
static bool EulaAccepted(const std::wstring& dir) {
    std::string d;
    if (!ReadAll(dir + L"\\eula.txt", d)) return false;
    std::transform(d.begin(), d.end(), d.begin(), [](unsigned char c) { return (char)tolower(c); });
    return d.find("eula=true") != std::string::npos;
}
static void UpdateProperties(const std::wstring& dir, const std::vector<std::pair<std::string, std::string>>& kv) {
    std::wstring path = dir + L"\\server.properties";
    std::string data; ReadAll(path, data);
    std::vector<std::string> lines;
    size_t p = 0;
    while (p < data.size()) {
        size_t e = data.find('\n', p); if (e == std::string::npos) e = data.size();
        std::string l = data.substr(p, e - p);
        if (!l.empty() && l.back() == '\r') l.pop_back();
        lines.push_back(l); p = e + 1;
    }
    for (auto& pr : kv) {
        bool found = false;
        for (auto& l : lines) {
            std::string t = TrimA(l);
            if (!t.empty() && t[0] == '#') continue;
            size_t eq = t.find('=');
            if (eq != std::string::npos && TrimA(t.substr(0, eq)) == pr.first) {
                l = pr.first + "=" + pr.second;
                found = true;
            }
        }
        if (!found) lines.push_back(pr.first + "=" + pr.second);
    }
    std::string out;
    for (auto& l : lines) out += l + "\n";
    WriteAll(path, out);
}

static std::wstring TrimQuotes(std::wstring s) {
    s = Trim(s);
    while (s.size() >= 2 && s.front() == L'"' && s.back() == L'"') {
        s = Trim(s.substr(1, s.size() - 2));
    }
    return s;
}

static std::wstring GetJarPath() {
    return g_hJar ? TrimQuotes(GetText(g_hJar)) : L"";
}

static bool HasServerJar() {
    return !GetJarPath().empty();
}

static std::wstring GetServerDir() {
    std::wstring jar = GetJarPath();
    if (jar.empty()) return L".";
    size_t sl = jar.find_last_of(L"\\/");
    return sl == std::wstring::npos ? L"." : jar.substr(0, sl);
}

static std::map<std::string, std::string> ReadPropertiesMap(const std::wstring& dir) {
    std::map<std::string, std::string> mp;
    std::string data;
    if (!ReadAll(dir + L"\\server.properties", data)) return mp;
    size_t p = 0;
    while (p < data.size()) {
        size_t e = data.find('\n', p);
        if (e == std::string::npos) e = data.size();
        std::string l = TrimA(data.substr(p, e - p));
        p = e + 1;
        if (l.empty() || l[0] == '#') continue;
        size_t eq = l.find('=');
        if (eq != std::string::npos) {
            mp[TrimA(l.substr(0, eq))] = TrimA(l.substr(eq + 1));
        }
    }
    return mp;
}

static void LoadServerPropertiesToUi() {
    if (!g_hMotd) return;
    std::map<std::string, std::string> mp;
    if (HasServerJar()) mp = ReadPropertiesMap(GetServerDir());
    auto get = [&](const char* k, const char* def) -> std::string {
        auto it = mp.find(k);
        return (it != mp.end() && !it->second.empty()) ? it->second : std::string(def);
    };
    auto getBool = [&](const char* k, bool def) -> bool {
        auto it = mp.find(k);
        if (it == mp.end()) return def;
        std::string v = ToLowerA(it->second);
        return v == "true" || v == "1" || v == "yes";
    };

    SetWindowTextW(g_hMotd,      FromUtf8(get("motd", "A Minecraft Server")).c_str());
    SetWindowTextW(g_hMaxPl,     FromUtf8(get("max-players", "20")).c_str());
    SetWindowTextW(g_hViewDist,  FromUtf8(get("view-distance", "10")).c_str());
    SetWindowTextW(g_hSpawnProt, FromUtf8(get("spawn-protection", "16")).c_str());

    std::string gm = ToLowerA(get("gamemode", "survival"));
    if (gm == "creative" || gm == "1") g_propGm = 1;
    else if (gm == "adventure" || gm == "2") g_propGm = 2;
    else if (gm == "spectator" || gm == "3") g_propGm = 3;
    else g_propGm = 0;

    std::string df = ToLowerA(get("difficulty", "normal"));
    if (df == "peaceful" || df == "0") g_propDf = 0;
    else if (df == "easy" || df == "1") g_propDf = 1;
    else if (df == "hard" || df == "3") g_propDf = 3;
    else g_propDf = 2;

    g_propOnline    = getBool("online-mode", true);
    g_propPvp       = getBool("pvp", true);
    g_propWhitelist = getBool("white-list", false);
    g_propFlight    = getBool("allow-flight", false);
    g_propCmdBlock  = getBool("enable-command-block", false);

    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
}

static void SaveServerPropertiesFromUi() {
    if (!HasServerJar()) {
        Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING);
        return;
    }
    std::wstring dir = GetServerDir();
    std::wstring motd = Trim(GetText(g_hMotd));
    std::wstring maxp = Trim(GetText(g_hMaxPl));
    std::wstring view = Trim(GetText(g_hViewDist));
    std::wstring spwn = Trim(GetText(g_hSpawnProt));
    if (maxp.empty()) maxp = L"20";
    if (view.empty()) view = L"10";
    if (spwn.empty()) spwn = L"16";

    static const char* gmNames[4] = { "survival", "creative", "adventure", "spectator" };
    static const char* dfNames[4] = { "peaceful", "easy", "normal", "hard" };

    std::vector<std::pair<std::string, std::string>> kv = {
        { "motd",                 ToUtf8(motd.empty() ? L"A Minecraft Server" : motd) },
        { "max-players",          ToUtf8(maxp) },
        { "view-distance",        ToUtf8(view) },
        { "spawn-protection",     ToUtf8(spwn) },
        { "gamemode",             gmNames[g_propGm & 3] },
        { "difficulty",           dfNames[g_propDf & 3] },
        { "online-mode",          g_propOnline    ? "true" : "false" },
        { "pvp",                  g_propPvp       ? "true" : "false" },
        { "white-list",           g_propWhitelist ? "true" : "false" },
        { "allow-flight",         g_propFlight    ? "true" : "false" },
        { "enable-command-block", g_propCmdBlock  ? "true" : "false" }
    };
    UpdateProperties(dir, kv);

    if (g_state == SrvState::Running) {
        WriteToConsole(std::wstring(L"defaultgamemode ") + FromUtf8(gmNames[g_propGm & 3]));
        WriteToConsole(std::wstring(L"difficulty ") + FromUtf8(dfNames[g_propDf & 3]));
        WriteToConsole(g_propWhitelist ? L"whitelist on" : L"whitelist off");
    }

    AppLog(Tx(L"server.properties kaydedildi.", L"server.properties saved."));
}

// ----------------------------------------------------------------------------
//  Whitelist & OP JSON Management (whitelist.json / ops.json)
// ----------------------------------------------------------------------------
struct JsonPlayerEntry {
    std::string uuid;
    std::string name;
    int level = 4;
    bool bypassesPlayerLimit = false;
};

static std::string ExtractJsonField(const std::string& obj, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    size_t k = obj.find(pat);
    if (k == std::string::npos) return "";
    size_t col = obj.find(':', k + pat.size());
    if (col == std::string::npos) return "";
    size_t q1 = obj.find('"', col + 1);
    if (q1 == std::string::npos) return "";
    size_t q2 = obj.find('"', q1 + 1);
    if (q2 == std::string::npos) return "";
    return obj.substr(q1 + 1, q2 - q1 - 1);
}

static int ExtractJsonInt(const std::string& obj, const std::string& key, int defVal) {
    std::string pat = "\"" + key + "\"";
    size_t k = obj.find(pat);
    if (k == std::string::npos) return defVal;
    size_t col = obj.find(':', k + pat.size());
    if (col == std::string::npos) return defVal;
    size_t p = col + 1;
    while (p < obj.size() && (obj[p] == ' ' || obj[p] == '\t' || obj[p] == '\r' || obj[p] == '\n')) ++p;
    if (p >= obj.size() || !(isdigit((unsigned char)obj[p]) || obj[p] == '-')) return defVal;
    return atoi(obj.c_str() + p);
}

static bool ExtractJsonBool(const std::string& obj, const std::string& key, bool defVal) {
    std::string pat = "\"" + key + "\"";
    size_t k = obj.find(pat);
    if (k == std::string::npos) return defVal;
    size_t col = obj.find(':', k + pat.size());
    if (col == std::string::npos) return defVal;
    size_t p = col + 1;
    while (p < obj.size() && (obj[p] == ' ' || obj[p] == '\t' || obj[p] == '\r' || obj[p] == '\n')) ++p;
    if (obj.compare(p, 4, "true") == 0) return true;
    if (obj.compare(p, 5, "false") == 0) return false;
    return defVal;
}

static std::vector<JsonPlayerEntry> ReadPlayerJsonList(const std::wstring& path) {
    std::vector<JsonPlayerEntry> out;
    std::string data;
    if (!ReadAll(path, data)) return out;
    size_t pos = 0;
    while (true) {
        size_t b = data.find('{', pos);
        if (b == std::string::npos) break;
        size_t e = data.find('}', b);
        if (e == std::string::npos) break;
        std::string obj = data.substr(b, e - b + 1);
        JsonPlayerEntry pe;
        pe.name = ExtractJsonField(obj, "name");
        pe.uuid = ExtractJsonField(obj, "uuid");
        pe.level = ExtractJsonInt(obj, "level", 4);
        pe.bypassesPlayerLimit = ExtractJsonBool(obj, "bypassesPlayerLimit", false);
        if (!pe.name.empty()) out.push_back(pe);
        pos = e + 1;
    }
    return out;
}

// Generates deterministic offline-mode UUID v3 style string when UUID isn't known yet
static std::string OfflineUuidFor(const std::string& name) {
    unsigned long long h1 = 1469598103934665603ULL, h2 = 1099511628211ULL;
    std::string s = "OfflinePlayer:" + name;
    for (unsigned char c : s) {
        h1 = (h1 ^ c) * 1099511628211ULL;
        h2 = (h2 ^ (c + 0x9e3779b9ULL)) * 1469598103934665603ULL;
    }
    h1 = (h1 & ~0xF000ULL) | 0x3000ULL;
    h2 = (h2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    char buf[64];
    snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx",
             (unsigned)(h1 >> 32), (unsigned)((h1 >> 16) & 0xFFFF), (unsigned)(h1 & 0xFFFF),
             (unsigned)(h2 >> 48), (unsigned long long)(h2 & 0xFFFFFFFFFFFFULL));
    return buf;
}

static void WritePlayerJsonList(const std::wstring& path, const std::vector<JsonPlayerEntry>& list, bool isOps) {
    std::string out = "[\n";
    for (size_t i = 0; i < list.size(); i++) {
        const auto& e = list[i];
        std::string u = e.uuid.empty() ? OfflineUuidFor(e.name) : e.uuid;
        out += "  {\n";
        out += "    \"uuid\": \"" + u + "\",\n";
        out += "    \"name\": \"" + e.name + "\"";
        if (isOps) {
            out += ",\n    \"level\": " + std::to_string(e.level) + ",\n";
            out += std::string("    \"bypassesPlayerLimit\": ") + (e.bypassesPlayerLimit ? "true\n" : "false\n");
        } else {
            out += "\n";
        }
        out += (i + 1 < list.size()) ? "  },\n" : "  }\n";
    }
    out += "]\n";
    WriteAll(path, out);
}

static void LoadAccessListsToUi() {
    if (!g_hListWl || !g_hListOp) return;
    SendMessageW(g_hListWl, LB_RESETCONTENT, 0, 0);
    SendMessageW(g_hListOp, LB_RESETCONTENT, 0, 0);
    if (!HasServerJar()) {
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
        return;
    }
    std::wstring dir = GetServerDir();
    for (const auto& e : ReadPlayerJsonList(dir + L"\\whitelist.json")) {
        std::wstring w = FromUtf8(e.name);
        if (ValidPlayerName(w)) {
            RequestPlayerSkin(w);
            SendMessageW(g_hListWl, LB_ADDSTRING, 0, (LPARAM)w.c_str());
        }
    }
    for (const auto& e : ReadPlayerJsonList(dir + L"\\ops.json")) {
        std::wstring w = FromUtf8(e.name);
        if (ValidPlayerName(w)) {
            RequestPlayerSkin(w);
            SendMessageW(g_hListOp, LB_ADDSTRING, 0, (LPARAM)w.c_str());
        }
    }
    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
}

static void ModifyAccessList(bool isOp, bool add) {
    HWND hList = isOp ? g_hListOp : g_hListWl;
    HWND hEdit = isOp ? g_hEditOp : g_hEditWl;
    std::wstring name;
    if (add) {
        name = Trim(GetText(hEdit));
    } else {
        int idx = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
        if (idx != LB_ERR) {
            int n = (int)SendMessageW(hList, LB_GETTEXTLEN, idx, 0);
            if (n > 0) {
                name.assign(n + 1, L'\0');
                SendMessageW(hList, LB_GETTEXT, idx, (LPARAM)&name[0]);
                name.resize(n);
            }
        } else {
            name = Trim(GetText(hEdit));
        }
    }
    if (!add && name.empty()) {
        AppLog(L().logSelectPlayer);
        return;
    }
    if (!ValidPlayerName(name)) {
        AppLog(L().logInvalidPlayer);
        return;
    }
    if (!HasServerJar()) {
        Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring dir = GetServerDir();
    std::wstring path = dir + (isOp ? L"\\ops.json" : L"\\whitelist.json");
    auto list = ReadPlayerJsonList(path);
    std::string targetLow = ToLowerA(ToUtf8(name));

    if (add) {
        bool exists = false;
        for (const auto& e : list) if (ToLowerA(e.name) == targetLow) { exists = true; break; }
        if (!exists) {
            std::string uuid;
            EnterCriticalSection(&g_csUuid);
            auto it = g_playerUuids.find(ToLowerW(name));
            if (it != g_playerUuids.end()) uuid = ToUtf8(it->second);
            LeaveCriticalSection(&g_csUuid);
            list.push_back({ uuid, ToUtf8(name), 4, false });
            WritePlayerJsonList(path, list, isOp);
        }
        SetWindowTextW(hEdit, L"");
    } else {
        std::vector<JsonPlayerEntry> filtered;
        for (const auto& e : list) if (ToLowerA(e.name) != targetLow) filtered.push_back(e);
        WritePlayerJsonList(path, filtered, isOp);
    }

    if (g_state == SrvState::Running) {
        if (isOp) WriteToConsole((add ? L"op " : L"deop ") + name);
        else {
            WriteToConsole((add ? L"whitelist add " : L"whitelist remove ") + name);
            WriteToConsole(L"whitelist reload");
        }
    }
    LoadAccessListsToUi();
}

// ----------------------------------------------------------------------------
//  World Backups (<serverDir>\backups\*.zip)
// ----------------------------------------------------------------------------
static bool DirExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring BackupsDir() {
    std::wstring d = GetServerDir() + L"\\backups";
    CreateDirectoryW(d.c_str(), NULL);
    return d;
}

static void LoadBackupsListToUi() {
    if (!g_hListBk) return;
    SendMessageW(g_hListBk, LB_RESETCONTENT, 0, 0);
    g_backupFiles.clear();

    if (!HasServerJar()) {
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
        return;
    }

    std::wstring dir = GetServerDir() + L"\\backups";
    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW((dir + L"\\*.zip").c_str(), &fd);
    struct BkItem { std::wstring file, label; FILETIME ft; };
    std::vector<BkItem> items;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                unsigned long long bytes = ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
                double mb = (double)bytes / (1024.0 * 1024.0);
                wchar_t szBuf[64];
                swprintf(szBuf, 64, L"%.2f MB", mb);
                std::wstring lbl = std::wstring(fd.cFileName) + L"   (" + szBuf + L")";
                items.push_back({ fd.cFileName, lbl, fd.ftLastWriteTime });
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    std::sort(items.begin(), items.end(), [](const BkItem& a, const BkItem& b) {
        return CompareFileTime(&a.ft, &b.ft) > 0; // newest first
    });
    for (const auto& it : items) {
        g_backupFiles.push_back(it.file);
        SendMessageW(g_hListBk, LB_ADDSTRING, 0, (LPARAM)it.label.c_str());
    }
    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
}

struct BackupTaskArgs {
    HWND hwnd;
    bool restore;
    bool serverWasRunning;
    Lang lang;
    std::wstring srvDir;
    std::wstring zipPath;
    std::vector<std::wstring> worldFolders;
};

static DWORD WINAPI BackupWorkerThread(LPVOID param) {
    std::unique_ptr<BackupTaskArgs> a((BackupTaskArgs*)param);
    if (a->serverWasRunning && !a->restore) {
        Sleep(1500); // give server time to finish "save-all flush"
    }

    std::wstring cmd;
    if (!a->restore) {
        cmd = L"tar.exe -a -c --exclude=session.lock --exclude=*/session.lock -f \"" + a->zipPath + L"\"";
        for (const auto& wf : a->worldFolders) cmd += L" \"" + wf + L"\"";
    } else {
        cmd = L"tar.exe -x -f \"" + a->zipPath + L"\"";
    }

    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(0);
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    DWORD exitCode = 1;

    if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, a->srvDir.c_str(), &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    std::wstring msg;
    if (exitCode == 0) {
        msg = a->restore
            ? (a->lang == Lang::TR ? L"Yedek başarıyla geri yüklendi: " : L"Backup restored successfully: ") + a->zipPath
            : (a->lang == Lang::TR ? L"Dünya yedeği oluşturuldu: " : L"World backup created: ") + a->zipPath;
    } else {
        msg = a->restore
            ? (a->lang == Lang::TR ? L"Yedek geri yüklenemedi!" : L"Failed to restore backup!")
            : (a->lang == Lang::TR ? L"Yedek oluşturulamadı!" : L"Failed to create backup!");
    }

    g_backupBusy = false;
    SrvEvent* ev = new (std::nothrow) SrvEvent{ EV_BACKUP_DONE, 0, msg, NULL };
    if (ev && !PostMessageW(a->hwnd, WM_SC_EVENT, 0, (LPARAM)ev)) delete ev;
    return 0;
}

static void StartCreateBackup(bool silent = false) {
    if (g_backupBusy.load()) {
        if (!silent) AppLog(Tx(L"Şu anda başka bir yedekleme işlemi devam ediyor.", L"A backup operation is already in progress."));
        return;
    }
    if (!HasServerJar()) {
        if (silent) AppLog(Tx(L"[Yedekleme] Hata: Sunucu JAR dosyası seçilmedi.", L"[Backup] Error: Server JAR not specified."));
        else Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING);
        return;
    }
    std::wstring srvDir = GetServerDir();
    auto mp = ReadPropertiesMap(srvDir);
    std::wstring lvl = L"world";
    auto it = mp.find("level-name");
    if (it != mp.end() && !it->second.empty()) lvl = FromUtf8(it->second);

    std::vector<std::wstring> folders;
    for (const std::wstring& cand : { lvl, lvl + L"_nether", lvl + L"_the_end" }) {
        if (DirExists(srvDir + L"\\" + cand)) folders.push_back(cand);
    }
    if (folders.empty()) {
        if (silent) AppLog(Tx(L"[Yedekleme] Uyarı: Yedeklenecek dünya klasörü (world) bulunamadı.", L"[Backup] Warning: No world folder found to back up."));
        else Msg(Tx(L"Yedeklenecek dünya klasörü (world) bulunamadı.", L"No world folder found to back up."),
                 Tx(L"Yedekleme", L"Backup"), MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring tag = Trim(GetText(g_hEditBkTag));
    std::wstring cleanTag;
    for (wchar_t c : tag) if (iswalnum(c) || c == L'-' || c == L'_') cleanTag.push_back(c);

    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t ts[64];
    swprintf(ts, 64, L"%04d-%02d-%02d_%02d-%02d-%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    std::wstring fileName = L"backup_" + std::wstring(ts);
    if (!cleanTag.empty()) fileName += L"_" + cleanTag;
    fileName += L".zip";

    std::wstring zipPath = BackupsDir() + L"\\" + fileName;
    bool running = (g_state == SrvState::Running);
    if (running) WriteToConsole(L"save-all flush");

    g_backupBusy = true;
    EnableWindow(g_hBtnBkCreate, FALSE);
    AppLog(Tx(L"Dünya yedekleniyor, lütfen bekleyin…", L"Creating world backup, please wait…"));

    BackupTaskArgs* args = new (std::nothrow) BackupTaskArgs{ g_hwnd, false, running, g_lang, srvDir, zipPath, folders };
    HANDLE th = args ? CreateThread(NULL, 0, BackupWorkerThread, args, 0, NULL) : NULL;
    if (th) CloseHandle(th);
    else { delete args; g_backupBusy = false; EnableWindow(g_hBtnBkCreate, TRUE); }
}

static void RestoreSelectedBackup() {
    if (g_backupBusy.load()) return;
    if (!HasServerJar()) { Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING); return; }
    if (g_state != SrvState::Stopped) {
        Msg(Tx(L"Yedeği geri yüklemeden önce sunucuyu durdurmalısınız.", L"Please stop the server before restoring a backup."),
            Tx(L"Sunucu Açık", L"Server Running"), MB_OK | MB_ICONWARNING);
        return;
    }
    int idx = (int)SendMessageW(g_hListBk, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR || idx < 0 || idx >= (int)g_backupFiles.size()) {
        AppLog(Tx(L"Önce listeden bir yedek seçin.", L"Please select a backup from the list first."));
        return;
    }
    std::wstring file = g_backupFiles[idx];
    std::wstring q = (g_lang == Lang::TR)
        ? (L"\"" + file + L"\" yedeği mevcut dünyanın üzerine yazılacak.\nDevam edilsin mi?")
        : (L"Restore \"" + file + L"\" and overwrite the current world files?");
    if (Msg(q, Tx(L"Yedeği Geri Yükle", L"Restore Backup"), MB_YESNO | MB_ICONWARNING) != IDYES) return;

    std::wstring srvDir = GetServerDir();
    std::wstring zipPath = BackupsDir() + L"\\" + file;
    g_backupBusy = true;
    EnableWindow(g_hBtnBkRestore, FALSE);
    AppLog(Tx(L"Yedek geri yükleniyor…", L"Restoring backup…"));

    BackupTaskArgs* args = new (std::nothrow) BackupTaskArgs{ g_hwnd, true, false, g_lang, srvDir, zipPath, {} };
    HANDLE th = args ? CreateThread(NULL, 0, BackupWorkerThread, args, 0, NULL) : NULL;
    if (th) CloseHandle(th);
    else { delete args; g_backupBusy = false; EnableWindow(g_hBtnBkRestore, TRUE); }
}

static void DeleteSelectedBackup() {
    if (!HasServerJar()) { Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING); return; }
    int idx = (int)SendMessageW(g_hListBk, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR || idx < 0 || idx >= (int)g_backupFiles.size()) {
        AppLog(Tx(L"Önce listeden bir yedek seçin.", L"Please select a backup from the list first."));
        return;
    }
    std::wstring file = g_backupFiles[idx];
    std::wstring q = (g_lang == Lang::TR)
        ? (L"\"" + file + L"\" yedeği kalıcı olarak silinsin mi?")
        : (L"Permanently delete backup \"" + file + L"\"?");
    if (Msg(q, Tx(L"Yedeği Sil", L"Delete Backup"), MB_YESNO | MB_ICONWARNING) != IDYES) return;
    DeleteFileW((BackupsDir() + L"\\" + file).c_str());
    LoadBackupsListToUi();
}

// ----------------------------------------------------------------------------
//  Server Profiles (settings.ini sections named "Profile|<name>")
// ----------------------------------------------------------------------------
static std::wstring SanitizeProfileName(const std::wstring& in) {
    std::wstring o;
    for (wchar_t c : Trim(in))
        if (c >= 32 && c != L'[' && c != L']' && c != L'=' && c != L'|') o.push_back(c);
    return Trim(o);
}
static std::wstring ProfGet(const std::wstring& sec, const wchar_t* key) {
    wchar_t b[2048];
    GetPrivateProfileStringW(sec.c_str(), key, L"", b, 2048, SettingsPath().c_str());
    return b;
}

static void LoadProfilesListToUi() {
    if (!g_hListProf) return;
    g_profiles.clear();
    SendMessageW(g_hListProf, LB_RESETCONTENT, 0, 0);

    std::vector<wchar_t> buf(65536, 0);
    DWORD n = GetPrivateProfileSectionNamesW(buf.data(), (DWORD)buf.size(), SettingsPath().c_str());
    for (DWORD i = 0; i < n && buf[i]; ) {
        std::wstring sec = &buf[i];
        i += (DWORD)sec.size() + 1;
        if (sec.compare(0, 8, L"Profile|") != 0) continue;
        ServerProfile pr;
        pr.name = sec.substr(8);
        pr.jar  = ProfGet(sec, L"Jar");
        pr.ip   = ProfGet(sec, L"Ip");
        pr.port = ProfGet(sec, L"Port");
        pr.ram  = ProfGet(sec, L"Ram");
        pr.jvm  = ProfGet(sec, L"Jvm");
        g_profiles.push_back(pr);
    }
    std::sort(g_profiles.begin(), g_profiles.end(), [](const ServerProfile& a, const ServerProfile& b) {
        return ToLowerW(a.name) < ToLowerW(b.name);
    });
    for (const auto& pr : g_profiles) {
        std::wstring lbl = pr.name + L"   •   " + (pr.ram.empty() ? L"?" : pr.ram) +
                           L"   •   :" + (pr.port.empty() ? L"25565" : pr.port);
        SendMessageW(g_hListProf, LB_ADDSTRING, 0, (LPARAM)lbl.c_str());
    }
    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
}

static void SaveCurrentAsProfile() {
    std::wstring name = SanitizeProfileName(GetText(g_hEditProfName));
    if (name.empty()) {
        AppLog(Tx(L"Lütfen bir profil adı girin.", L"Please enter a profile name."));
        return;
    }
    std::wstring sec = L"Profile|" + name, path = SettingsPath();
    WritePrivateProfileStringW(sec.c_str(), L"Jar",  GetJarPath().c_str(),           path.c_str());
    WritePrivateProfileStringW(sec.c_str(), L"Ip",   Trim(GetText(g_hIp)).c_str(),   path.c_str());
    WritePrivateProfileStringW(sec.c_str(), L"Port", Trim(GetText(g_hPort)).c_str(), path.c_str());
    WritePrivateProfileStringW(sec.c_str(), L"Ram",  Trim(GetText(g_hRam)).c_str(),  path.c_str());
    WritePrivateProfileStringW(sec.c_str(), L"Jvm",  g_jvmArgs.c_str(),              path.c_str());
    SetWindowTextW(g_hEditProfName, L"");
    AppLog(std::wstring(Tx(L"Profil kaydedildi: ", L"Profile saved: ")) + name);
    LoadProfilesListToUi();
}

static void LoadSelectedProfile() {
    if (g_state != SrvState::Stopped) {
        Msg(Tx(L"Profil değiştirmeden önce sunucuyu durdurmalısınız.", L"Please stop the server before switching profiles."),
            Tx(L"Sunucu Açık", L"Server Running"), MB_OK | MB_ICONWARNING);
        return;
    }
    int idx = (int)SendMessageW(g_hListProf, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR || idx < 0 || idx >= (int)g_profiles.size()) {
        AppLog(Tx(L"Önce listeden bir profil seçin.", L"Please select a profile from the list first."));
        return;
    }
    const ServerProfile& pr = g_profiles[idx];
    SetWindowTextW(g_hJar,  pr.jar.c_str());
    SetWindowTextW(g_hIp,   pr.ip.c_str());
    SetWindowTextW(g_hPort, pr.port.empty() ? L"25565" : pr.port.c_str());
    SetWindowTextW(g_hRam,  pr.ram.empty() ? L"4G" : pr.ram.c_str());
    if (pr.jvm.find(L'"') == std::wstring::npos) {
        g_jvmArgs = pr.jvm;
        SetWindowTextW(g_hJvm, g_jvmArgs.c_str());
    }
    SaveSettings();
    AppLog(std::wstring(Tx(L"Profil yüklendi: ", L"Profile loaded: ")) + pr.name);
}

static void DeleteSelectedProfile() {
    int idx = (int)SendMessageW(g_hListProf, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR || idx < 0 || idx >= (int)g_profiles.size()) {
        AppLog(Tx(L"Önce listeden bir profil seçin.", L"Please select a profile from the list first."));
        return;
    }
    std::wstring name = g_profiles[idx].name;
    std::wstring q = (g_lang == Lang::TR)
        ? (L"\"" + name + L"\" profili silinsin mi?")
        : (L"Delete profile \"" + name + L"\"?");
    if (Msg(q, Tx(L"Profili Sil", L"Delete Profile"), MB_YESNO | MB_ICONWARNING) != IDYES) return;
    std::wstring sec = L"Profile|" + name;
    WritePrivateProfileStringW(sec.c_str(), NULL, NULL, SettingsPath().c_str());
    LoadProfilesListToUi();
}

// ----------------------------------------------------------------------------
//  Plugins & Mods Management (plugins\ and mods\)
// ----------------------------------------------------------------------------
static void LoadModFolder(const std::wstring& subDir, std::vector<ModItem>& outList, HWND hLb) {
    outList.clear();
    if (hLb) SendMessageW(hLb, LB_RESETCONTENT, 0, 0);
    if (!HasServerJar()) {
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
        return;
    }
    std::wstring srvDir = GetServerDir();
    std::wstring dir = srvDir + L"\\" + subDir;
    if (!DirExists(dir)) {
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
        return;
    }

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fn = fd.cFileName;
        bool isJar = fn.size() >= 4 && ToLowerW(fn.substr(fn.size() - 4)) == L".jar";
        bool isDisabled = fn.size() >= 13 && ToLowerW(fn.substr(fn.size() - 13)) == L".jar.disabled";
        if (!isJar && !isDisabled) continue;

        ModItem item;
        item.filename = fn;
        item.fullPath = dir + L"\\" + fn;
        item.enabled = isJar;
        if (isDisabled) item.displayName = fn.substr(0, fn.size() - 9);
        else item.displayName = fn;

        unsigned long long sz = ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        if (sz >= 1024 * 1024) {
            wchar_t b[32]; swprintf(b, 32, L"%.1f MB", sz / (1024.0 * 1024.0));
            item.sizeStr = b;
        } else {
            item.sizeStr = std::to_wstring(std::max(1ULL, sz / 1024ULL)) + L" KB";
        }
        outList.push_back(item);
    } while (FindNextFileW(h, &fd));
    FindClose(h);

    std::sort(outList.begin(), outList.end(), [](const ModItem& a, const ModItem& b) {
        if (a.enabled != b.enabled) return a.enabled > b.enabled;
        return ToLowerW(a.displayName) < ToLowerW(b.displayName);
    });

    if (hLb) {
        for (const auto& it : outList) {
            SendMessageW(hLb, LB_ADDSTRING, 0, (LPARAM)it.displayName.c_str());
        }
    }
    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
}

static void ToggleModItem(bool isPlugin) {
    HWND hLb = isPlugin ? g_hListPlugins : g_hListMods;
    auto& list = isPlugin ? g_pluginsList : g_modsList;
    int idx = (int)SendMessageW(hLb, LB_GETCURSEL, 0, 0);
    if (idx < 0 || idx >= (int)list.size()) {
        AppLog(Tx(L"Lütfen önce listeden bir öğe seçin.", L"Please select an item from the list first."));
        return;
    }
    ModItem& it = list[idx];
    std::wstring newPath;
    if (it.enabled) {
        newPath = it.fullPath + L".disabled";
    } else {
        if (it.fullPath.size() > 9) newPath = it.fullPath.substr(0, it.fullPath.size() - 9);
        else return;
    }

    if (FileExists(newPath)) {
        size_t sl = newPath.find_last_of(L"\\/");
        std::wstring targetName = (sl == std::wstring::npos) ? newPath : newPath.substr(sl + 1);
        AppLog(Tx(L"Hedef dosya zaten mevcut: ", L"Target file already exists: ") + targetName);
        return;
    }

    if (_wrename(it.fullPath.c_str(), newPath.c_str()) == 0) {
        std::wstring m = it.enabled
            ? (Tx(L"Devre dışı bırakıldı: ", L"Disabled: ") + it.displayName)
            : (Tx(L"Etkinleştirildi: ", L"Enabled: ") + it.displayName);
        AppLog(m);
        std::wstring curName = it.displayName;
        LoadModFolder(isPlugin ? L"plugins" : L"mods", list, hLb);
        for (size_t i = 0; i < list.size(); i++) {
            if (list[i].displayName == curName) {
                SendMessageW(hLb, LB_SETCURSEL, (WPARAM)i, 0);
                break;
            }
        }
    } else {
        AppLog(Tx(L"Dosya adı değiştirilemedi! (Sunucu çalışırken dosya kilitli olabilir)",
                  L"Failed to rename file! (The file may be locked by the server)"));
    }
}

static void AddModItem(bool isPlugin) {
    if (!HasServerJar()) {
        Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING);
        return;
    }
    std::wstring srvDir = GetServerDir();
    std::wstring subDir = isPlugin ? L"plugins" : L"mods";
    std::wstring destDir = srvDir + L"\\" + subDir;
    CreateDirectoryW(destDir.c_str(), NULL);

    wchar_t file[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Java Dosyaları (*.jar)\0*.jar\0Tüm Dosyalar (*.*)\0*.*\0";
    ofn.lpstrTitle = isPlugin ? Tx(L"Eklenti Seç (.jar)", L"Select Plugin (.jar)") : Tx(L"Mod Seç (.jar)", L"Select Mod (.jar)");
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring src = file;
        size_t sl = src.find_last_of(L"\\/");
        std::wstring fname = sl == std::wstring::npos ? src : src.substr(sl + 1);
        std::wstring target = destDir + L"\\" + fname;

        if (CopyFileW(src.c_str(), target.c_str(), FALSE)) {
            AppLog((isPlugin ? Tx(L"Eklenti eklendi: ", L"Plugin added: ") : Tx(L"Mod eklendi: ", L"Mod added: ")) + fname);
            LoadModFolder(subDir, isPlugin ? g_pluginsList : g_modsList, isPlugin ? g_hListPlugins : g_hListMods);
        } else {
            AppLog(Tx(L"Dosya kopyalanamadı!", L"Failed to copy file!"));
        }
    }
}

static void DeleteModItem(bool isPlugin) {
    HWND hLb = isPlugin ? g_hListPlugins : g_hListMods;
    auto& list = isPlugin ? g_pluginsList : g_modsList;
    int idx = (int)SendMessageW(hLb, LB_GETCURSEL, 0, 0);
    if (idx < 0 || idx >= (int)list.size()) {
        AppLog(Tx(L"Lütfen önce listeden bir öğe seçin.", L"Please select an item from the list first."));
        return;
    }
    const ModItem& it = list[idx];

    std::wstring q = (g_lang == Lang::TR)
        ? (L"\"" + it.displayName + L"\" dosyasını silmek istediğinize emin misiniz?")
        : (L"Are you sure you want to delete \"" + it.displayName + L"\"?");
    if (Msg(q, Tx(L"Silme Onayı", L"Confirm Delete"), MB_YESNO | MB_ICONWARNING) != IDYES) return;

    if (DeleteFileW(it.fullPath.c_str())) {
        AppLog(Tx(L"Dosya silindi: ", L"File deleted: ") + it.displayName);
        LoadModFolder(isPlugin ? L"plugins" : L"mods", list, hLb);
    } else {
        AppLog(Tx(L"Dosya silinemedi! (Sunucu açıkken kilitli olabilir)", L"Failed to delete file! (File may be locked)"));
    }
}

static void OpenModFolder(bool isPlugin) {
    if (!HasServerJar()) {
        Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING);
        return;
    }
    std::wstring srvDir = GetServerDir();
    std::wstring target = srvDir + L"\\" + (isPlugin ? L"plugins" : L"mods");
    CreateDirectoryW(target.c_str(), NULL);
    ShellExecuteW(g_hwnd, L"open", target.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

// ----------------------------------------------------------------------------
//  UI State, Theme & Language Application
// ----------------------------------------------------------------------------
static void UpdateUi() {
    bool stopped = g_state == SrvState::Stopped;
    bool active = g_state == SrvState::Starting || g_state == SrvState::Running;
    bool running = g_state == SrvState::Running;
    EnableWindow(g_hBtnStart, stopped);
    EnableWindow(g_hBtnStop, active);
    EnableWindow(g_hBtnBrowse, stopped);
    EnableWindow(g_hBtnKick, running);
    EnableWindow(g_hBtnBan, running);
    EnableWindow(g_hBtnUnban, running);
    EnableWindow(g_hBtnSend, active || g_state == SrvState::Stopping);
    HWND ro[4] = { g_hJar, g_hIp, g_hPort, g_hRam };
    for (HWND h : ro) SendMessageW(h, EM_SETREADONLY, !stopped, 0);
    TrayUpdate();
    InvalidateRect(g_hwnd, NULL, FALSE);
}

static void ApplyScrollTheme(HWND h) {
    if (!h) return;
    static HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (!ux) return;
    typedef HRESULT(WINAPI * PFN)(HWND, LPCWSTR, LPCWSTR);
    static PFN f = reinterpret_cast<PFN>(reinterpret_cast<void*>(GetProcAddress(ux, "SetWindowTheme")));
    if (f) {
        f(h, (g_theme != Theme::Light) ? L"DarkMode_Explorer" : L"Explorer", NULL);
        SetWindowPos(h, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

static void ApplyTitleBarTheme(HWND hwnd) {
    static HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;
    typedef HRESULT(WINAPI * PFN)(HWND, DWORD, LPCVOID, DWORD);
    static PFN f = reinterpret_cast<PFN>(reinterpret_cast<void*>(GetProcAddress(dwm, "DwmSetWindowAttribute")));
    if (!f) return;
    BOOL dark = (g_theme != Theme::Light) ? TRUE : FALSE;
    COLORREF cap = P.bg;
    if (FAILED(f(hwnd, 20, &dark, sizeof(dark)))) f(hwnd, 19, &dark, sizeof(dark));
    f(hwnd, 35, &cap, sizeof(cap));
    f(hwnd, 34, &cap, sizeof(cap));
}

static void ApplyTheme() {
    switch (g_theme) {
        case Theme::Light:     P = PAL_LIGHT; break;
        case Theme::Redstone:  P = PAL_REDSTONE; break;
        case Theme::Minecraft: P = PAL_MINECRAFT; break;
        case Theme::Dark:
        default:               P = PAL_DARK; break;
    }
    if (g_bBg) DeleteObject(g_bBg);
    if (g_bField) DeleteObject(g_bField);
    if (g_bLog) DeleteObject(g_bLog);
    g_bBg = CreateSolidBrush(P.bg);
    g_bField = CreateSolidBrush(P.field);
    g_bLog = CreateSolidBrush(P.logBg);

    if (g_hwnd) {
        ApplyTitleBarTheme(g_hwnd);
        ApplyScrollTheme(g_hLog);
        ApplyScrollTheme(g_hList);
        ApplyScrollTheme(g_hListWl);
        ApplyScrollTheme(g_hListOp);
        ApplyScrollTheme(g_hListBk);
        ApplyScrollTheme(g_hListProf);
        ApplyScrollTheme(g_hListPlugins);
        ApplyScrollTheme(g_hListMods);
        if (g_hLog) SendMessageW(g_hLog, EM_SETBKGNDCOLOR, 0, (LPARAM)P.logBg);

        // Only recolor log immediately if user is currently looking at the dashboard tab
        if (g_page == PG_PANEL) {
            ReapplyLogColors();
        } else {
            g_logThemeDirty = true;
        }

        // Re-synchronize current page control visibility and layout
        ShowPage();
    }
}

static void ApplyLanguage() {
    if (!g_hwnd) return;
    SetWindowTextW(g_hwnd, L().wndTitle);
    SetWindowTextW(g_hBtnBrowse, L().btnBrowse);
    SetWindowTextW(g_hBtnStart,  L().btnStart);
    SetWindowTextW(g_hBtnStop,   L().btnStop);
    SetWindowTextW(g_hBtnKick,   L().btnKick);
    SetWindowTextW(g_hBtnBan,    L().btnBan);
    SetWindowTextW(g_hBtnUnban,  L().btnUnban);
    SetWindowTextW(g_hBtnSend,   L().btnSend);

    g_f[F_JAR].caption   = L().capJar;
    g_f[F_IP].caption    = L().capIp;
    g_f[F_PORT].caption  = L().capPort;
    g_f[F_RAM].caption   = L().capRam;
    g_f[F_UNBAN].caption = L().capUnban;
    g_f[F_CMD].caption   = NULL;
    g_f[F_JVM].caption   = Tx(L"JVM argümanları", L"JVM arguments");
    g_f[F_MOTD].caption      = Tx(L"Sunucu açıklaması (MOTD)", L"Server description (MOTD)");
    g_f[F_MAXPL].caption     = Tx(L"Maks. oyuncu", L"Max players");
    g_f[F_VIEWDIST].caption  = Tx(L"Görüş mesafesi (chunk)", L"View distance (chunks)");
    g_f[F_SPAWNPROT].caption = Tx(L"Başlangıç koruması", L"Spawn protection");
    g_f[F_WL].caption        = Tx(L"Oyuncu adı", L"Player name");
    g_f[F_OP].caption        = Tx(L"Oyuncu adı", L"Player name");
    g_f[F_BK_TAG].caption    = Tx(L"Yedek etiketi (isteğe bağlı, ör: sezon1)", L"Backup label (optional, e.g. pre-update)");
    g_f[F_PROF_NAME].caption = Tx(L"Profil adı (ör: Survival 1.21)", L"Profile name (e.g. Survival 1.21)");
    g_f[F_BK_HOURS].caption      = Tx(L"Yedekleme aralığı (saat)", L"Backup interval (hours)");
    g_f[F_RESTART_TIME].caption  = Tx(L"Yeniden başlatma saati (SS:DD)", L"Restart time (HH:MM)");

    SetWindowTextW(g_hChkAutoStart,   L().menuAutoStart);
    SetWindowTextW(g_hChkAutoRestart, Tx(L"Sunucu çökerse otomatik yeniden başlat", L"Restart the server automatically if it crashes"));
    SetWindowTextW(g_hBtnJvmSave,     Tx(L"Kaydet", L"Save"));
    SetWindowTextW(g_hBtnLogs,        Tx(L"Log Klasörünü Aç", L"Open Logs Folder"));
    SetWindowTextW(g_hBtnThDark,      L().menuThemeDark);
    SetWindowTextW(g_hBtnThLight,     L().menuThemeLight);
    SetWindowTextW(g_hBtnThRedstone,  L().menuThemeRedstone);
    SetWindowTextW(g_hBtnThMc,        L().menuThemeMc);
    SetWindowTextW(g_hChkSchedBk,         Tx(L"Otomatik zamanlı yedeklemeyi etkinleştir", L"Enable scheduled auto-backups"));
    SetWindowTextW(g_hBtnSchedBkSave,     Tx(L"Kaydet", L"Save"));
    SetWindowTextW(g_hChkSchedRestart,    Tx(L"Günlük otomatik yeniden başlatmayı etkinleştir", L"Enable daily scheduled restart"));
    SetWindowTextW(g_hBtnSchedRestartSave, Tx(L"Kaydet", L"Save"));

    SetWindowTextW(g_hBtnPropSave,    Tx(L"Ayarları Kaydet", L"Save Properties"));
    SetWindowTextW(g_hBtnPropReload,  Tx(L"Yeniden Yükle", L"Reload"));
    SetWindowTextW(g_hBtnGm[0],       Tx(L"Hayatta Kalma", L"Survival"));
    SetWindowTextW(g_hBtnGm[1],       Tx(L"Yaratıcı", L"Creative"));
    SetWindowTextW(g_hBtnGm[2],       Tx(L"Macera", L"Adventure"));
    SetWindowTextW(g_hBtnGm[3],       Tx(L"İzleyici", L"Spectator"));
    SetWindowTextW(g_hBtnDf[0],       Tx(L"Barışçıl", L"Peaceful"));
    SetWindowTextW(g_hBtnDf[1],       Tx(L"Kolay", L"Easy"));
    SetWindowTextW(g_hBtnDf[2],       Tx(L"Normal", L"Normal"));
    SetWindowTextW(g_hBtnDf[3],       Tx(L"Zor", L"Hard"));
    SetWindowTextW(g_hChkOnline,      Tx(L"Orijinal Hesap Zorunlu (online-mode)", L"Require Premium Account (online-mode)"));
    SetWindowTextW(g_hChkPvp,         Tx(L"Oyuncular Arası Hasar (PVP)", L"Player vs Player Damage (PVP)"));
    SetWindowTextW(g_hChkWhitelist,   Tx(L"Whitelist Zorunlu (white-list)", L"Enforce Whitelist (white-list)"));
    SetWindowTextW(g_hChkFlight,      Tx(L"Uçmaya İzin Ver (allow-flight)", L"Allow Flight (allow-flight)"));
    SetWindowTextW(g_hChkCmdBlock,    Tx(L"Komut Bloklarını Etkinleştir (enable-command-block)", L"Enable Command Blocks (enable-command-block)"));

    SetWindowTextW(g_hBtnWlAdd,       Tx(L"Whitelist Ekle", L"Add to Whitelist"));
    SetWindowTextW(g_hBtnWlRem,       Tx(L"Seçileni Çıkar", L"Remove Selected"));
    SetWindowTextW(g_hBtnWlReload,    Tx(L"Yenile", L"Reload"));
    SetWindowTextW(g_hBtnOpAdd,       Tx(L"OP Ver", L"Grant OP"));
    SetWindowTextW(g_hBtnOpRem,       Tx(L"OP Al (Deop)", L"Revoke OP"));
    SetWindowTextW(g_hBtnOpReload,    Tx(L"Yenile", L"Reload"));

    SetWindowTextW(g_hBtnBkCreate,    Tx(L"Şimdi Yedek Oluştur (.zip)", L"Create Backup Now (.zip)"));
    SetWindowTextW(g_hBtnBkOpen,      Tx(L"Yedekler Klasörünü Aç", L"Open Backups Folder"));
    SetWindowTextW(g_hBtnBkRestore,   Tx(L"Seçileni Geri Yükle", L"Restore Selected"));
    SetWindowTextW(g_hBtnBkDel,       Tx(L"Seçileni Sil", L"Delete Selected"));
    SetWindowTextW(g_hBtnBkReload,    Tx(L"Yenile", L"Reload"));

    SetWindowTextW(g_hBtnProfSave,    Tx(L"Mevcut Ayarları Profil Olarak Kaydet", L"Save Current Settings as Profile"));
    SetWindowTextW(g_hBtnProfLoad,    Tx(L"Seçili Profili Yükle (Aktif Et)", L"Load Selected Profile"));
    SetWindowTextW(g_hBtnProfDel,     Tx(L"Seçili Profili Sil", L"Delete Selected Profile"));

    SetWindowTextW(g_hBtnPlugToggle, Tx(L"Durumu Değiştir (Aktif / Devre Dışı)", L"Toggle Status (Active / Disabled)"));
    SetWindowTextW(g_hBtnPlugAdd,    Tx(L"+ Ekle", L"+ Add"));
    SetWindowTextW(g_hBtnPlugDel,    Tx(L"Sil", L"Delete"));
    SetWindowTextW(g_hBtnPlugOpen,   Tx(L"Klasör", L"Folder"));
    SetWindowTextW(g_hBtnPlugReload, Tx(L"Yenile", L"Reload"));

    SetWindowTextW(g_hBtnModToggle,  Tx(L"Durumu Değiştir (Aktif / Devre Dışı)", L"Toggle Status (Active / Disabled)"));
    SetWindowTextW(g_hBtnModAdd,     Tx(L"+ Ekle", L"+ Add"));
    SetWindowTextW(g_hBtnModDel,     Tx(L"Sil", L"Delete"));
    SetWindowTextW(g_hBtnModOpen,    Tx(L"Klasör", L"Folder"));
    SetWindowTextW(g_hBtnModReload,  Tx(L"Yenile", L"Reload"));

    g_lastTip.clear();
    TrayUpdate();
    RedrawWindow(g_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

// ----------------------------------------------------------------------------
//  Server Lifecycle
// ----------------------------------------------------------------------------
static std::wstring OpenJarDialog() {
    wchar_t file[MAX_PATH] = {};
    std::wstring cur = Trim(GetText(g_hJar));
    if (cur.size() < MAX_PATH) wcscpy(file, cur.c_str());
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = g_hwnd;
    ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L().dlgFilterJar;
    ofn.lpstrTitle = L().dlgTitleJar;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
    return GetOpenFileNameW(&ofn) ? std::wstring(file) : L"";
}

static void StartServer() {
    if (g_state != SrvState::Stopped) { Msg(L().msgAlreadyRunning, L"Redstone Server Panel"); return; }

    std::wstring jar = GetJarPath(), ip = Trim(GetText(g_hIp));
    std::wstring port = Trim(GetText(g_hPort)), ram = Trim(GetText(g_hRam));

    if (jar.empty())        { Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING); return; }
    if (!FileExists(jar))   { Msg(std::wstring(L().msgJarNotFound) + jar, L().titleFileNotFound, MB_OK | MB_ICONERROR); return; }
    if (!ValidRam(ram))     { Msg(L().msgInvalidRam, L().titleInvalidVal, MB_OK | MB_ICONWARNING); return; }
    {
        // Warn when the requested heap exceeds the machine's physical memory
        // (-Xms equals -Xmx here, so the JVM would fail or page heavily).
        MEMORYSTATUSEX ms = {}; ms.dwLength = sizeof(ms);
        if (GlobalMemoryStatusEx(&ms)) {
            unsigned long long reqMb = (unsigned long long)_wtoi(ram.substr(0, ram.size() - 1).c_str());
            if (towupper(ram.back()) == L'G') reqMb *= 1024;
            unsigned long long totMb = ms.ullTotalPhys / (1024ULL * 1024ULL);
            if (reqMb > totMb) {
                std::wstring m = g_lang == Lang::TR
                    ? L"İstenen bellek (" + ram + L"), bilgisayarın toplam RAM'inden (" + std::to_wstring(totMb) + L" MB) fazla. Yine de başlatılsın mı?"
                    : L"Requested memory (" + ram + L") exceeds the system's total RAM (" + std::to_wstring(totMb) + L" MB). Start anyway?";
                if (Msg(m, L().titleInvalidVal, MB_YESNO | MB_ICONWARNING) != IDYES) return;
            }
        }
    }
    if (!port.empty() && !ValidPort(port)) { Msg(L().msgInvalidPort, L().titleInvalidVal, MB_OK | MB_ICONWARNING); return; }
    if (!ValidIp(ip))       { Msg(L().msgInvalidIp, L().titleInvalidVal, MB_OK | MB_ICONWARNING); return; }

    std::wstring java = FindJava();
    if (java.empty()) {
        Msg(L().msgJavaNotFound, L().titleJavaMissing, MB_OK | MB_ICONERROR);
        return;
    }

    size_t sl = jar.find_last_of(L"\\/");
    std::wstring dir = sl == std::wstring::npos ? L"." : jar.substr(0, sl);

    if (!EulaAccepted(dir)) {
        if (Msg(L().msgEulaPrompt, L().titleEula, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
        WriteAll(dir + L"\\eula.txt", "# Accepted via Redstone Server Panel\neula=true\n");
    }

    std::vector<std::pair<std::string, std::string>> kv;
    if (!port.empty()) kv.push_back({ "server-port", ToUtf8(port) });
    if (!ip.empty())   kv.push_back({ "server-ip", ToUtf8(ip) });
    if (!kv.empty()) UpdateProperties(dir, kv);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE outRd = NULL, outWr = NULL, inRd = NULL, inWr = NULL;
    if (!CreatePipe(&outRd, &outWr, &sa, 0)) { Msg(L().msgPipeErr, L().titleError, MB_OK | MB_ICONERROR); return; }
    if (!SetHandleInformation(outRd, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(outRd); CloseHandle(outWr);
        Msg(L().msgPipeErr, L().titleError, MB_OK | MB_ICONERROR); return;
    }
    if (!CreatePipe(&inRd, &inWr, &sa, 0)) {
        CloseHandle(outRd); CloseHandle(outWr);
        Msg(L().msgPipeErr, L().titleError, MB_OK | MB_ICONERROR); return;
    }
    if (!SetHandleInformation(inWr, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(outRd); CloseHandle(outWr);
        CloseHandle(inRd); CloseHandle(inWr);
        Msg(L().msgPipeErr, L().titleError, MB_OK | MB_ICONERROR); return;
    }

    std::wstring cmd = L"\"" + java + L"\" -Xms" + ram + L" -Xmx" + ram +
        L" " + g_jvmArgs + L" -Dfile.encoding=UTF-8 -Dstdout.encoding=UTF-8 -Dstderr.encoding=UTF-8 -jar \"" + jar + L"\" nogui";
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end()); cmdBuf.push_back(0);

    STARTUPINFOW si = {}; si.cb = sizeof(si);
    si.hStdError = outWr; si.hStdOutput = outWr; si.hStdInput = inRd;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW,
                             NULL, dir.c_str(), &si, &pi);
    DWORD err = GetLastError();
    CloseHandle(outWr); CloseHandle(inRd);
    if (!ok) {
        CloseHandle(outRd); CloseHandle(inWr);
        Msg(std::wstring(L().msgStartErr) + SysError(err), L().titleError, MB_OK | MB_ICONERROR);
        return;
    }
    if (pi.hThread) CloseHandle(pi.hThread);

    if (g_hProc) { CloseHandle(g_hProc); g_hProc = NULL; }
    if (g_hStdinWr) { CloseHandle(g_hStdinWr); g_hStdinWr = NULL; }
    g_hProc = pi.hProcess; g_hStdinWr = inWr;
    ++g_gen; g_pendingTime = 0; g_useTimelineCmd = false; g_suppressErrLines = 0;
    KillTimer(g_hwnd, TID_RESTART);
    g_schedRestartPending = false;
    g_scheduledRestartTrigger = false;
    g_lastAutoBackupTime = 0;
    g_restartWarningStep = 0;
    g_startTick = GetTickCount64();
    g_state = SrvState::Starting;
    g_igTime = L"--:--"; g_upText = L"00:00:00";
    UpdateTelemetryStats();
    SendMessageW(g_hList, LB_RESETCONTENT, 0, 0);

    ReaderArgs* ra = new (std::nothrow) ReaderArgs{ g_hwnd, outRd, g_gen };
    HANDLE t = ra ? CreateThread(NULL, 0, ReaderThread, ra, 0, NULL) : NULL;
    if (!t) {
        delete ra; CloseHandle(outRd); TerminateProcess(g_hProc, 1);
        CloseHandle(g_hProc); g_hProc = NULL; CloseHandle(g_hStdinWr); g_hStdinWr = NULL;
        g_state = SrvState::Stopped; UpdateTelemetryStats(); UpdateUi();
        Msg(L().msgThreadErr, L().titleError, MB_OK | MB_ICONERROR);
        return;
    }
    CloseHandle(t);

    SetTimer(g_hwnd, TID_STATS, 1000, NULL);
    SaveSettings();
    AppLog(std::wstring(L().logStarting) + ram + L().logWorkDir + dir + L")");
    UpdateUi();
    Balloon(L"Redstone Server Panel", L().balloonStarting);
}

static void StopServer(bool fromScheduledRestart = false) {
    if (g_state != SrvState::Starting && g_state != SrvState::Running) return;
    if (!fromScheduledRestart) {
        g_schedRestartPending = false;
        g_scheduledRestartTrigger = false;
        KillTimer(g_hwnd, TID_RESTART);
    }
    g_state = SrvState::Stopping;
    WriteToConsole(L"stop");
    SetTimer(g_hwnd, TID_STOPWATCH, 60000, NULL);
    AppLog(L().logStopping);
    UpdateUi();
}

static void OnServerExited() {
    DWORD code = 0;
    if (g_hProc) { WaitForSingleObject(g_hProc, 3000); GetExitCodeProcess(g_hProc, &code); CloseHandle(g_hProc); g_hProc = NULL; }
    if (g_hStdinWr) { CloseHandle(g_hStdinWr); g_hStdinWr = NULL; }
    bool expected = (g_state == SrvState::Stopping);
    ULONGLONG upSec = (GetTickCount64() - g_startTick) / 1000;
    KillTimer(g_hwnd, TID_TIME); KillTimer(g_hwnd, TID_STOPWATCH);
    g_state = SrvState::Stopped;
    SendMessageW(g_hList, LB_RESETCONTENT, 0, 0);
    g_igTime = L"--:--"; g_upText = L"00:00:00";
    UpdateTelemetryStats();

    if (!expected && code != 0) {
        g_schedRestartPending = false;
        AppLog(std::wstring(L().logCrashed) + std::to_wstring((long long)(int)code) + L").");
        Balloon(L"Redstone Server Panel", L().balloonCrashed);
        if (g_autoRestart && !g_quitPending) {
            // Servers that die within 2 minutes count as a crash loop; give up after 3 tries.
            g_crashCount = (upSec < 120) ? g_crashCount + 1 : 1;
            if (g_crashCount <= 3) {
                AppLog(g_lang == Lang::TR ? L"10 saniye içinde otomatik yeniden başlatılacak…"
                                          : L"Restarting automatically in 10 seconds…");
                SetTimer(g_hwnd, TID_RESTART, 10000, NULL);
            } else {
                AppLog(g_lang == Lang::TR ? L"Art arda çökme: otomatik yeniden başlatma durduruldu."
                                          : L"Repeated crashes: automatic restart stopped.");
                g_crashCount = 0;
            }
        }
    } else {
        g_crashCount = 0;
        AppLog(L().logStopped);
        Balloon(L"Redstone Server Panel", L().balloonStopped);
        if (g_schedRestartPending && !g_quitPending) {
            g_schedRestartPending = false;
            g_scheduledRestartTrigger = true;
            AppLog(Tx(L"[Otomasyon] Planlı yeniden başlatma için sunucu 5 saniye içinde açılacak…",
                      L"[Automation] Scheduled restart: starting server in 5 seconds…"));
            SetTimer(g_hwnd, TID_RESTART, 5000, NULL);
        }
    }
    UpdateUi();
    if (g_quitPending) DestroyWindow(g_hwnd);
}

static void RequestExit() {
    if (g_state == SrvState::Stopped) { DestroyWindow(g_hwnd); return; }
    if (!g_quitPending) {
        if (Msg(L().msgExitConfirm, L().titleExit, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
        g_quitPending = true;
    }
    if (g_state == SrvState::Stopped) { DestroyWindow(g_hwnd); return; }
    StopServer();
}

static void QueryTime() {
    ++g_pendingTime;
    const wchar_t* cmd = g_useTimelineCmd.load() ? L"time query day" : L"time query daytime";
    if (!WriteToConsole(cmd)) --g_pendingTime;
    if (g_pendingTime.load() > 3) g_pendingTime = 0;
}

// ----------------------------------------------------------------------------
//  Player Actions & Console Commands
// ----------------------------------------------------------------------------
//Listbox helper for any owner-draw player list
static std::wstring ListTextFrom(HWND hLb, int idx) {
    if (!hLb || idx < 0) return L"";
    int n = (int)SendMessageW(hLb, LB_GETTEXTLEN, idx, 0);
    if (n <= 0) return L"";
    std::wstring s(n + 1, L'\0');
    SendMessageW(hLb, LB_GETTEXT, idx, (LPARAM)&s[0]);
    s.resize(n);
    return s;
}
static std::wstring ListText(int idx) { return ListTextFrom(g_hList, idx); }
static void DoKickBan(bool ban) {
    int idx = (int)SendMessageW(g_hList, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR) { AppLog(L().logSelectPlayer); return; }
    std::wstring name = ListText(idx);
    if (!ValidPlayerName(name)) return;
    if (ban && Msg(L"\"" + name + L"\"" + L().msgBanConfirm, L().titleBanConfirm, MB_YESNO | MB_ICONWARNING) != IDYES) return;
    WriteToConsole((ban ? L"ban " : L"kick ") + name);
}
static void DoUnban() {
    std::wstring n = Trim(GetText(g_hUnban));
    if (n.empty()) return;
    if (!ValidPlayerName(n)) { AppLog(L().logInvalidPlayer); return; }
    WriteToConsole(L"pardon " + n);
    AppLog(std::wstring(L().logUnbanReq) + n);
    SetWindowTextW(g_hUnban, L"");
}
static void SaveJvmFromUi() {
    std::wstring a = Trim(GetText(g_hJvm));
    if (a.find(L'"') != std::wstring::npos) {
        AppLog(Tx(L"JVM argümanları çift tırnak içeremez.", L"JVM arguments cannot contain double quotes."));
        return;
    }
    g_jvmArgs = a;
    SaveSettings();
    AppLog(Tx(L"JVM argümanları kaydedildi (sonraki başlatmada geçerli).", L"JVM arguments saved (applied on next start)."));
}
// ----------------------------------------------------------------------------
//  Console Auto-Completion (Tab Completion)
// ----------------------------------------------------------------------------
static const wchar_t* const MC_AUTO_CMDS[] = {
    L"advancement", L"attribute", L"ban", L"ban-ip", L"banlist", L"bossbar",
    L"clear", L"clone", L"datapack", L"defaultgamemode", L"deop", L"difficulty",
    L"effect", L"enchant", L"execute", L"experience", L"fill", L"forceload",
    L"function", L"gamemode", L"gamerule", L"give", L"help", L"kick", L"kill",
    L"list", L"locate", L"loot", L"me", L"msg", L"op", L"pardon", L"pardon-ip",
    L"particle", L"playsound", L"reload", L"save-all", L"save-off", L"save-on",
    L"say", L"schedule", L"scoreboard", L"seed", L"setblock", L"setworldspawn",
    L"spawnpoint", L"spectate", L"spreadplayers", L"stop", L"stopsound", L"summon",
    L"tag", L"team", L"teleport", L"tell", L"tellraw", L"time", L"title",
    L"tm", L"tp", L"trigger", L"weather", L"whisper", L"whitelist", L"worldborder", L"xp"
};

struct TabCompleteState {
    bool active = false;
    std::wstring originalPrefix;
    std::wstring originalSuffix;
    std::vector<std::wstring> matches;
    int matchIndex = 0;
};
static TabCompleteState g_tabState;

static void ResetTabCompletion() {
    g_tabState.active = false;
    g_tabState.originalPrefix.clear();
    g_tabState.originalSuffix.clear();
    g_tabState.matches.clear();
    g_tabState.matchIndex = 0;
}

static std::vector<std::wstring> CollectKnownPlayers() {
    std::vector<std::wstring> result;
    std::set<std::wstring> seen;

    auto addFromList = [&](HWND hList) {
        if (!hList) return;
        int count = (int)SendMessageW(hList, LB_GETCOUNT, 0, 0);
        for (int i = 0; i < count; i++) {
            int len = (int)SendMessageW(hList, LB_GETTEXTLEN, i, 0);
            if (len > 0) {
                std::wstring name(len + 1, L'\0');
                SendMessageW(hList, LB_GETTEXT, i, (LPARAM)&name[0]);
                name.resize(len);
                if (ValidPlayerName(name)) {
                    std::wstring lower = ToLowerW(name);
                    if (seen.find(lower) == seen.end()) {
                        seen.insert(lower);
                        result.push_back(name);
                    }
                }
            }
        }
    };

    // Priority: 1. Online players, 2. Operators, 3. Whitelisted players
    addFromList(g_hList);
    addFromList(g_hListOp);
    addFromList(g_hListWl);
    return result;
}

static void HandleConsoleTabCompletion() {
    if (!g_hCmd) return;

    bool shift = (GetKeyState(VK_SHIFT) < 0);

    // If already in tab-cycling mode, advance or reverse to the next match
    if (g_tabState.active && !g_tabState.matches.empty()) {
        int n = (int)g_tabState.matches.size();
        if (shift) {
            g_tabState.matchIndex = (g_tabState.matchIndex - 1 + n) % n;
        } else {
            g_tabState.matchIndex = (g_tabState.matchIndex + 1) % n;
        }
        std::wstring cand = g_tabState.matches[g_tabState.matchIndex];
        std::wstring newLine = g_tabState.originalPrefix + cand;
        if (g_tabState.originalSuffix.empty() || g_tabState.originalSuffix[0] != L' ') {
            newLine += L" ";
        }
        newLine += g_tabState.originalSuffix;
        SetWindowTextW(g_hCmd, newLine.c_str());
        int caretPos = (int)(g_tabState.originalPrefix.size() + cand.size() + 1);
        SendMessageW(g_hCmd, EM_SETSEL, (WPARAM)caretPos, (LPARAM)caretPos);
        return;
    }

    std::wstring text = GetText(g_hCmd);
    DWORD selStart = 0, selEnd = 0;
    SendMessageW(g_hCmd, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);
    int caret = (int)selStart;
    if (caret < 0) caret = 0;
    if (caret > (int)text.size()) caret = (int)text.size();

    // Identify token before caret
    int tokenStart = caret;
    while (tokenStart > 0 && text[tokenStart - 1] != L' ') {
        tokenStart--;
    }
    int tokenEnd = caret;
    while (tokenEnd < (int)text.size() && text[tokenEnd] != L' ') {
        tokenEnd++;
    }

    std::wstring prefixBefore = text.substr(0, tokenStart);
    std::wstring token = text.substr(tokenStart, caret - tokenStart);
    std::wstring suffixAfter = text.substr(tokenEnd);

    // Split words in prefixBefore to determine command context
    std::vector<std::wstring> words;
    {
        size_t p = 0;
        while (p < prefixBefore.size()) {
            while (p < prefixBefore.size() && prefixBefore[p] == L' ') p++;
            if (p >= prefixBefore.size()) break;
            size_t wEnd = prefixBefore.find(L' ', p);
            if (wEnd == std::wstring::npos) wEnd = prefixBefore.size();
            words.push_back(prefixBefore.substr(p, wEnd - p));
            p = wEnd;
        }
    }

    std::vector<std::wstring> candidates;

    if (words.empty()) {
        // First token -> commands
        bool hasSlash = (!token.empty() && token[0] == L'/');
        for (const wchar_t* cmd : MC_AUTO_CMDS) {
            std::wstring item = hasSlash ? (L"/" + std::wstring(cmd)) : std::wstring(cmd);
            candidates.push_back(item);
        }
    } else {
        std::wstring cmd = words[0];
        if (!cmd.empty() && cmd[0] == L'/') cmd.erase(0, 1);
        std::wstring cmdLow = ToLowerW(cmd);

        // Command-specific sub-arguments
        if (cmdLow == L"gamemode" && words.size() == 1) {
            candidates.push_back(L"survival");
            candidates.push_back(L"creative");
            candidates.push_back(L"adventure");
            candidates.push_back(L"spectator");
        } else if (cmdLow == L"difficulty" && words.size() == 1) {
            candidates.push_back(L"peaceful");
            candidates.push_back(L"easy");
            candidates.push_back(L"normal");
            candidates.push_back(L"hard");
        } else if (cmdLow == L"weather" && words.size() == 1) {
            candidates.push_back(L"clear");
            candidates.push_back(L"rain");
            candidates.push_back(L"thunder");
        } else if (cmdLow == L"time" && words.size() == 1) {
            candidates.push_back(L"set");
            candidates.push_back(L"add");
            candidates.push_back(L"query");
        } else if (cmdLow == L"time" && words.size() == 2 && ToLowerW(words[1]) == L"set") {
            candidates.push_back(L"day");
            candidates.push_back(L"night");
            candidates.push_back(L"noon");
            candidates.push_back(L"midnight");
        } else if (cmdLow == L"whitelist" && words.size() == 1) {
            candidates.push_back(L"add");
            candidates.push_back(L"remove");
            candidates.push_back(L"list");
            candidates.push_back(L"reload");
            candidates.push_back(L"on");
            candidates.push_back(L"off");
        }

        // Add player names
        std::vector<std::wstring> players = CollectKnownPlayers();
        for (const auto& pl : players) candidates.push_back(pl);

        // Minecraft target selectors
        candidates.push_back(L"@a");
        candidates.push_back(L"@p");
        candidates.push_back(L"@r");
        candidates.push_back(L"@s");
        candidates.push_back(L"@e");
    }

    std::wstring tokenLow = ToLowerW(token);
    std::vector<std::wstring> matches;
    for (const auto& c : candidates) {
        std::wstring cLow = ToLowerW(c);
        if (tokenLow.empty() || (cLow.size() >= tokenLow.size() && cLow.compare(0, tokenLow.size(), tokenLow) == 0)) {
            if (std::find(matches.begin(), matches.end(), c) == matches.end()) {
                matches.push_back(c);
            }
        }
    }

    if (matches.empty()) return;

    g_tabState.active = true;
    g_tabState.originalPrefix = prefixBefore;
    g_tabState.originalSuffix = suffixAfter;
    g_tabState.matches = matches;
    g_tabState.matchIndex = shift ? ((int)matches.size() - 1) : 0;

    std::wstring cand = g_tabState.matches[g_tabState.matchIndex];
    std::wstring newLine = prefixBefore + cand;
    if (suffixAfter.empty() || suffixAfter[0] != L' ') {
        newLine += L" ";
    }
    newLine += suffixAfter;

    SetWindowTextW(g_hCmd, newLine.c_str());
    int caretPos = (int)(prefixBefore.size() + cand.size() + 1);
    SendMessageW(g_hCmd, EM_SETSEL, (WPARAM)caretPos, (LPARAM)caretPos);
}

static void SendCommandFromUi() {
    ResetTabCompletion();
    std::wstring c = Trim(GetText(g_hCmd));
    if (c.empty()) return;
    if (!c.empty() && c[0] == L'/') c.erase(0, 1);
    if (!WriteToConsole(c)) { AppLog(L().logSrvNotRunning); return; }
    AppendLog(L"> " + c + L"\r\n");
    if (g_hLog) {
        SendMessageW(g_hLog, WM_VSCROLL, SB_BOTTOM, 0);
        LockConsoleHorizontalScroll();
    }
    g_hist.push_back(c); g_histPos = (int)g_hist.size();
    SetWindowTextW(g_hCmd, L"");
}
static void HistoryNav(int d) {
    ResetTabCompletion();
    if (g_hist.empty()) return;
    g_histPos = std::max(0, std::min((int)g_hist.size(), g_histPos + d));
    std::wstring t = g_histPos == (int)g_hist.size() ? L"" : g_hist[g_histPos];
    SetWindowTextW(g_hCmd, t.c_str());
    SendMessageW(g_hCmd, EM_SETSEL, (WPARAM)t.size(), (LPARAM)t.size());
}

// ----------------------------------------------------------------------------
//  Subclasses: Enter/Tab navigation, Button Hover
// ----------------------------------------------------------------------------
static void FocusNext(HWND h) {
    bool shift = GetKeyState(VK_SHIFT) < 0;
    HWND n = GetNextDlgTabItem(g_hwnd, h, shift);
    if (n) SetFocus(n);
}
static LRESULT CALLBACK EditProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (h == g_hCmd) {
        if (m == WM_KEYDOWN) {
            if (w == VK_ESCAPE) {
                ResetTabCompletion();
                SetWindowTextW(g_hCmd, L"");
                return 0;
            }
            if (w == VK_TAB) {
                std::wstring text = GetText(g_hCmd);
                if (text.empty()) {
                    FocusNext(h);
                    return 0;
                }
                HandleConsoleTabCompletion();
                return 0;
            } else if (w != VK_SHIFT && w != VK_CONTROL && w != VK_MENU) {
                ResetTabCompletion();
            }
        } else if (m == WM_LBUTTONDOWN) {
            ResetTabCompletion();
        } else if (m == WM_CHAR) {
            if (w == VK_TAB || w == VK_ESCAPE) return 0;
            ResetTabCompletion();
        }
    }

    switch (m) {
        case WM_KEYDOWN:
            if (w == VK_TAB) { FocusNext(h); return 0; }
            if (w == VK_RETURN) {
                if (h == g_hCmd) SendCommandFromUi();
                else if (h == g_hUnban) DoUnban();
                else if (h == g_hJvm) SaveJvmFromUi();
                else if (h == g_hEditWl) ModifyAccessList(false, true);
                else if (h == g_hEditOp) ModifyAccessList(true, true);
                else if (h == g_hEditProfName) SaveCurrentAsProfile();
                else if (h == g_hEditBkHours) PostMessageW(g_hwnd, WM_COMMAND, ID_BTN_SCHED_BK_SAVE, 0);
                else if (h == g_hEditRestartTime) PostMessageW(g_hwnd, WM_COMMAND, ID_BTN_SCHED_RESTART_SAVE, 0);
                return 0;
            }
            if (h == g_hCmd && (w == VK_UP || w == VK_DOWN)) { HistoryNav(w == VK_UP ? -1 : 1); return 0; }
            break;
        case WM_CHAR:
            if (w == VK_TAB || w == VK_RETURN) return 0;
            break;
    }
    return CallWindowProcW(g_oldEdit, h, m, w, l);
}
static LRESULT CALLBACK BtnProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
        case WM_MOUSEMOVE:
            if (g_hHover != h) {
                g_hHover = h;
                TRACKMOUSEEVENT t = { sizeof(t), TME_LEAVE, h, 0 };
                TrackMouseEvent(&t);
                InvalidateRect(h, NULL, FALSE);
            }
            break;
        case WM_MOUSELEAVE:
            if (g_hHover == h) { g_hHover = NULL; InvalidateRect(h, NULL, FALSE); }
            break;
        case WM_SETCURSOR:
            if (IsWindowEnabled(h)) { SetCursor(LoadCursor(NULL, IDC_HAND)); return TRUE; }
            break;
        case WM_KEYDOWN:
            if (w == VK_TAB) { FocusNext(h); return 0; }
            break;
        case WM_CHAR:
            if (w == VK_TAB) return 0;
            break;
        case WM_ERASEBKGND: return 1;
    }
    return CallWindowProcW(g_oldBtn, h, m, w, l);
}

// ----------------------------------------------------------------------------
//  Custom GDI Painting
// ----------------------------------------------------------------------------
static void FillRound(HDC dc, const RECT& r, COLORREF fill, COLORREF border, int rad) {
    HBRUSH b = CreateSolidBrush(fill); HPEN p = CreatePen(PS_SOLID, 1, border);
    if (b && p) {
        HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
        RoundRect(dc, r.left, r.top, r.right, r.bottom, rad, rad);
        SelectObject(dc, ob); SelectObject(dc, op);
    }
    if (b) DeleteObject(b);
    if (p) DeleteObject(p);
}
static void DrawTxt(HDC dc, const std::wstring& t, RECT r, HFONT f, COLORREF c, UINT fmt) {
    HGDIOBJ of = SelectObject(dc, f);
    SetTextColor(dc, c); SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, t.c_str(), -1, &r, fmt | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(dc, of);
}

static void FillRc(HDC dc, const RECT& r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    if (b) { FillRect(dc, &r, b); DeleteObject(b); }
}

static int GetMonitorRefreshRate(HWND hwnd) {
    if (hwnd) {
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (hMon) {
            MONITORINFOEXW mi;
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoW(hMon, &mi)) {
                DEVMODEW dm;
                ZeroMemory(&dm, sizeof(dm));
                dm.dmSize = sizeof(dm);
                if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm)) {
                    if (dm.dmDisplayFrequency >= 30 && dm.dmDisplayFrequency <= 500) {
                        return (int)dm.dmDisplayFrequency;
                    }
                }
            }
        }
        HDC hdc = GetDC(hwnd);
        if (hdc) {
            int v = GetDeviceCaps(hdc, VREFRESH);
            ReleaseDC(hwnd, hdc);
            if (v >= 30 && v <= 500) return v;
        }
    }
    return 60;
}

static void InitHighResTimer() {
    static bool s_inited = false;
    if (!s_inited) {
        s_inited = true;
        HMODULE hMm = LoadLibraryW(L"winmm.dll");
        if (hMm) {
            typedef MMRESULT (WINAPI *PfnTimeBeginPeriod)(UINT);
            PfnTimeBeginPeriod pfn = reinterpret_cast<PfnTimeBeginPeriod>(reinterpret_cast<void*>(GetProcAddress(hMm, "timeBeginPeriod")));
            if (pfn) pfn(1);
        }
    }
}

static void ComputeChrome() {
    int m = S(16), tabH = S(46), tw = S(124);
    for (int i = 0; i < PG_COUNT; i++) {
        int x = m + i * (tw + S(6));
        g_rcTab[i] = { x, S(8), x + tw, tabH };
    }
    if (g_rcTab[g_page].right > g_rcTab[g_page].left) {
        g_tabTargetX = (float)g_rcTab[g_page].left;
        g_tabTargetW = (float)(g_rcTab[g_page].right - g_rcTab[g_page].left);
        if (g_tabCurX < 0 || !g_animatingTab) {
            g_tabCurX = g_tabTargetX;
            g_tabCurW = g_tabTargetW;
        }
    }
}

static bool OverChrome(POINT pt) {
    for (int i = 0; i < PG_COUNT; i++) if (PtInRect(&g_rcTab[i], pt)) return true;
    if (g_page == PG_SETTINGS) for (int i = 0; i < 2; i++) if (PtInRect(&g_rcSub[i], pt)) return true;
    return false;
}

// Tab bar (pages) + sub-header bar (page title, clearly separated from main content)
static void PaintHeader(HDC mem, int W) {
    int tabH = S(46), navH = S(38);
    RECT bar = { 0, 0, W, tabH };
    FillRc(mem, bar, P.tabBar);

    if (g_tabCurX < 0 && g_rcTab[g_page].right > g_rcTab[g_page].left) {
        g_tabCurX = (float)g_rcTab[g_page].left;
        g_tabCurW = (float)(g_rcTab[g_page].right - g_rcTab[g_page].left);
        g_tabTargetX = g_tabCurX;
        g_tabTargetW = g_tabCurW;
    }

    int ax = -1, aw = 0;
    if (g_tabCurX >= 0) {
        ax = (int)(g_tabCurX + 0.5f);
        aw = (int)(g_tabCurW + 0.5f);
    }

    // 1. Bottom border along tab strip, split around the active tab (no bottom line cutting through active tab)
    if (ax > 0) {
        RECT lLine = { 0, tabH - 1, ax, tabH };
        FillRc(mem, lLine, P.border);
    }
    if (ax >= 0 && (ax + aw) < W) {
        RECT rLine = { ax + aw, tabH - 1, W, tabH };
        FillRc(mem, rLine, P.border);
    } else if (ax < 0) {
        RECT fullLine = { 0, tabH - 1, W, tabH };
        FillRc(mem, fullLine, P.border);
    }

    // 2. Inactive tabs: clean flat titles, soft subtle hover pill (no corner box drawings between tabs)
    for (int i = 0; i < PG_COUNT; i++) {
        if (i == g_page) continue;
        if (i == g_tabHover) {
            RECT t = g_rcTab[i];
            RECT hovR = { t.left + S(4), t.top + S(6), t.right - S(4), tabH - S(4) };
            COLORREF hovBg = (g_theme == Theme::Light) ? RGB(232, 238, 246) :
                             (g_theme == Theme::Redstone) ? RGB(32, 18, 24) :
                             (g_theme == Theme::Minecraft) ? RGB(28, 32, 36) : RGB(20, 27, 39);
            FillRound(mem, hovR, hovBg, hovBg, S(6));
        }
    }

    // 3. Connected active tab: rounded top corners, open bottom merging directly into the subBar
    if (ax >= 0 && aw > 0) {
        int tabTop = S(8);
        int rad = S(8);

        // Fill active tab surface with P.subBar so it seamlessly connects down to the sub-header
        RECT at = { ax, tabTop, ax + aw, tabH + rad };
        FillRound(mem, at, P.subBar, P.subBar, rad);
        RECT sq = { ax, tabTop + rad, ax + aw, tabH };
        FillRc(mem, sq, P.subBar);

        // Accent outline on top-left, top, and top-right (bottom is open into the sub-header)
        HPEN pAccent = CreatePen(PS_SOLID, 1, P.accent);
        HGDIOBJ op = SelectObject(mem, pAccent);
        HGDIOBJ ob = SelectObject(mem, GetStockObject(NULL_BRUSH));

        // Drawing RoundRect with bottom below tabH ensures bottom border is hidden under subBar
        RoundRect(mem, ax, tabTop, ax + aw, tabH + rad + 2, rad * 2, rad * 2);

        SelectObject(mem, op);
        SelectObject(mem, ob);
        DeleteObject(pAccent);
    }

    // 4. Tab text labels
    for (int i = 0; i < PG_COUNT; i++) {
        RECT t = g_rcTab[i];
        COLORREF txtCol;
        if (i == g_page) {
            txtCol = (g_theme != Theme::Light) ? RGB(255, 255, 255) : P.accent;
        } else if (i == g_tabHover) {
            txtCol = P.text;
        } else {
            txtCol = P.muted;
        }
        DrawTxt(mem, PageName(i), t, g_fSemi, txtCol, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 5. Sub-header bar (seamlessly connects with active tab)
    RECT nav = { 0, tabH, W, tabH + navH };
    FillRc(mem, nav, P.subBar);
    RECT ln = { 0, tabH + navH, W, tabH + navH + 1 };
    FillRc(mem, ln, P.border);

    std::wstring up = PageName(g_page);
    for (wchar_t& c : up) c = (wchar_t)towupper(c);
    RECT title = { S(40), tabH, W - S(40), tabH + navH };
    DrawTxt(mem, up, title, g_fCap, P.text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static COLORREF ContrastText(COLORREF bg) {
    int lum = (GetRValue(bg) * 299 + GetGValue(bg) * 587 + GetBValue(bg) * 114) / 1000;
    return lum >= 150 ? RGB(15, 23, 42) : RGB(255, 255, 255);
}

// Card pages (Properties & Settings): optional sub-tab pills + cards + fields + descriptions
static void PaintCardPage(HDC mem) {
    int p = S(16);
    if (g_page == PG_SETTINGS) {
        static const wchar_t* subTr[2] = { L"GENEL", L"SUNUCU" };
        static const wchar_t* subEn[2] = { L"GENERAL", L"SERVER" };
        for (int i = 0; i < 2; i++) {
            bool act = (i == g_subTab);
            COLORREF bg = act ? P.accent : ((i == g_subTabHover) ? P.field : P.card);
            COLORREF bdr = act ? P.accent : ((i == g_subTabHover) ? P.accent : P.border);
            COLORREF txt = act ? ContrastText(bg) : ((i == g_subTabHover) ? P.text : P.muted);
            FillRound(mem, g_rcSub[i], bg, bdr, S(30));
            DrawTxt(mem, g_lang == Lang::TR ? subTr[i] : subEn[i], g_rcSub[i], g_fCap,
                    txt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
    for (const TxtBlock& c : g_scards) {
        FillRound(mem, c.rc, P.card, P.border, S(14));
        RECT r = { c.rc.left + p, c.rc.top + S(14), c.rc.right - p, c.rc.top + S(34) };
        DrawTxt(mem, CardTitle(c.id), r, g_fCap, P.accent, DT_LEFT | DT_SINGLELINE);
    }
    if (g_page == PG_ACCESS) {
        FillRound(mem, g_rcWlBg, P.logBg, P.border, S(10));
        FillRound(mem, g_rcOpBg, P.logBg, P.border, S(10));
        if (g_hListWl && SendMessageW(g_hListWl, LB_GETCOUNT, 0, 0) == 0)
            DrawTxt(mem, Tx(L"Whitelist boş", L"Whitelist is empty"), g_rcWlBg, g_fCap, P.muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (g_hListOp && SendMessageW(g_hListOp, LB_GETCOUNT, 0, 0) == 0)
            DrawTxt(mem, Tx(L"Kayıtlı OP oyuncu yok", L"No operators registered"), g_rcOpBg, g_fCap, P.muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    if (g_page == PG_BACKUPS) {
        FillRound(mem, g_rcBkBg, P.logBg, P.border, S(10));
        if (g_hListBk && SendMessageW(g_hListBk, LB_GETCOUNT, 0, 0) == 0)
            DrawTxt(mem, Tx(L"Henüz alınmış yedek yok", L"No backups found yet"), g_rcBkBg, g_fCap, P.muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    if (g_page == PG_PROFILES) {
        FillRound(mem, g_rcProfBg, P.logBg, P.border, S(10));
        if (g_hListProf && SendMessageW(g_hListProf, LB_GETCOUNT, 0, 0) == 0)
            DrawTxt(mem, Tx(L"Kayıtlı profil yok", L"No saved profiles"), g_rcProfBg, g_fCap, P.muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    if (g_page == PG_PLUGINS) {
        FillRound(mem, g_rcPlugBg, P.logBg, P.border, S(10));
        FillRound(mem, g_rcModBg, P.logBg, P.border, S(10));
        if (g_hListPlugins && SendMessageW(g_hListPlugins, LB_GETCOUNT, 0, 0) == 0)
            DrawTxt(mem, Tx(L"plugins\\ klasöründe eklenti yok", L"No plugins found in plugins\\"), g_rcPlugBg, g_fCap, P.muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (g_hListMods && SendMessageW(g_hListMods, LB_GETCOUNT, 0, 0) == 0)
            DrawTxt(mem, Tx(L"mods\\ klasöründe mod yok", L"No mods found in mods\\"), g_rcModBg, g_fCap, P.muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    HWND focus = GetFocus();
    for (int i = F_JVM; i < F_COUNT; i++) {
        if (!g_f[i].h || !IsWindowVisible(g_f[i].h)) continue;
        FillRound(mem, g_f[i].rc, P.field, focus == g_f[i].h ? P.accent : P.border, S(10));
        if (g_f[i].caption) {
            RECT r = { g_f[i].rc.left + S(2), g_f[i].rc.top - S(19), g_f[i].rc.right, g_f[i].rc.top - S(3) };
            DrawTxt(mem, g_f[i].caption, r, g_fCap, P.muted, DT_LEFT | DT_SINGLELINE);
        }
    }
    for (const TxtBlock& t : g_txts) {
        HGDIOBJ of = SelectObject(mem, g_fCap);
        SetTextColor(mem, P.muted); SetBkMode(mem, TRANSPARENT);
        RECT r = t.rc;
        DrawTextW(mem, DescText(t.id), -1, &r, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
        SelectObject(mem, of);
    }
}

static void PaintMain(HWND hwnd) {
    PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
    RECT rc; GetClientRect(hwnd, &rc);
    int W = rc.right, H = rc.bottom;
    if (W <= 0 || H <= 0) { EndPaint(hwnd, &ps); return; }

    HDC mem = CreateCompatibleDC(dc);
    if (!mem) { EndPaint(hwnd, &ps); return; }
    HBITMAP bm = CreateCompatibleBitmap(dc, W, H);
    if (!bm) { DeleteDC(mem); EndPaint(hwnd, &ps); return; }
    HGDIOBJ ob = SelectObject(mem, bm);
    FillRect(mem, &rc, g_bBg);

    PaintHeader(mem, W);
    if (g_page != PG_PANEL) {
        PaintCardPage(mem);
        BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
               mem, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
        SelectObject(mem, ob); DeleteObject(bm); DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return;
    }

    int rad = S(14), p = S(16);
    const RECT* cards[3] = { &g_rcSet, &g_rcPl, &g_rcCon };
    for (const RECT* c : cards) FillRound(mem, *c, P.card, P.border, rad);

    auto title = [&](const RECT& c, const std::wstring& t) {
        RECT r = { c.left + p, c.top + S(14), c.right - p, c.top + S(34) };
        DrawTxt(mem, t, r, g_fCap, P.accent, DT_LEFT | DT_SINGLELINE);
    };
    title(g_rcSet, L().cardSettings);
    title(g_rcPl, std::wstring(L().cardPlayers) + L"  •  " + std::to_wstring(PlayerCount()));
    title(g_rcCon, L().cardConsole);

    // Input field backgrounds + captions
    HWND focus = GetFocus();
    for (int i = 0; i < F_JVM; i++) {
        if (!IsWindowVisible(g_f[i].h)) continue;
        bool foc = (focus == g_f[i].h);
        FillRound(mem, g_f[i].rc, P.field, foc ? P.accent : P.border, S(10));
        if (g_f[i].caption) {
            RECT r = { g_f[i].rc.left + S(2), g_f[i].rc.top - S(19), g_f[i].rc.right, g_f[i].rc.top - S(3) };
            DrawTxt(mem, g_f[i].caption, r, g_fCap, P.muted, DT_LEFT | DT_SINGLELINE);
        }
    }
    FillRound(mem, g_rcListBg, P.logBg, P.border, S(10));
    FillRound(mem, g_rcLogBg, P.logBg, P.border, S(10));

    if (PlayerCount() == 0) {
        RECT r = g_rcListBg;
        DrawTxt(mem, g_state == SrvState::Running ? L().noPlayersOnline : L().serverOffline, r, g_fCap, P.muted,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Stat tiles
    const wchar_t* caps[5] = { L().stStatus, L().stUptime, L().stMemory, L().stPlayers, L().stWorldTime };
    std::wstring vals[5] = { StateText(), g_upText, g_ramText, std::to_wstring(PlayerCount()), g_igTime };
    for (int i = 0; i < 5; i++) {
        FillRound(mem, g_rcTile[i], P.card, P.border, S(12));
        RECT r1 = { g_rcTile[i].left + S(14), g_rcTile[i].top + S(12), g_rcTile[i].right - S(8), g_rcTile[i].top + S(28) };
        RECT r2 = { g_rcTile[i].left + S(14), g_rcTile[i].top + S(32), g_rcTile[i].right - S(8), g_rcTile[i].bottom - S(8) };
        DrawTxt(mem, caps[i], r1, g_fCap, P.muted, DT_LEFT | DT_SINGLELINE);
        DrawTxt(mem, vals[i], r2, g_fVal, i == 0 ? StateColor() : P.text, DT_LEFT | DT_SINGLELINE);
    }

    BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
           mem, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
    SelectObject(mem, ob); DeleteObject(bm); DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

static void DrawButton(const DRAWITEMSTRUCT* di) {
    HDC dc = di->hDC; RECT r = di->rcItem;
    int id = (int)di->CtlID;
    bool dis = (di->itemState & ODS_DISABLED) != 0;
    bool press = (di->itemState & ODS_SELECTED) != 0;
    bool hover = (g_hHover == di->hwndItem);
    bool focus = (di->itemState & ODS_FOCUS) && !(di->itemState & ODS_NOFOCUSRECT);

    // Checkbox-style toggles on the settings / properties pages
    if (id == ID_CHK_AUTOSTART || id == ID_MENU_AUTORESTART ||
        id == ID_CHK_ONLINE || id == ID_CHK_PVP || id == ID_CHK_WHITELIST || id == ID_CHK_FLIGHT || id == ID_CHK_CMDBLOCK ||
        id == ID_CHK_SCHED_BK || id == ID_CHK_SCHED_RESTART) {
        bool on = false;
        switch (id) {
            case ID_CHK_AUTOSTART:     on = g_autoStartCache; break;
            case ID_MENU_AUTORESTART:  on = g_autoRestart; break;
            case ID_CHK_ONLINE:        on = g_propOnline; break;
            case ID_CHK_PVP:           on = g_propPvp; break;
            case ID_CHK_WHITELIST:     on = g_propWhitelist; break;
            case ID_CHK_FLIGHT:        on = g_propFlight; break;
            case ID_CHK_CMDBLOCK:      on = g_propCmdBlock; break;
            case ID_CHK_SCHED_BK:      on = g_schedBkEnabled; break;
            case ID_CHK_SCHED_RESTART: on = g_schedRestartEnabled; break;
        }
        FillRc(dc, r, P.card);
        int bs = S(18);
        RECT box = { r.left, (r.top + r.bottom - bs) / 2, r.left + bs, (r.top + r.bottom - bs) / 2 + bs };
        FillRound(dc, box, on ? P.accent : P.field, on ? P.accent : ((hover || focus) ? P.accent : P.border), S(6));
        if (on) {
            HPEN pen = CreatePen(PS_SOLID, std::max(2, S(2)), ContrastText(P.accent));
            HGDIOBJ op = SelectObject(dc, pen);
            MoveToEx(dc, box.left + S(4), box.top + bs / 2, NULL);
            LineTo(dc, box.left + S(7), box.bottom - S(5));
            LineTo(dc, box.right - S(4), box.top + S(5));
            SelectObject(dc, op); DeleteObject(pen);
        }
        wchar_t ctxt[128]; GetWindowTextW(di->hwndItem, ctxt, 128);
        RECT tr = { box.right + S(10), r.top, r.right, r.bottom };
        DrawTxt(dc, ctxt, tr, g_fUI, P.text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    // Selector buttons (language, theme, gamemode, difficulty) highlight the active choice
    bool isSel = (id == ID_MENU_LANG_TR || id == ID_MENU_LANG_EN ||
                  id == ID_MENU_THEME_DARK || id == ID_MENU_THEME_LIGHT ||
                  id == ID_MENU_THEME_REDSTONE || id == ID_MENU_THEME_MC ||
                  (id >= ID_BTN_GM_SURV && id <= ID_BTN_DF_HARD));
    bool selActive = (id == ID_MENU_LANG_TR && g_lang == Lang::TR) || (id == ID_MENU_LANG_EN && g_lang == Lang::EN) ||
                     (id == ID_MENU_THEME_DARK && g_theme == Theme::Dark) ||
                     (id == ID_MENU_THEME_LIGHT && g_theme == Theme::Light) ||
                     (id == ID_MENU_THEME_REDSTONE && g_theme == Theme::Redstone) ||
                     (id == ID_MENU_THEME_MC && g_theme == Theme::Minecraft) ||
                     (id >= ID_BTN_GM_SURV && id <= ID_BTN_GM_SPEC && (id - ID_BTN_GM_SURV) == g_propGm) ||
                     (id >= ID_BTN_DF_PEACE && id <= ID_BTN_DF_HARD && (id - ID_BTN_DF_PEACE) == g_propDf);

    COLORREF base, hov, txtCol = RGB(255, 255, 255);
    switch (id) {
        case ID_BTN_START:
        case ID_BTN_PROPSAVE:
        case ID_BTN_WL_ADD:
        case ID_BTN_OP_ADD:
        case ID_BTN_BK_CREATE:
        case ID_BTN_PROF_SAVE:
        case ID_BTN_PLUG_ADD:
        case ID_BTN_MOD_ADD:
        case ID_BTN_SCHED_BK_SAVE:
        case ID_BTN_SCHED_RESTART_SAVE: base = RGB(34, 150, 90);  hov = RGB(46, 176, 108); break;
        case ID_BTN_STOP:
        case ID_BTN_BAN:
        case ID_BTN_WL_REM:
        case ID_BTN_OP_REM:
        case ID_BTN_BK_DEL:
        case ID_BTN_PROF_DEL:
        case ID_BTN_PLUG_DEL:
        case ID_BTN_MOD_DEL:  base = RGB(190, 62, 74);  hov = RGB(216, 78, 92);  break;
        case ID_BTN_KICK:
        case ID_BTN_BK_RESTORE: base = RGB(196, 140, 34); hov = RGB(220, 160, 48); break;
        case ID_BTN_SEND:
        case ID_BTN_PROF_LOAD:
        case ID_BTN_PLUG_TOGGLE:
        case ID_BTN_MOD_TOGGLE: base = RGB(46, 110, 200); hov = RGB(64, 130, 224); break;
        default:
            base = P.btnDef; hov = P.btnDefHov;
            txtCol = (isSel && selActive) ? P.accent : P.text;
            break;
    }
    COLORREF fill = dis ? P.btnDis : press ? RGB(GetRValue(base) * 4 / 5, GetGValue(base) * 4 / 5, GetBValue(base) * 4 / 5)
                                           : hover ? hov : base;
    HBRUSH bg = CreateSolidBrush(P.card); FillRect(dc, &r, bg); DeleteObject(bg);
    RECT rr = r; InflateRect(&rr, -1, -1);
    FillRound(dc, rr, fill, focus && !dis ? P.accent : ((isSel && selActive) ? P.accent : (id == ID_BTN_BROWSE || id == ID_BTN_UNBAN || isSel ? P.border : fill)), S(10));

    wchar_t txt[128]; GetWindowTextW(di->hwndItem, txt, 128);
    if (press && !dis) OffsetRect(&r, 0, 1);
    DrawTxt(dc, txt, r, g_fSemi, dis ? P.btnDisTxt : txtCol, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void DrawPlayer(const DRAWITEMSTRUCT* di) {
    if ((int)di->itemID < 0) return;
    HDC dc = di->hDC; RECT r = di->rcItem;
    HBRUSH bg = CreateSolidBrush(P.logBg); FillRect(dc, &r, bg); DeleteObject(bg);
    std::wstring name = ListTextFrom(di->hwndItem, (int)di->itemID);
    bool sel = (di->itemState & ODS_SELECTED) != 0;
    RECT row = r; InflateRect(&row, -S(4), -S(2));
    if (sel) FillRound(dc, row, P.sel, P.accent, S(8));

    // Backup and profile listbox rows do not draw a player skinhead
    if (di->hwndItem == g_hListBk || di->hwndItem == g_hListProf) {
        RECT tr = { row.left + S(10), row.top, row.right - S(8), row.bottom };
        DrawTxt(dc, name, tr, g_fUI, P.text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    if (di->hwndItem == g_hListPlugins || di->hwndItem == g_hListMods) {
        bool isPlug = (di->hwndItem == g_hListPlugins);
        const auto& list = isPlug ? g_pluginsList : g_modsList;
        if ((size_t)di->itemID < list.size()) {
            const ModItem& item = list[di->itemID];
            int pillW = S(58), pillH = S(20);
            int pillX = row.left + S(8);
            int pillY = row.top + (row.bottom - row.top - pillH) / 2;
            RECT pill = { pillX, pillY, pillX + pillW, pillY + pillH };

            COLORREF pillBg = item.enabled ? RGB(34, 150, 90) : RGB(90, 95, 105);
            FillRound(dc, pill, pillBg, pillBg, S(6));
            DrawTxt(dc, item.enabled ? Tx(L"AKTİF", L"ACTIVE") : Tx(L"PASİF", L"OFF"), pill, g_fCap, RGB(255, 255, 255),
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            int nameX = pillX + pillW + S(10);
            int sizeW = S(65);
            RECT tr = { nameX, row.top, row.right - sizeW - S(6), row.bottom };
            DrawTxt(dc, item.displayName, tr, g_fUI, item.enabled ? P.text : P.muted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            RECT sr = { row.right - sizeW, row.top, row.right - S(8), row.bottom };
            DrawTxt(dc, item.sizeStr, sr, g_fCap, P.muted, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        }
        return;
    }

    int d = S(26);
    int ax = row.left + S(8);
    int ay = row.top + (row.bottom - row.top - d) / 2;

    // Draw player's Minecraft Skinhead (32x32 bitmap scaled crisply to d x d)
    HBITMAP hSkin = GetPlayerSkinBitmap(name);
    if (hSkin) {
        HDC mdc = CreateCompatibleDC(dc);
        if (mdc) {
            HGDIOBJ old = SelectObject(mdc, hSkin);
            int oldMode = SetStretchBltMode(dc, COLORONCOLOR);
            StretchBlt(dc, ax, ay, d, d, mdc, 0, 0, 32, 32, SRCCOPY);
            SetStretchBltMode(dc, oldMode);
            SelectObject(mdc, old);
            DeleteDC(mdc);
        }

        // Subtle crisp border around the Minecraft head
        RECT br = { ax - 1, ay - 1, ax + d + 1, ay + d + 1 };
        HBRUSH frameBr = CreateSolidBrush(P.border);
        if (frameBr) {
            FrameRect(dc, &br, frameBr);
            DeleteObject(frameBr);
        }
    }

    RECT tr = { ax + d + S(10), row.top, row.right - S(8), row.bottom };
    DrawTxt(dc, name, tr, g_fUI, P.text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

// ----------------------------------------------------------------------------
//  Layout
// ----------------------------------------------------------------------------
static void LayoutSettings(int W, int m, int g, int p, int fh);
static void LayoutProps(int W, int m, int g, int p, int fh);
static void LayoutAccess(int W, int H, int m, int g, int p, int fh);
static void LayoutBackups(int W, int H, int m, int g, int p, int fh);
static void LayoutPlugins(int W, int H, int m, int g, int p, int fh);
static void LayoutProfiles(int W, int H, int m, int g, int p, int fh);

static void Layout(int W, int H) {
    if (!g_hCmd) return;
    ComputeChrome();
    int m = S(16), g = S(12), p = S(16), lw = S(330), fh = S(34), oy = HeaderH();
    HDWP dw = BeginDeferWindowPos(26);
    auto put = [&](HWND h, int x, int y, int w, int hh) {
        if (dw) dw = DeferWindowPos(dw, h, NULL, x, y, std::max(w, 1), std::max(hh, 1), SWP_NOZORDER | SWP_NOACTIVATE);
    };
    auto fieldRect = [&](int i, int x, int y, int w) {
        g_f[i].rc = { x, y, x + w, y + fh };
        put(g_f[i].h, x + S(10), y + (fh - g_textH) / 2, w - S(20), g_textH);
    };

    // --- Settings Card
    g_rcSet = { m, oy + m, m + lw, oy + m + S(302) };
    int x = g_rcSet.left + p, iw = lw - 2 * p, y0 = g_rcSet.top;
    fieldRect(F_JAR, x, y0 + S(64), iw - S(78));
    put(g_hBtnBrowse, x + iw - S(70), y0 + S(64), S(70), fh);
    int half = (iw - S(12)) / 2;
    fieldRect(F_IP, x, y0 + S(130), half);
    fieldRect(F_PORT, x + half + S(12), y0 + S(130), iw - half - S(12));
    fieldRect(F_RAM, x, y0 + S(196), iw);
    int bw = (iw - S(10)) / 2;
    put(g_hBtnStart, x, y0 + S(246), bw, S(40));
    put(g_hBtnStop, x + bw + S(10), y0 + S(246), iw - bw - S(10), S(40));

    // --- Players Card
    g_rcPl = { m, g_rcSet.bottom + g, m + lw, std::max((int)(g_rcSet.bottom + g + S(200)), H - m) };
    int unbanTop = g_rcPl.bottom - p - fh;
    int kickTop = unbanTop - S(26) - fh;
    g_rcListBg = { x, g_rcPl.top + S(40), x + iw, kickTop - S(10) };
    RECT li = g_rcListBg; InflateRect(&li, -S(4), -S(4));
    put(g_hList, li.left, li.top, li.right - li.left, li.bottom - li.top);
    put(g_hBtnKick, x, kickTop, bw, fh);
    put(g_hBtnBan, x + bw + S(10), kickTop, iw - bw - S(10), fh);
    fieldRect(F_UNBAN, x, unbanTop, iw - S(104));
    put(g_hBtnUnban, x + iw - S(94), unbanTop, S(94), fh);

    // --- Top-Right: Stat Tiles
    int rx = m + lw + g, tg = S(10), th = S(72);
    int statsRight = W - m;
    int rw = statsRight - rx;
    int tw = (rw - 4 * tg) / 5;
    for (int i = 0; i < 5; i++) {
        int tx = rx + i * (tw + tg);
        g_rcTile[i] = { tx, oy + m, (i == 4) ? statsRight : tx + tw, oy + m + th };
    }
    g_rcStats = { rx, oy + m, statsRight, oy + m + th };

    // --- Console Card
    g_rcCon = { rx, oy + m + th + g, W - m, H - m };
    int cx = g_rcCon.left + p, ciw = g_rcCon.right - g_rcCon.left - 2 * p;
    int cmdTop = g_rcCon.bottom - p - fh;
    g_rcLogBg = { cx, g_rcCon.top + S(40), cx + ciw, cmdTop - S(10) };
    put(g_hLog, g_rcLogBg.left + S(10), g_rcLogBg.top + S(8), ciw - S(20), g_rcLogBg.bottom - g_rcLogBg.top - S(16));
    fieldRect(F_CMD, cx, cmdTop, ciw - S(90));
    put(g_hBtnSend, cx + ciw - S(80), cmdTop, S(80), fh);

    if (dw) EndDeferWindowPos(dw);
    if (g_hLog) {
        SendMessageW(g_hLog, EM_SETTARGETDEVICE, 0, 0);
        LockConsoleHorizontalScroll();
    }
    if (g_page == PG_PROPS) LayoutProps(W, m, g, p, fh);
    else if (g_page == PG_ACCESS) LayoutAccess(W, H, m, g, p, fh);
    else if (g_page == PG_BACKUPS) LayoutBackups(W, H, m, g, p, fh);
    else if (g_page == PG_PLUGINS) LayoutPlugins(W, H, m, g, p, fh);
    else if (g_page == PG_PROFILES) LayoutProfiles(W, H, m, g, p, fh);
    else if (g_page == PG_SETTINGS) LayoutSettings(W, m, g, p, fh);
    InvalidateRect(g_hwnd, NULL, FALSE);
}

// Plugins & Mods page: left card for plugins (plugins\), right card for mods (mods\)
static void LayoutPlugins(int W, int H, int m, int g, int p, int fh) {
    g_scards.clear(); g_txts.clear();
    int oy = HeaderH(), top = oy + m;
    int colW = (W - 2 * m - g) / 2, lx = m, rxc = m + colW + g;
    int iw = colW - 2 * p;
    int cardH = std::max(S(340), H - top - m);

    auto card = [&](int x, int y, int h, int titleId) -> RECT {
        RECT r = { x, y, x + colW, y + h };
        g_scards.push_back({ r, titleId });
        return r;
    };
    auto mv = [&](HWND h, int x, int y, int w, int hh) { MoveWindow(h, x, y, std::max(w, 1), std::max(hh, 1), TRUE); };
    auto desc = [&](int x, int y, int w, int h, int id) { RECT r = { x, y, x + w, y + h }; g_txts.push_back({ r, id }); };

    // Left Card: Plugins
    RECT cPlug = card(lx, top, cardH, 17);
    desc(cPlug.left + p, cPlug.top + S(42), iw, S(32), 10);
    int r2Y = cPlug.bottom - p - fh;
    int r1Y = r2Y - S(8) - fh;
    g_rcPlugBg = { cPlug.left + p, cPlug.top + S(78), cPlug.left + p + iw, r1Y - S(10) };
    RECT liP = g_rcPlugBg; InflateRect(&liP, -S(4), -S(4));
    mv(g_hListPlugins, liP.left, liP.top, liP.right - liP.left, liP.bottom - liP.top);
    mv(g_hBtnPlugToggle, cPlug.left + p, r1Y, iw, fh);

    int b4w = (iw - 3 * S(6)) / 4;
    mv(g_hBtnPlugAdd,    cPlug.left + p,                    r2Y, b4w, fh);
    mv(g_hBtnPlugDel,    cPlug.left + p + b4w + S(6),       r2Y, b4w, fh);
    mv(g_hBtnPlugOpen,   cPlug.left + p + 2 * (b4w + S(6)), r2Y, b4w, fh);
    mv(g_hBtnPlugReload, cPlug.left + p + 3 * (b4w + S(6)), r2Y, iw - 3 * (b4w + S(6)), fh);

    // Right Card: Mods
    RECT cMod = card(rxc, top, cardH, 18);
    desc(cMod.left + p, cMod.top + S(42), iw, S(32), 11);
    g_rcModBg = { cMod.left + p, cMod.top + S(78), cMod.left + p + iw, r1Y - S(10) };
    RECT liM = g_rcModBg; InflateRect(&liM, -S(4), -S(4));
    mv(g_hListMods, liM.left, liM.top, liM.right - liM.left, liM.bottom - liM.top);
    mv(g_hBtnModToggle, cMod.left + p, r1Y, iw, fh);

    mv(g_hBtnModAdd,    cMod.left + p,                    r2Y, b4w, fh);
    mv(g_hBtnModDel,    cMod.left + p + b4w + S(6),       r2Y, b4w, fh);
    mv(g_hBtnModOpen,   cMod.left + p + 2 * (b4w + S(6)), r2Y, b4w, fh);
    mv(g_hBtnModReload, cMod.left + p + 3 * (b4w + S(6)), r2Y, iw - 3 * (b4w + S(6)), fh);
}

// Profiles page: left card for saving current settings as a profile, right card for listing/loading/deleting profiles
static void LayoutProfiles(int W, int H, int m, int g, int p, int fh) {
    g_scards.clear(); g_txts.clear();
    int oy = HeaderH(), top = oy + m;
    int colW = (W - 2 * m - g) / 2, lx = m, rxc = m + colW + g;
    int iw = colW - 2 * p;
    int cardH = std::max(S(320), H - top - m);

    auto card = [&](int x, int y, int h, int titleId) -> RECT {
        RECT r = { x, y, x + colW, y + h };
        g_scards.push_back({ r, titleId });
        return r;
    };
    auto mv = [&](HWND h, int x, int y, int w, int hh) { MoveWindow(h, x, y, std::max(w, 1), std::max(hh, 1), TRUE); };
    auto desc = [&](int x, int y, int w, int h, int id) { RECT r = { x, y, x + w, y + h }; g_txts.push_back({ r, id }); };
    auto putField = [&](int fi, int x, int y, int w) {
        g_f[fi].rc = { x, y, x + w, y + fh };
        mv(g_f[fi].h, x + S(10), y + (fh - g_textH) / 2, w - S(20), g_textH);
    };

    // Left Card: Save New Profile
    RECT cSave = card(lx, top, S(230), 13);
    desc(cSave.left + p, cSave.top + S(42), iw, S(48), 7);
    putField(F_PROF_NAME, cSave.left + p, cSave.top + S(110), iw);
    mv(g_hBtnProfSave, cSave.left + p, cSave.top + S(160), iw, S(40));

    // Right Card: Saved Server Profiles List
    RECT cList = card(rxc, top, cardH, 14);
    int btnRowY = cList.bottom - p - fh;
    g_rcProfBg = { cList.left + p, cList.top + S(42), cList.left + p + iw, btnRowY - S(12) };
    RECT li = g_rcProfBg; InflateRect(&li, -S(4), -S(4));
    mv(g_hListProf, li.left, li.top, li.right - li.left, li.bottom - li.top);
    int bw = (iw - S(10)) / 2;
    mv(g_hBtnProfLoad, cList.left + p,             btnRowY, bw, fh);
    mv(g_hBtnProfDel,  cList.left + p + bw + S(10), btnRowY, iw - bw - S(10), fh);
}

// Backups page: left card for creating backups, right card for listing/restoring/deleting
static void LayoutBackups(int W, int H, int m, int g, int p, int fh) {
    g_scards.clear(); g_txts.clear();
    int oy = HeaderH(), top = oy + m;
    int colW = (W - 2 * m - g) / 2, lx = m, rxc = m + colW + g;
    int iw = colW - 2 * p;
    int cardH = std::max(S(320), H - top - m);

    auto card = [&](int x, int y, int h, int titleId) -> RECT {
        RECT r = { x, y, x + colW, y + h };
        g_scards.push_back({ r, titleId });
        return r;
    };
    auto mv = [&](HWND h, int x, int y, int w, int hh) { MoveWindow(h, x, y, std::max(w, 1), std::max(hh, 1), TRUE); };
    auto desc = [&](int x, int y, int w, int h, int id) { RECT r = { x, y, x + w, y + h }; g_txts.push_back({ r, id }); };
    auto putField = [&](int fi, int x, int y, int w) {
        g_f[fi].rc = { x, y, x + w, y + fh };
        mv(g_f[fi].h, x + S(10), y + (fh - g_textH) / 2, w - S(20), g_textH);
    };

    // Left Card: Create Backup
    RECT cCreate = card(lx, top, S(260), 11);
    desc(cCreate.left + p, cCreate.top + S(42), iw, S(48), 6);
    putField(F_BK_TAG, cCreate.left + p, cCreate.top + S(116), iw);
    mv(g_hBtnBkCreate, cCreate.left + p, cCreate.top + S(162), iw, S(40));
    mv(g_hBtnBkOpen,   cCreate.left + p, cCreate.top + S(210), iw, fh);

    // Right Card: Saved Backups List
    RECT cList = card(rxc, top, cardH, 12);
    int btnRowY = cList.bottom - p - fh;
    g_rcBkBg = { cList.left + p, cList.top + S(42), cList.left + p + iw, btnRowY - S(12) };
    RECT li = g_rcBkBg; InflateRect(&li, -S(4), -S(4));
    mv(g_hListBk, li.left, li.top, li.right - li.left, li.bottom - li.top);
    int w3 = (iw - 2 * S(8)) / 3;
    mv(g_hBtnBkRestore, cList.left + p,                   btnRowY, w3, fh);
    mv(g_hBtnBkDel,     cList.left + p + w3 + S(8),       btnRowY, w3, fh);
    mv(g_hBtnBkReload,  cList.left + p + 2 * (w3 + S(8)), btnRowY, iw - 2 * (w3 + S(8)), fh);
}

// Access page: two-column card layout for Whitelist & Operators
static void LayoutAccess(int W, int H, int m, int g, int p, int fh) {
    g_scards.clear(); g_txts.clear();
    int oy = HeaderH(), top = oy + m;
    int colW = (W - 2 * m - g) / 2, lx = m, rxc = m + colW + g;
    int iw = colW - 2 * p;
    int cardH = std::max(S(320), H - top - m);

    auto card = [&](int x, int y, int h, int titleId) -> RECT {
        RECT r = { x, y, x + colW, y + h };
        g_scards.push_back({ r, titleId });
        return r;
    };
    auto mv = [&](HWND h, int x, int y, int w, int hh) { MoveWindow(h, x, y, std::max(w, 1), std::max(hh, 1), TRUE); };
    auto putField = [&](int fi, int x, int y, int w) {
        g_f[fi].rc = { x, y, x + w, y + fh };
        mv(g_f[fi].h, x + S(10), y + (fh - g_textH) / 2, w - S(20), g_textH);
    };

    // Left Card: Whitelist
    RECT cWl = card(lx, top, cardH, 9);
    int btnRowY = cWl.bottom - p - fh;
    int editRowY = btnRowY - S(12) - fh;
    g_rcWlBg = { cWl.left + p, cWl.top + S(42), cWl.left + p + iw, editRowY - S(26) };
    RECT liW = g_rcWlBg; InflateRect(&liW, -S(4), -S(4));
    mv(g_hListWl, liW.left, liW.top, liW.right - liW.left, liW.bottom - liW.top);
    putField(F_WL, cWl.left + p, editRowY, iw - S(130));
    mv(g_hBtnWlAdd, cWl.left + p + iw - S(122), editRowY, S(122), fh);
    int bw = (iw - S(10)) / 2;
    mv(g_hBtnWlRem,    cWl.left + p,             btnRowY, bw, fh);
    mv(g_hBtnWlReload, cWl.left + p + bw + S(10), btnRowY, iw - bw - S(10), fh);

    // Right Card: Operators (OP)
    RECT cOp = card(rxc, top, cardH, 10);
    g_rcOpBg = { cOp.left + p, cOp.top + S(42), cOp.left + p + iw, editRowY - S(26) };
    RECT liO = g_rcOpBg; InflateRect(&liO, -S(4), -S(4));
    mv(g_hListOp, liO.left, liO.top, liO.right - liO.left, liO.bottom - liO.top);
    putField(F_OP, cOp.left + p, editRowY, iw - S(130));
    mv(g_hBtnOpAdd, cOp.left + p + iw - S(122), editRowY, S(122), fh);
    mv(g_hBtnOpRem,    cOp.left + p,             btnRowY, bw, fh);
    mv(g_hBtnOpReload, cOp.left + p + bw + S(10), btnRowY, iw - bw - S(10), fh);
}

// Properties page: two-column card layout for server.properties
static void LayoutProps(int W, int m, int g, int p, int fh) {
    g_scards.clear(); g_txts.clear();
    int oy = HeaderH(), top = oy + m;
    int colW = (W - 2 * m - g) / 2, lx = m, rxc = m + colW + g;
    int iw = colW - 2 * p;

    auto card = [&](int x, int y, int h, int titleId) -> RECT {
        RECT r = { x, y, x + colW, y + h };
        g_scards.push_back({ r, titleId });
        return r;
    };
    auto mv = [&](HWND h, int x, int y, int w, int hh) { MoveWindow(h, x, y, std::max(w, 1), std::max(hh, 1), TRUE); };
    auto desc = [&](int x, int y, int w, int h, int id) { RECT r = { x, y, x + w, y + h }; g_txts.push_back({ r, id }); };
    auto putField = [&](int fi, int x, int y, int w) {
        g_f[fi].rc = { x, y, x + w, y + fh };
        mv(g_f[fi].h, x + S(10), y + (fh - g_textH) / 2, w - S(20), g_textH);
    };

    // Left Column Card 1: Server Info (MOTD, Max Players, View Distance, Spawn Protection, Save/Reload)
    RECT cInfo = card(lx, top, S(268), 6);
    putField(F_MOTD, cInfo.left + p, cInfo.top + S(62), iw);
    int w3 = (iw - 2 * S(10)) / 3;
    int yRow2 = cInfo.top + S(128);
    putField(F_MAXPL,     cInfo.left + p,                    yRow2, w3);
    putField(F_VIEWDIST,  cInfo.left + p + w3 + S(10),       yRow2, w3);
    putField(F_SPAWNPROT, cInfo.left + p + 2 * (w3 + S(10)), yRow2, iw - 2 * (w3 + S(10)));
    int bw = (iw - S(10)) / 2;
    mv(g_hBtnPropSave,   cInfo.left + p,             cInfo.top + S(182), bw, S(38));
    mv(g_hBtnPropReload, cInfo.left + p + bw + S(10), cInfo.top + S(182), iw - bw - S(10), S(38));
    desc(cInfo.left + p, cInfo.top + S(228), iw, S(34), 5);

    // Left Column Card 2: Gamemode & Difficulty
    RECT cGm = card(lx, cInfo.bottom + g, S(194), 7);
    desc(cGm.left + p, cGm.top + S(42), iw, S(18), 3);
    int gbw = (iw - 3 * S(8)) / 4;
    for (int i = 0; i < 4; i++) {
        int bx = cGm.left + p + i * (gbw + S(8));
        int w  = (i == 3) ? (cGm.left + p + iw - bx) : gbw;
        mv(g_hBtnGm[i], bx, cGm.top + S(62), w, fh);
    }
    desc(cGm.left + p, cGm.top + S(112), iw, S(18), 4);
    for (int i = 0; i < 4; i++) {
        int bx = cGm.left + p + i * (gbw + S(8));
        int w  = (i == 3) ? (cGm.left + p + iw - bx) : gbw;
        mv(g_hBtnDf[i], bx, cGm.top + S(132), w, fh);
    }

    // Right Column Card: Rules & Security
    RECT cRules = card(rxc, top, S(246), 8);
    HWND chks[5] = { g_hChkOnline, g_hChkPvp, g_hChkWhitelist, g_hChkFlight, g_hChkCmdBlock };
    for (int i = 0; i < 5; i++) {
        mv(chks[i], cRules.left + p, cRules.top + S(48) + i * S(36), iw, S(26));
    }
}

// Settings page: two-column card layout (General / Server sub-tabs)
static void LayoutSettings(int W, int m, int g, int p, int fh) {
    g_scards.clear(); g_txts.clear();
    int oy = HeaderH(), subW = S(110), subH = S(30), cx = W / 2;
    g_rcSub[0] = { cx - subW - S(4), oy + m, cx - S(4), oy + m + subH };
    g_rcSub[1] = { cx + S(4), oy + m, cx + S(4) + subW, oy + m + subH };
    int top = oy + m + subH + g;
    int colW = (W - 2 * m - g) / 2, lx = m, rxc = m + colW + g;
    int iw = colW - 2 * p;

    auto card = [&](int x, int y, int h, int titleId) -> RECT {
        RECT r = { x, y, x + colW, y + h };
        g_scards.push_back({ r, titleId });
        return r;
    };
    auto mv = [&](HWND h, int x, int y, int w, int hh) { MoveWindow(h, x, y, std::max(w, 1), std::max(hh, 1), TRUE); };
    auto desc = [&](int x, int y, int w, int h, int id) { RECT r = { x, y, x + w, y + h }; g_txts.push_back({ r, id }); };

    if (g_subTab == 0) {
        RECT a = card(lx, top, S(92), 0);
        mv(g_hChkAutoStart, a.left + p, a.top + S(46), iw, S(24));
        RECT b = card(lx, a.bottom + g, S(106), 1);
        int bw = (iw - S(10)) / 2;
        mv(g_hBtnLangTR, b.left + p, b.top + S(52), bw, fh);
        mv(g_hBtnLangEN, b.left + p + bw + S(10), b.top + S(52), iw - bw - S(10), fh);
        RECT c = card(rxc, top, S(148), 2);
        int tbw = (iw - S(10)) / 2;
        int r1y = c.top + S(52);
        int r2y = r1y + fh + S(8);
        mv(g_hBtnThDark,     c.left + p,              r1y, tbw, fh);
        mv(g_hBtnThLight,    c.left + p + tbw + S(10), r1y, iw - tbw - S(10), fh);
        mv(g_hBtnThRedstone, c.left + p,              r2y, tbw, fh);
        mv(g_hBtnThMc,       c.left + p + tbw + S(10), r2y, iw - tbw - S(10), fh);
    } else {
        RECT d = card(lx, top, S(138), 3);
        mv(g_hChkAutoRestart, d.left + p, d.top + S(46), iw, S(24));
        desc(d.left + p, d.top + S(80), iw, S(48), 0);

        RECT cDaily = card(lx, d.bottom + g, S(172), 16);
        mv(g_hChkSchedRestart, cDaily.left + p, cDaily.top + S(46), iw, S(24));
        desc(cDaily.left + p, cDaily.top + S(74), iw, S(36), 9);
        int ry = cDaily.top + S(122);
        g_f[F_RESTART_TIME].rc = { cDaily.left + p, ry, cDaily.left + p + iw - S(90), ry + fh };
        mv(g_hEditRestartTime, cDaily.left + p + S(10), ry + (fh - g_textH) / 2, iw - S(110), g_textH);
        mv(g_hBtnSchedRestartSave, cDaily.left + p + iw - S(80), ry, S(80), fh);

        RECT f = card(lx, cDaily.bottom + g, S(118), 5);
        mv(g_hBtnLogs, f.left + p, f.top + S(46), iw, fh);
        desc(f.left + p, f.top + S(88), iw, S(24), 2);

        RECT e = card(rxc, top, S(190), 4);
        desc(e.left + p, e.top + S(44), iw, S(40), 1);
        int fy = e.top + S(96);
        g_f[F_JVM].rc = { e.left + p, fy, e.left + p + iw, fy + fh };
        mv(g_hJvm, e.left + p + S(10), fy + (fh - g_textH) / 2, iw - S(20), g_textH);
        mv(g_hBtnJvmSave, e.left + p, fy + fh + S(12), S(120), fh);

        RECT cAutoBk = card(rxc, e.bottom + g, S(172), 15);
        mv(g_hChkSchedBk, cAutoBk.left + p, cAutoBk.top + S(46), iw, S(24));
        desc(cAutoBk.left + p, cAutoBk.top + S(74), iw, S(36), 8);
        int by = cAutoBk.top + S(122);
        g_f[F_BK_HOURS].rc = { cAutoBk.left + p, by, cAutoBk.left + p + iw - S(90), by + fh };
        mv(g_hEditBkHours, cAutoBk.left + p + S(10), by + (fh - g_textH) / 2, iw - S(110), g_textH);
        mv(g_hBtnSchedBkSave, cAutoBk.left + p + iw - S(80), by, S(80), fh);
    }
}

// Shows the controls of the current page and hides the rest
static void ShowPage() {
    if (g_hwnd) SendMessageW(g_hwnd, WM_SETREDRAW, FALSE, 0);

    bool pan = (g_page == PG_PANEL), prp = (g_page == PG_PROPS), acc = (g_page == PG_ACCESS),
         bkp = (g_page == PG_BACKUPS), plug = (g_page == PG_PLUGINS), prof = (g_page == PG_PROFILES), st = (g_page == PG_SETTINGS);
    HWND panelCtl[] = { g_hJar, g_hBtnBrowse, g_hIp, g_hPort, g_hRam, g_hBtnStart, g_hBtnStop, g_hList,
                        g_hBtnKick, g_hBtnBan, g_hUnban, g_hBtnUnban, g_hLog, g_hCmd, g_hBtnSend };
    HWND propCtl[]  = { g_hMotd, g_hMaxPl, g_hViewDist, g_hSpawnProt, g_hBtnPropSave, g_hBtnPropReload,
                        g_hBtnGm[0], g_hBtnGm[1], g_hBtnGm[2], g_hBtnGm[3],
                        g_hBtnDf[0], g_hBtnDf[1], g_hBtnDf[2], g_hBtnDf[3],
                        g_hChkOnline, g_hChkPvp, g_hChkWhitelist, g_hChkFlight, g_hChkCmdBlock };
    HWND accCtl[]   = { g_hListWl, g_hEditWl, g_hBtnWlAdd, g_hBtnWlRem, g_hBtnWlReload,
                        g_hListOp, g_hEditOp, g_hBtnOpAdd, g_hBtnOpRem, g_hBtnOpReload };
    HWND bkpCtl[]   = { g_hEditBkTag, g_hBtnBkCreate, g_hBtnBkOpen, g_hListBk, g_hBtnBkRestore, g_hBtnBkDel, g_hBtnBkReload };
    HWND plugCtl[]  = { g_hListPlugins, g_hBtnPlugToggle, g_hBtnPlugAdd, g_hBtnPlugDel, g_hBtnPlugOpen, g_hBtnPlugReload,
                        g_hListMods,    g_hBtnModToggle,  g_hBtnModAdd,  g_hBtnModDel,  g_hBtnModOpen,  g_hBtnModReload };
    HWND profCtl[]  = { g_hEditProfName, g_hBtnProfSave, g_hListProf, g_hBtnProfLoad, g_hBtnProfDel };
    HWND genCtl[] = { g_hChkAutoStart, g_hBtnLangTR, g_hBtnLangEN,
                      g_hBtnThDark, g_hBtnThLight, g_hBtnThRedstone, g_hBtnThMc };
    HWND srvCtl[] = { g_hChkAutoRestart, g_hBtnLogs, g_hJvm, g_hBtnJvmSave,
                      g_hChkSchedBk, g_hEditBkHours, g_hBtnSchedBkSave,
                      g_hChkSchedRestart, g_hEditRestartTime, g_hBtnSchedRestartSave };
    for (HWND h : panelCtl) if (h) ShowWindow(h, pan ? SW_SHOW : SW_HIDE);
    for (HWND h : propCtl)  if (h) ShowWindow(h, prp ? SW_SHOW : SW_HIDE);
    for (HWND h : accCtl)   if (h) ShowWindow(h, acc ? SW_SHOW : SW_HIDE);
    for (HWND h : bkpCtl)   if (h) ShowWindow(h, bkp ? SW_SHOW : SW_HIDE);
    for (HWND h : plugCtl)  if (h) ShowWindow(h, plug ? SW_SHOW : SW_HIDE);
    for (HWND h : profCtl)  if (h) ShowWindow(h, prof ? SW_SHOW : SW_HIDE);
    for (HWND h : genCtl)   if (h) ShowWindow(h, (st && g_subTab == 0) ? SW_SHOW : SW_HIDE);
    for (HWND h : srvCtl)   if (h) ShowWindow(h, (st && g_subTab == 1) ? SW_SHOW : SW_HIDE);
    if (prp) LoadServerPropertiesToUi();
    if (acc) LoadAccessListsToUi();
    if (bkp) LoadBackupsListToUi();
    if (plug) {
        LoadModFolder(L"plugins", g_pluginsList, g_hListPlugins);
        LoadModFolder(L"mods", g_modsList, g_hListMods);
    }
    if (prof) LoadProfilesListToUi();
    g_autoStartCache = IsAutoStart();

    // If theme changed while on another page, recolor log now that we returned to the dashboard
    if (pan && g_logThemeDirty) {
        g_logThemeDirty = false;
        ReapplyLogColors();
    } else if (pan && g_hLog) {
        SendMessageW(g_hLog, WM_VSCROLL, SB_BOTTOM, 0);
        LockConsoleHorizontalScroll();
    }

    RECT rc; GetClientRect(g_hwnd, &rc);
    Layout(rc.right, rc.bottom);

    if (g_hwnd) {
        SendMessageW(g_hwnd, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(g_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}
static void NavigateTo(int pg) {
    if (pg < 0 || pg >= PG_COUNT || pg == g_page) return;
    int oldPage = g_page;
    g_page = pg;

    g_tabTargetX = (float)g_rcTab[pg].left;
    g_tabTargetW = (float)(g_rcTab[pg].right - g_rcTab[pg].left);

    if (g_tabCurX < 0 && g_rcTab[oldPage].right > g_rcTab[oldPage].left) {
        g_tabCurX = (float)g_rcTab[oldPage].left;
        g_tabCurW = (float)(g_rcTab[oldPage].right - g_rcTab[oldPage].left);
    }

    if (g_hwnd) {
        InitHighResTimer();
        if (g_animPerfFreq.QuadPart == 0) QueryPerformanceFrequency(&g_animPerfFreq);
        QueryPerformanceCounter(&g_animLastTime);
        g_animatingTab = true;

        int hz = GetMonitorRefreshRate(g_hwnd);
        int timerMs = std::max(1, 1000 / hz);
        SetTimer(g_hwnd, TID_ANIM, (UINT)timerMs, NULL);
    }

    ShowPage();
}

// ----------------------------------------------------------------------------
//  Window Creation & Menus
// ----------------------------------------------------------------------------
static HFONT MakeFont(int pt, int weight, const wchar_t* face) {
    return CreateFontW(-MulDiv(pt, g_dpi, 72), 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
}
static HWND MakeEdit(int id, DWORD extra = 0) {
    HWND h = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | extra,
                             0, 0, 10, 10, g_hwnd, (HMENU)(INT_PTR)id, g_hInst, NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)g_fUI, TRUE);
    WNDPROC prev = (WNDPROC)SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)EditProc);
    if (!g_oldEdit) g_oldEdit = prev;
    return h;
}
static HWND MakeBtn(int id, const wchar_t* text) {
    HWND h = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                             0, 0, 10, 10, g_hwnd, (HMENU)(INT_PTR)id, g_hInst, NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)g_fSemi, TRUE);
    WNDPROC prev = (WNDPROC)SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)BtnProc);
    if (!g_oldBtn) g_oldBtn = prev;
    return h;
}

static bool OnCreate(HWND hwnd) {
    g_hwnd = hwnd;
    g_fUI   = MakeFont(10, FW_NORMAL, L"Segoe UI");
    g_fSemi = MakeFont(10, FW_SEMIBOLD, L"Segoe UI");
    g_fCap  = MakeFont(8, FW_SEMIBOLD, L"Segoe UI");
    g_fVal  = MakeFont(13, FW_SEMIBOLD, L"Segoe UI");
    g_fMono = MakeFont(9, FW_NORMAL, L"Consolas");
    g_bBg = CreateSolidBrush(P.bg); g_bField = CreateSolidBrush(P.field); g_bLog = CreateSolidBrush(P.logBg);

    HDC dc = GetDC(hwnd);
    HGDIOBJ of = SelectObject(dc, g_fUI);
    TEXTMETRICW tm; GetTextMetricsW(dc, &tm); g_textH = tm.tmHeight;
    SelectObject(dc, of); ReleaseDC(hwnd, dc);

    g_hJar  = MakeEdit(ID_EDIT_JAR);
    g_hBtnBrowse = MakeBtn(ID_BTN_BROWSE, L().btnBrowse);
    g_hIp   = MakeEdit(ID_EDIT_IP);
    g_hPort = MakeEdit(ID_EDIT_PORT, ES_NUMBER);
    g_hRam  = MakeEdit(ID_EDIT_RAM);
    g_hBtnStart = MakeBtn(ID_BTN_START, L().btnStart);
    g_hBtnStop  = MakeBtn(ID_BTN_STOP, L().btnStop);

    g_hList = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                              LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
                              0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_LIST_PLAYERS, g_hInst, NULL);
    SendMessageW(g_hList, WM_SETFONT, (WPARAM)g_fUI, TRUE);
    g_hBtnKick = MakeBtn(ID_BTN_KICK, L().btnKick);
    g_hBtnBan  = MakeBtn(ID_BTN_BAN, L().btnBan);
    g_hUnban   = MakeEdit(ID_EDIT_UNBAN);
    g_hBtnUnban = MakeBtn(ID_BTN_UNBAN, L().btnUnban);

    g_hLog = CreateWindowExW(0, GetRichEditClass(), L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                             0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_EDIT_LOG, g_hInst, NULL);
    SendMessageW(g_hLog, WM_SETFONT, (WPARAM)g_fMono, TRUE);
    SendMessageW(g_hLog, EM_EXLIMITTEXT, 0, 10000000);
    SendMessageW(g_hLog, EM_SETBKGNDCOLOR, 0, (LPARAM)P.logBg);
    SendMessageW(g_hLog, EM_SETTARGETDEVICE, 0, 0);
    SendMessageW(g_hLog, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(S(6), S(6)));
    SendMessageW(g_hLog, EM_AUTOURLDETECT, FALSE, 0);
    g_hCmd = MakeEdit(ID_EDIT_CMD);
    g_hBtnSend = MakeBtn(ID_BTN_SEND, L().btnSend);

    // Settings page controls (hidden until the page / sub-tab is shown)
    g_hChkAutoStart   = MakeBtn(ID_CHK_AUTOSTART, L().menuAutoStart);
    g_hChkAutoRestart = MakeBtn(ID_MENU_AUTORESTART, Tx(L"Sunucu çökerse otomatik yeniden başlat", L"Restart the server automatically if it crashes"));
    g_hBtnLangTR  = MakeBtn(ID_MENU_LANG_TR, L"Türkçe");
    g_hBtnLangEN  = MakeBtn(ID_MENU_LANG_EN, L"English");
    g_hBtnThDark     = MakeBtn(ID_MENU_THEME_DARK, L().menuThemeDark);
    g_hBtnThLight    = MakeBtn(ID_MENU_THEME_LIGHT, L().menuThemeLight);
    g_hBtnThRedstone = MakeBtn(ID_MENU_THEME_REDSTONE, L().menuThemeRedstone);
    g_hBtnThMc       = MakeBtn(ID_MENU_THEME_MC, L().menuThemeMc);
    g_hBtnLogs    = MakeBtn(ID_MENU_OPENLOGS, Tx(L"Log Klasörünü Aç", L"Open Logs Folder"));
    g_hBtnJvmSave = MakeBtn(ID_BTN_JVMSAVE, Tx(L"Kaydet", L"Save"));
    g_hJvm = MakeEdit(ID_EDIT_JVM);
    SendMessageW(g_hJvm, EM_LIMITTEXT, 256, 0);

    // Server Properties page controls
    g_hMotd          = MakeEdit(ID_EDIT_MOTD);
    g_hMaxPl         = MakeEdit(ID_EDIT_MAXPL, ES_NUMBER);
    g_hViewDist      = MakeEdit(ID_EDIT_VIEWDIST, ES_NUMBER);
    g_hSpawnProt     = MakeEdit(ID_EDIT_SPAWNPROT, ES_NUMBER);
    SendMessageW(g_hMotd, EM_LIMITTEXT, 128, 0);
    SendMessageW(g_hMaxPl, EM_LIMITTEXT, 5, 0);
    SendMessageW(g_hViewDist, EM_LIMITTEXT, 2, 0);
    SendMessageW(g_hSpawnProt, EM_LIMITTEXT, 4, 0);
    g_hBtnPropSave   = MakeBtn(ID_BTN_PROPSAVE,   Tx(L"Ayarları Kaydet", L"Save Properties"));
    g_hBtnPropReload = MakeBtn(ID_BTN_PROPRELOAD, Tx(L"Yeniden Yükle", L"Reload"));
    g_hBtnGm[0]      = MakeBtn(ID_BTN_GM_SURV,    Tx(L"Hayatta Kalma", L"Survival"));
    g_hBtnGm[1]      = MakeBtn(ID_BTN_GM_CREA,    Tx(L"Yaratıcı", L"Creative"));
    g_hBtnGm[2]      = MakeBtn(ID_BTN_GM_ADV,     Tx(L"Macera", L"Adventure"));
    g_hBtnGm[3]      = MakeBtn(ID_BTN_GM_SPEC,    Tx(L"İzleyici", L"Spectator"));
    g_hBtnDf[0]      = MakeBtn(ID_BTN_DF_PEACE,   Tx(L"Barışçıl", L"Peaceful"));
    g_hBtnDf[1]      = MakeBtn(ID_BTN_DF_EASY,    Tx(L"Kolay", L"Easy"));
    g_hBtnDf[2]      = MakeBtn(ID_BTN_DF_NORM,    Tx(L"Normal", L"Normal"));
    g_hBtnDf[3]      = MakeBtn(ID_BTN_DF_HARD,    Tx(L"Zor", L"Hard"));
    g_hChkOnline     = MakeBtn(ID_CHK_ONLINE,     Tx(L"Orijinal Hesap Zorunlu (online-mode)", L"Require Premium Account (online-mode)"));
    g_hChkPvp        = MakeBtn(ID_CHK_PVP,        Tx(L"Oyuncular Arası Hasar (PVP)", L"Player vs Player Damage (PVP)"));
    g_hChkWhitelist  = MakeBtn(ID_CHK_WHITELIST,  Tx(L"Whitelist Zorunlu (white-list)", L"Enforce Whitelist (white-list)"));
    g_hChkFlight     = MakeBtn(ID_CHK_FLIGHT,     Tx(L"Uçmaya İzin Ver (allow-flight)", L"Allow Flight (allow-flight)"));
    g_hChkCmdBlock   = MakeBtn(ID_CHK_CMDBLOCK,   Tx(L"Komut Bloklarını Etkinleştir (enable-command-block)", L"Enable Command Blocks (enable-command-block)"));

    // Access (Whitelist & OP) page controls
    g_hListWl = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                                LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
                                0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_LIST_WL, g_hInst, NULL);
    SendMessageW(g_hListWl, WM_SETFONT, (WPARAM)g_fUI, TRUE);
    g_hEditWl      = MakeEdit(ID_EDIT_WL);
    SendMessageW(g_hEditWl, EM_LIMITTEXT, 32, 0);
    g_hBtnWlAdd    = MakeBtn(ID_BTN_WL_ADD,    Tx(L"Whitelist Ekle", L"Add to Whitelist"));
    g_hBtnWlRem    = MakeBtn(ID_BTN_WL_REM,    Tx(L"Seçileni Çıkar", L"Remove Selected"));
    g_hBtnWlReload = MakeBtn(ID_BTN_WL_RELOAD, Tx(L"Yenile", L"Reload"));

    g_hListOp = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                                LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
                                0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_LIST_OP, g_hInst, NULL);
    SendMessageW(g_hListOp, WM_SETFONT, (WPARAM)g_fUI, TRUE);
    g_hEditOp      = MakeEdit(ID_EDIT_OP);
    SendMessageW(g_hEditOp, EM_LIMITTEXT, 32, 0);
    g_hBtnOpAdd    = MakeBtn(ID_BTN_OP_ADD,    Tx(L"OP Ver", L"Grant OP"));
    g_hBtnOpRem    = MakeBtn(ID_BTN_OP_REM,    Tx(L"OP Al (Deop)", L"Revoke OP"));
    g_hBtnOpReload = MakeBtn(ID_BTN_OP_RELOAD, Tx(L"Yenile", L"Reload"));

    // Backups page controls
    g_hEditBkTag    = MakeEdit(ID_EDIT_BK_TAG);
    SendMessageW(g_hEditBkTag, EM_LIMITTEXT, 32, 0);
    g_hBtnBkCreate  = MakeBtn(ID_BTN_BK_CREATE,  Tx(L"Şimdi Yedek Oluştur (.zip)", L"Create Backup Now (.zip)"));
    g_hBtnBkOpen    = MakeBtn(ID_BTN_BK_OPEN,    Tx(L"Yedekler Klasörünü Aç", L"Open Backups Folder"));
    g_hListBk = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                                LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
                                0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_LIST_BK, g_hInst, NULL);
    SendMessageW(g_hListBk, WM_SETFONT, (WPARAM)g_fUI, TRUE);
    g_hBtnBkRestore = MakeBtn(ID_BTN_BK_RESTORE, Tx(L"Seçileni Geri Yükle", L"Restore Selected"));
    g_hBtnBkDel     = MakeBtn(ID_BTN_BK_DEL,     Tx(L"Seçileni Sil", L"Delete Selected"));
    g_hBtnBkReload  = MakeBtn(ID_BTN_BK_RELOAD,  Tx(L"Yenile", L"Reload"));

    // Profiles page controls
    g_hEditProfName = MakeEdit(ID_EDIT_PROF_NAME);
    SendMessageW(g_hEditProfName, EM_LIMITTEXT, 48, 0);
    g_hBtnProfSave  = MakeBtn(ID_BTN_PROF_SAVE, Tx(L"Mevcut Ayarları Profil Olarak Kaydet", L"Save Current Settings as Profile"));
    g_hListProf = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                                  LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
                                  0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_LIST_PROF, g_hInst, NULL);
    SendMessageW(g_hListProf, WM_SETFONT, (WPARAM)g_fUI, TRUE);
    g_hBtnProfLoad = MakeBtn(ID_BTN_PROF_LOAD, Tx(L"Seçili Profili Yükle (Aktif Et)", L"Load Selected Profile"));
    g_hBtnProfDel  = MakeBtn(ID_BTN_PROF_DEL,  Tx(L"Seçili Profili Sil", L"Delete Selected Profile"));

    // Plugins & Mods page controls
    g_hListPlugins = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                                     LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
                                     0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_LIST_PLUGINS, g_hInst, NULL);
    SendMessageW(g_hListPlugins, WM_SETFONT, (WPARAM)g_fUI, TRUE);
    g_hBtnPlugToggle = MakeBtn(ID_BTN_PLUG_TOGGLE, Tx(L"Durumu Değiştir (Aktif / Devre Dışı)", L"Toggle Status (Active / Disabled)"));
    g_hBtnPlugAdd    = MakeBtn(ID_BTN_PLUG_ADD,    Tx(L"+ Ekle", L"+ Add"));
    g_hBtnPlugDel    = MakeBtn(ID_BTN_PLUG_DEL,    Tx(L"Sil", L"Delete"));
    g_hBtnPlugOpen   = MakeBtn(ID_BTN_PLUG_OPEN,   Tx(L"Klasör", L"Folder"));
    g_hBtnPlugReload = MakeBtn(ID_BTN_PLUG_RELOAD, Tx(L"Yenile", L"Reload"));

    g_hListMods = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                                  LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
                                  0, 0, 10, 10, hwnd, (HMENU)(INT_PTR)ID_LIST_MODS, g_hInst, NULL);
    SendMessageW(g_hListMods, WM_SETFONT, (WPARAM)g_fUI, TRUE);
    g_hBtnModToggle = MakeBtn(ID_BTN_MOD_TOGGLE, Tx(L"Durumu Değiştir (Aktif / Devre Dışı)", L"Toggle Status (Active / Disabled)"));
    g_hBtnModAdd    = MakeBtn(ID_BTN_MOD_ADD,    Tx(L"+ Ekle", L"+ Add"));
    g_hBtnModDel    = MakeBtn(ID_BTN_MOD_DEL,    Tx(L"Sil", L"Delete"));
    g_hBtnModOpen   = MakeBtn(ID_BTN_MOD_OPEN,   Tx(L"Klasör", L"Folder"));
    g_hBtnModReload = MakeBtn(ID_BTN_MOD_RELOAD, Tx(L"Yenile", L"Reload"));

    // Server Automation controls (Settings -> Server)
    g_hChkSchedBk          = MakeBtn(ID_CHK_SCHED_BK, Tx(L"Otomatik zamanlı yedeklemeyi etkinleştir", L"Enable scheduled auto-backups"));
    g_hEditBkHours         = MakeEdit(ID_EDIT_BK_HOURS, ES_NUMBER);
    SendMessageW(g_hEditBkHours, EM_LIMITTEXT, 2, 0);
    g_hBtnSchedBkSave      = MakeBtn(ID_BTN_SCHED_BK_SAVE, Tx(L"Kaydet", L"Save"));
    g_hChkSchedRestart     = MakeBtn(ID_CHK_SCHED_RESTART, Tx(L"Günlük otomatik yeniden başlatmayı etkinleştir", L"Enable daily scheduled restart"));
    g_hEditRestartTime     = MakeEdit(ID_EDIT_RESTART_TIME);
    SendMessageW(g_hEditRestartTime, EM_LIMITTEXT, 5, 0);
    g_hBtnSchedRestartSave = MakeBtn(ID_BTN_SCHED_RESTART_SAVE, Tx(L"Kaydet", L"Save"));

    SendMessageW(g_hIp, EM_LIMITTEXT, 64, 0); SendMessageW(g_hPort, EM_LIMITTEXT, 5, 0);
    SendMessageW(g_hRam, EM_LIMITTEXT, 7, 0); SendMessageW(g_hUnban, EM_LIMITTEXT, 32, 0);
    SendMessageW(g_hCmd, EM_LIMITTEXT, 256, 0);
    SendMessageW(g_hCmd, WM_SETFONT, (WPARAM)g_fMono, TRUE);

    g_f[F_JAR]   = { g_hJar,   {}, L().capJar };
    g_f[F_IP]    = { g_hIp,    {}, L().capIp };
    g_f[F_PORT]  = { g_hPort,  {}, L().capPort };
    g_f[F_RAM]   = { g_hRam,   {}, L().capRam };
    g_f[F_UNBAN] = { g_hUnban, {}, L().capUnban };
    g_f[F_CMD]   = { g_hCmd,   {}, NULL };
    g_f[F_JVM]   = { g_hJvm,   {}, Tx(L"JVM argümanları", L"JVM arguments") };
    g_f[F_MOTD]      = { g_hMotd,      {}, Tx(L"Sunucu açıklaması (MOTD)", L"Server description (MOTD)") };
    g_f[F_MAXPL]     = { g_hMaxPl,     {}, Tx(L"Maks. oyuncu", L"Max players") };
    g_f[F_VIEWDIST]  = { g_hViewDist,  {}, Tx(L"Görüş mesafesi (chunk)", L"View distance (chunks)") };
    g_f[F_SPAWNPROT] = { g_hSpawnProt, {}, Tx(L"Başlangıç koruması", L"Spawn protection") };
    g_f[F_WL]        = { g_hEditWl,    {}, Tx(L"Oyuncu adı", L"Player name") };
    g_f[F_OP]        = { g_hEditOp,    {}, Tx(L"Oyuncu adı", L"Player name") };
    g_f[F_BK_TAG]    = { g_hEditBkTag, {}, Tx(L"Yedek etiketi (isteğe bağlı, ör: sezon1)", L"Backup label (optional, e.g. pre-update)") };
    g_f[F_PROF_NAME] = { g_hEditProfName, {}, Tx(L"Profil adı (ör: Survival 1.21)", L"Profile name (e.g. Survival 1.21)") };
    g_f[F_BK_HOURS]      = { g_hEditBkHours,     {}, Tx(L"Yedekleme aralığı (saat)", L"Backup interval (hours)") };
    g_f[F_RESTART_TIME]  = { g_hEditRestartTime, {}, Tx(L"Yeniden başlatma saati (SS:DD)", L"Restart time (HH:MM)") };

    ApplyScrollTheme(g_hLog);
    ApplyScrollTheme(g_hList);
    ApplyScrollTheme(g_hListWl);
    ApplyScrollTheme(g_hListOp);
    ApplyScrollTheme(g_hListBk);
    ApplyScrollTheme(g_hListProf);
    ApplyScrollTheme(g_hListPlugins);
    ApplyScrollTheme(g_hListMods);
    ApplyTitleBarTheme(hwnd);

    g_icoBig = MakeIcon(32, 0); g_icoSmall = MakeIcon(16, 0);
    int ts = GetSystemMetrics(SM_CXSMICON);
    g_trayIco[0] = MakeIcon(ts, 0); g_trayIco[1] = MakeIcon(ts, 1);
    g_trayIco[2] = MakeIcon(ts, 2); g_trayIco[3] = MakeIcon(ts, 3);
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_icoBig);
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_icoSmall);

    InitHighResTimer();
    if (g_animPerfFreq.QuadPart == 0) QueryPerformanceFrequency(&g_animPerfFreq);

    LoadSettings();
    UpdateTelemetryStats();
    SetTimer(hwnd, TID_STATS, 1000, NULL);
    TrayAdd();
    UpdateUi();
    ShowPage();
    AppLog(L().logAppReady);
    return true;
}

static void AppendLangAndThemeMenus(HMENU parent) {
    HMENU mLang = CreatePopupMenu();
    if (mLang) {
        AppendMenuW(mLang, MF_STRING | (g_lang == Lang::TR ? MF_CHECKED : 0), ID_MENU_LANG_TR, L"Türkçe");
        AppendMenuW(mLang, MF_STRING | (g_lang == Lang::EN ? MF_CHECKED : 0), ID_MENU_LANG_EN, L"English");
        if (!AppendMenuW(parent, MF_POPUP, (UINT_PTR)mLang, L().menuLang)) DestroyMenu(mLang);
    }

    HMENU mTheme = CreatePopupMenu();
    if (mTheme) {
        AppendMenuW(mTheme, MF_STRING | (g_theme == Theme::Dark ? MF_CHECKED : 0), ID_MENU_THEME_DARK, L().menuThemeDark);
        AppendMenuW(mTheme, MF_STRING | (g_theme == Theme::Light ? MF_CHECKED : 0), ID_MENU_THEME_LIGHT, L().menuThemeLight);
        AppendMenuW(mTheme, MF_STRING | (g_theme == Theme::Redstone ? MF_CHECKED : 0), ID_MENU_THEME_REDSTONE, L().menuThemeRedstone);
        AppendMenuW(mTheme, MF_STRING | (g_theme == Theme::Minecraft ? MF_CHECKED : 0), ID_MENU_THEME_MC, L().menuThemeMc);
        if (!AppendMenuW(parent, MF_POPUP, (UINT_PTR)mTheme, L().menuTheme)) DestroyMenu(mTheme);
    }
}

static void ShowTrayMenu() {
    HMENU m = CreatePopupMenu();
    if (!m) return;
    bool vis = IsWindowVisible(g_hwnd) != 0;
    bool active = g_state == SrvState::Starting || g_state == SrvState::Running;
    AppendMenuW(m, MF_STRING, ID_TRAY_SHOW, vis ? L().trayHide : L().trayShow);
    SetMenuDefaultItem(m, ID_TRAY_SHOW, FALSE);
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING | (g_state == SrvState::Stopped ? 0 : MF_GRAYED), ID_TRAY_START, L().trayStart);
    AppendMenuW(m, MF_STRING | (active ? 0 : MF_GRAYED), ID_TRAY_STOP, L().trayStop);
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendLangAndThemeMenus(m);
    AppendMenuW(m, MF_STRING | (IsAutoStart() ? MF_CHECKED : 0), ID_TRAY_AUTOSTART, L().menuAutoStart);
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, ID_TRAY_EXIT, L().trayExit);
    POINT pt; GetCursorPos(&pt);
    SetForegroundWindow(g_hwnd);
    int cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, g_hwnd, NULL);
    PostMessageW(g_hwnd, WM_NULL, 0, 0);
    DestroyMenu(m);
    if (cmd) PostMessageW(g_hwnd, WM_COMMAND, cmd, 0);
}

static void HandleServerEvent(SrvEvent* e) {
    std::unique_ptr<SrvEvent> own(e);
    if (e->type == EV_SKIN) {
        std::wstring key = ToLowerW(e->text);
        g_skinPending.erase(key);
            if (e->hbm) {
                auto it = g_skinCache.find(key);
                if (it != g_skinCache.end() && it->second) DeleteObject(it->second);
                g_skinCache[key] = e->hbm;
                e->hbm = NULL;
                if (g_hList) InvalidateRect(g_hList, NULL, FALSE);
                if (g_hListWl) InvalidateRect(g_hListWl, NULL, FALSE);
                if (g_hListOp) InvalidateRect(g_hListOp, NULL, FALSE);
            }
        return;
    }
    if (e->type == EV_BACKUP_DONE) {
        if (g_hBtnBkCreate)  EnableWindow(g_hBtnBkCreate, TRUE);
        if (g_hBtnBkRestore) EnableWindow(g_hBtnBkRestore, TRUE);
        AppLog(e->text);
        LoadBackupsListToUi();
        return;
    }
    if (e->hbm) { DeleteObject(e->hbm); e->hbm = NULL; }
    if (e->gen != g_gen || g_state == SrvState::Stopped) return;

    switch (e->type) {
        case EV_LOG: AppendLog(e->text); break;
        case EV_JOIN:
            if (!ValidPlayerName(e->text)) break;
            RequestPlayerSkin(e->text);
            if (SendMessageW(g_hList, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)e->text.c_str()) == LB_ERR) {
                SendMessageW(g_hList, LB_ADDSTRING, 0, (LPARAM)e->text.c_str());
                Balloon(L().balloonJoinTitle, e->text + L().balloonJoinSuffix);
            }
            TrayUpdate(); InvalidateRect(g_hwnd, NULL, FALSE); break;
        case EV_LEAVE: {
            int i = (int)SendMessageW(g_hList, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)e->text.c_str());
            if (i != LB_ERR) {
                SendMessageW(g_hList, LB_DELETESTRING, i, 0);
                Balloon(L().balloonLeaveTitle, e->text + L().balloonLeaveSuffix);
            }
            TrayUpdate(); InvalidateRect(g_hwnd, NULL, FALSE); break;
        }
        case EV_SYNC_LIST: {
            SendMessageW(g_hList, LB_RESETCONTENT, 0, 0);
            size_t p = 0;
            while (p < e->text.size()) {
                size_t c = e->text.find(L',', p);
                if (c == std::wstring::npos) c = e->text.size();
                std::wstring n = Trim(e->text.substr(p, c - p));
                size_t sp = n.rfind(L' ');
                if (sp != std::wstring::npos) n = n.substr(sp + 1);
                if (ValidPlayerName(n)) {
                    RequestPlayerSkin(n);
                    if (SendMessageW(g_hList, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)n.c_str()) == LB_ERR)
                        SendMessageW(g_hList, LB_ADDSTRING, 0, (LPARAM)n.c_str());
                }
                p = c + 1;
            }
            TrayUpdate(); InvalidateRect(g_hwnd, NULL, FALSE); break;
        }
        case EV_TIME: g_igTime = e->text; InvalidateRect(g_hwnd, &g_rcStats, FALSE); break;
        case EV_RETRY_TIME: if (g_state == SrvState::Running) QueryTime(); break;
        case EV_READY:
            if (g_state == SrvState::Starting) {
                g_state = SrvState::Running;
                SetTimer(g_hwnd, TID_TIME, 15000, NULL);
                QueryTime();
                AppLog(L().logReady);
                Balloon(L"Redstone Server Panel", L().balloonReady);
                UpdateUi();
            }
            break;
        case EV_STOPPING:
            if (g_state == SrvState::Starting || g_state == SrvState::Running) { g_state = SrvState::Stopping; UpdateUi(); }
            break;
        case EV_EXITED: OnServerExited(); break;
    }
}

// ----------------------------------------------------------------------------
//  Main Window Procedure
// ----------------------------------------------------------------------------
static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE:
            return OnCreate(hwnd) ? 0 : -1;

        case WM_SIZE:
            if (wp != SIZE_MINIMIZED) {
                Layout(LOWORD(lp), HIWORD(lp));
                if (g_hLog) SendMessageW(g_hLog, EM_SETTARGETDEVICE, 0, 0);
            }
            return 0;

        case WM_GETMINMAXINFO: {
            MINMAXINFO* mm = (MINMAXINFO*)lp;
            RECT r = { 0, 0, S(980), S(700) + HeaderH() };
            AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
            mm->ptMinTrackSize.x = r.right - r.left; mm->ptMinTrackSize.y = r.bottom - r.top;
            return 0;
        }

        case WM_ERASEBKGND: return 1;
        case WM_PAINT: PaintMain(hwnd); return 0;

        case WM_MOUSEMOVE: {
            POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
            int hoverTab = -1;
            int hoverSub = -1;
            if (pt.y < S(46)) {
                for (int i = 0; i < PG_COUNT; i++) {
                    if (PtInRect(&g_rcTab[i], pt)) { hoverTab = i; break; }
                }
            } else if (g_page == PG_SETTINGS && pt.y < HeaderH() + S(46)) {
                for (int i = 0; i < 2; i++) {
                    if (PtInRect(&g_rcSub[i], pt)) { hoverSub = i; break; }
                }
            }
            bool changed = false;
            if (hoverTab != g_tabHover) { g_tabHover = hoverTab; changed = true; }
            if (hoverSub != g_subTabHover) { g_subTabHover = hoverSub; changed = true; }
            if (changed) {
                RECT rHead = { 0, 0, 4000, HeaderH() + S(46) };
                InvalidateRect(hwnd, &rHead, FALSE);
            }
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            break;
        }
        case WM_MOUSELEAVE: {
            if (g_tabHover != -1 || g_subTabHover != -1) {
                g_tabHover = -1;
                g_subTabHover = -1;
                RECT rHead = { 0, 0, 4000, HeaderH() + S(46) };
                InvalidateRect(hwnd, &rHead, FALSE);
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
            for (int i = 0; i < PG_COUNT; i++) if (PtInRect(&g_rcTab[i], pt)) { NavigateTo(i); return 0; }
            if (g_page == PG_SETTINGS)
                for (int i = 0; i < 2; i++) if (PtInRect(&g_rcSub[i], pt)) {
                    if (g_subTab != i) { g_subTab = i; ShowPage(); }
                    return 0;
                }
            return 0;
        }
        case WM_SETCURSOR:
            if (LOWORD(lp) == HTCLIENT) {
                POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
                if (OverChrome(pt)) { SetCursor(LoadCursor(NULL, IDC_HAND)); return TRUE; }
            }
            break;

        case WM_CTLCOLOREDIT: {
            HDC dc = (HDC)wp; SetTextColor(dc, P.text); SetBkColor(dc, P.field);
            return (LRESULT)g_bField;
        }
        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wp; HWND c = (HWND)lp;
            if (c == g_hLog) { SetTextColor(dc, P.logText); SetBkColor(dc, P.logBg); return (LRESULT)g_bLog; }
            SetTextColor(dc, P.muted); SetBkColor(dc, P.field); return (LRESULT)g_bField;
        }
        case WM_CTLCOLORLISTBOX: {
            HDC dc = (HDC)wp; SetTextColor(dc, P.text); SetBkColor(dc, P.logBg);
            return (LRESULT)g_bLog;
        }

        case WM_MEASUREITEM: {
            MEASUREITEMSTRUCT* mi = (MEASUREITEMSTRUCT*)lp;
            if (mi->CtlType == ODT_LISTBOX) { mi->itemHeight = S(38); return TRUE; }
            break;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lp;
            if (di->CtlType == ODT_BUTTON) { DrawButton(di); return TRUE; }
            if (di->CtlType == ODT_LISTBOX) { DrawPlayer(di); return TRUE; }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wp), code = HIWORD(wp);
            if (code == EN_SETFOCUS || code == EN_KILLFOCUS) {
                for (int i = 0; i < F_COUNT; i++) if (g_f[i].h == (HWND)lp) {
                    RECT r = g_f[i].rc; InflateRect(&r, S(3), S(3)); InvalidateRect(hwnd, &r, FALSE);
                }
                return 0;
            }
            if (id == ID_LIST_PLAYERS || id == ID_LIST_WL || id == ID_LIST_OP || id == ID_LIST_BK || id == ID_LIST_PROF || id == ID_LIST_PLUGINS || id == ID_LIST_MODS) return 0;
            if (code != BN_CLICKED && lp != 0) return 0;
            switch (id) {
                case ID_BTN_BROWSE: {
                    std::wstring p = OpenJarDialog();
                    if (!p.empty()) { SetWindowTextW(g_hJar, p.c_str()); SaveSettings(); LoadServerPropertiesToUi(); }
                    break;
                }
                case ID_BTN_PROPSAVE: SaveServerPropertiesFromUi(); break;
                case ID_BTN_PROPRELOAD: LoadServerPropertiesToUi(); break;
                case ID_BTN_GM_SURV: case ID_BTN_GM_CREA: case ID_BTN_GM_ADV: case ID_BTN_GM_SPEC:
                    g_propGm = id - ID_BTN_GM_SURV;
                    for (HWND b : g_hBtnGm) if (b) InvalidateRect(b, NULL, FALSE);
                    break;
                case ID_BTN_DF_PEACE: case ID_BTN_DF_EASY: case ID_BTN_DF_NORM: case ID_BTN_DF_HARD:
                    g_propDf = id - ID_BTN_DF_PEACE;
                    for (HWND b : g_hBtnDf) if (b) InvalidateRect(b, NULL, FALSE);
                    break;
                case ID_CHK_ONLINE:    g_propOnline    = !g_propOnline;    InvalidateRect(g_hChkOnline, NULL, FALSE); break;
                case ID_CHK_PVP:       g_propPvp       = !g_propPvp;       InvalidateRect(g_hChkPvp, NULL, FALSE); break;
                case ID_CHK_WHITELIST: g_propWhitelist = !g_propWhitelist; InvalidateRect(g_hChkWhitelist, NULL, FALSE); break;
                case ID_CHK_FLIGHT:    g_propFlight    = !g_propFlight;    InvalidateRect(g_hChkFlight, NULL, FALSE); break;
                case ID_CHK_CMDBLOCK:  g_propCmdBlock  = !g_propCmdBlock;  InvalidateRect(g_hChkCmdBlock, NULL, FALSE); break;
                case ID_BTN_WL_ADD:    ModifyAccessList(false, true); break;
                case ID_BTN_WL_REM:    ModifyAccessList(false, false); break;
                case ID_BTN_WL_RELOAD: LoadAccessListsToUi(); break;
                case ID_BTN_OP_ADD:    ModifyAccessList(true, true); break;
                case ID_BTN_OP_REM:    ModifyAccessList(true, false); break;
                case ID_BTN_OP_RELOAD: LoadAccessListsToUi(); break;
                case ID_BTN_BK_CREATE:  StartCreateBackup(); break;
                case ID_BTN_BK_OPEN:
                    if (!HasServerJar()) {
                        Msg(L().msgSelectJar, L().titleMissingInfo, MB_OK | MB_ICONWARNING);
                    } else {
                        ShellExecuteW(hwnd, L"open", BackupsDir().c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }
                    break;
                case ID_BTN_BK_RESTORE: RestoreSelectedBackup(); break;
                case ID_BTN_BK_DEL:     DeleteSelectedBackup(); break;
                case ID_BTN_BK_RELOAD:  LoadBackupsListToUi(); break;
                case ID_BTN_PROF_SAVE:  SaveCurrentAsProfile(); break;
                case ID_BTN_PROF_LOAD:  LoadSelectedProfile(); break;
                case ID_BTN_PROF_DEL:   DeleteSelectedProfile(); break;

                case ID_BTN_PLUG_TOGGLE: ToggleModItem(true); break;
                case ID_BTN_PLUG_ADD:    AddModItem(true); break;
                case ID_BTN_PLUG_DEL:    DeleteModItem(true); break;
                case ID_BTN_PLUG_OPEN:   OpenModFolder(true); break;
                case ID_BTN_PLUG_RELOAD: LoadModFolder(L"plugins", g_pluginsList, g_hListPlugins); break;

                case ID_BTN_MOD_TOGGLE:  ToggleModItem(false); break;
                case ID_BTN_MOD_ADD:     AddModItem(false); break;
                case ID_BTN_MOD_DEL:     DeleteModItem(false); break;
                case ID_BTN_MOD_OPEN:    OpenModFolder(false); break;
                case ID_BTN_MOD_RELOAD:  LoadModFolder(L"mods", g_modsList, g_hListMods); break;

                case ID_CHK_SCHED_BK:
                    g_schedBkEnabled = !g_schedBkEnabled; SaveSettings();
                    if (g_hChkSchedBk) InvalidateRect(g_hChkSchedBk, NULL, FALSE);
                    break;
                case ID_BTN_SCHED_BK_SAVE: {
                    int h = _wtoi(Trim(GetText(g_hEditBkHours)).c_str());
                    if (h < 1 || h > 72) {
                        AppLog(Tx(L"Yedekleme aralığı 1 ile 72 saat arasında olmalıdır.", L"Backup interval must be between 1 and 72 hours."));
                        break;
                    }
                    g_schedBkHours = h;
                    SaveSettings();
                    AppLog(Tx(L"Otomatik yedekleme aralığı kaydedildi.", L"Scheduled backup interval saved."));
                    break;
                }

                case ID_CHK_SCHED_RESTART:
                    g_schedRestartEnabled = !g_schedRestartEnabled; SaveSettings();
                    if (g_hChkSchedRestart) InvalidateRect(g_hChkSchedRestart, NULL, FALSE);
                    break;
                case ID_BTN_SCHED_RESTART_SAVE: {
                    std::wstring t = Trim(GetText(g_hEditRestartTime));
                    int th = 0, tm = 0;
                    if (swscanf(t.c_str(), L"%d:%d", &th, &tm) != 2 || th < 0 || th > 23 || tm < 0 || tm > 59) {
                        AppLog(Tx(L"Saat formatı geçersiz (örnek format: 04:00)", L"Invalid time format (e.g. 04:00)"));
                        break;
                    }
                    wchar_t b[16]; swprintf(b, 16, L"%02d:%02d", th, tm);
                    g_schedRestartTime = b;
                    SetWindowTextW(g_hEditRestartTime, g_schedRestartTime.c_str());
                    SaveSettings();
                    AppLog(std::wstring(Tx(L"Yeniden başlatma saati kaydedildi: ", L"Scheduled restart time saved: ")) + g_schedRestartTime);
                    break;
                }

                case ID_BTN_START: case ID_TRAY_START: StartServer(); break;
                case ID_BTN_STOP:  case ID_TRAY_STOP:  StopServer(); break;
                case ID_BTN_KICK: DoKickBan(false); break;
                case ID_BTN_BAN:  DoKickBan(true); break;
                case ID_BTN_UNBAN: DoUnban(); break;
                case ID_BTN_SEND: SendCommandFromUi(); break;
                case ID_TRAY_SHOW: if (IsWindowVisible(hwnd)) HideMain(); else ShowMain(); break;
                case ID_TRAY_AUTOSTART: case ID_CHK_AUTOSTART:
                    SetAutoStart(!IsAutoStart()); g_autoStartCache = IsAutoStart();
                    if (g_hChkAutoStart) InvalidateRect(g_hChkAutoStart, NULL, FALSE);
                    break;
                case ID_BTN_JVMSAVE: SaveJvmFromUi(); break;
                case ID_MENU_AUTORESTART:
                    g_autoRestart = !g_autoRestart; g_crashCount = 0; SaveSettings();
                    if (g_hChkAutoRestart) InvalidateRect(g_hChkAutoRestart, NULL, FALSE);
                    break;
                case ID_MENU_OPENLOGS: ShellExecuteW(hwnd, L"open", AppDataDir().c_str(), NULL, NULL, SW_SHOWNORMAL); break;
                case ID_TRAY_EXIT: RequestExit(); break;
                case ID_MENU_LANG_TR:
                    if (g_lang != Lang::TR) { g_lang = Lang::TR; ApplyLanguage(); SaveSettings(); }
                    break;
                case ID_MENU_LANG_EN:
                    if (g_lang != Lang::EN) { g_lang = Lang::EN; ApplyLanguage(); SaveSettings(); }
                    break;
                case ID_MENU_THEME_DARK:
                    if (g_theme != Theme::Dark) { g_theme = Theme::Dark; ApplyTheme(); SaveSettings(); }
                    break;
                case ID_MENU_THEME_LIGHT:
                    if (g_theme != Theme::Light) { g_theme = Theme::Light; ApplyTheme(); SaveSettings(); }
                    break;
                case ID_MENU_THEME_REDSTONE:
                    if (g_theme != Theme::Redstone) { g_theme = Theme::Redstone; ApplyTheme(); SaveSettings(); }
                    break;
                case ID_MENU_THEME_MC:
                    if (g_theme != Theme::Minecraft) { g_theme = Theme::Minecraft; ApplyTheme(); SaveSettings(); }
                    break;
            }
            return 0;
        }

        case WM_TIMER:
            if (wp == TID_ANIM) {
                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                float dt = 0.016f;
                if (g_animPerfFreq.QuadPart > 0 && g_animLastTime.QuadPart > 0) {
                    dt = (float)(now.QuadPart - g_animLastTime.QuadPart) / (float)g_animPerfFreq.QuadPart;
                }
                g_animLastTime = now;
                if (dt <= 0.0f) dt = 0.001f;
                if (dt > 0.05f) dt = 0.05f;

                float alpha = 1.0f - expf(-24.0f * dt);
                float dx = g_tabTargetX - g_tabCurX;
                float dw = g_tabTargetW - g_tabCurW;

                if (fabs(dx) < 0.5f && fabs(dw) < 0.5f) {
                    g_tabCurX = g_tabTargetX;
                    g_tabCurW = g_tabTargetW;
                    g_animatingTab = false;
                    KillTimer(hwnd, TID_ANIM);
                } else {
                    g_tabCurX += dx * alpha;
                    g_tabCurW += dw * alpha;
                }
                RECT rHead = { 0, 0, 4000, HeaderH() };
                InvalidateRect(hwnd, &rHead, FALSE);
                return 0;
            }
            if (wp == TID_STATS) {
                UpdateTelemetryStats();
                InvalidateRect(hwnd, &g_rcStats, FALSE);
                if (g_state != SrvState::Stopped) TrayUpdate();

                // Automation: Scheduled Backups & Daily Restarts
                if (g_state == SrvState::Running) {
                    time_t now = time(NULL);
                    if (g_schedBkEnabled && !g_backupBusy.load()) {
                        if (g_lastAutoBackupTime == 0) g_lastAutoBackupTime = now;
                        else if (now - g_lastAutoBackupTime >= (time_t)(g_schedBkHours * 3600)) {
                            g_lastAutoBackupTime = now;
                            AppLog(Tx(L"[Otomasyon] Zamanlanmış otomatik yedekleme başlatılıyor...",
                                      L"[Automation] Starting scheduled auto-backup..."));
                            StartCreateBackup(true);
                        }
                    }

                    if (g_schedRestartEnabled && g_hProc) {
                        SYSTEMTIME st; GetLocalTime(&st);
                        int curMinutes = st.wHour * 60 + st.wMinute;
                        int targetH = 4, targetM = 0;
                        if (swscanf(g_schedRestartTime.c_str(), L"%d:%d", &targetH, &targetM) == 2) {
                            int targetMinutes = targetH * 60 + targetM;
                            int diff = targetMinutes - curMinutes;
                            if (diff < -720) diff += 1440;

                            if (diff != 0) g_lastRestartMinute = -1;

                            if (diff == 5 && g_restartWarningStep == 0) {
                                g_restartWarningStep = 1;
                                WriteToConsole(Tx(L"say §c[Duyuru] Sunucu 5 dakika içinde planlı olarak yeniden başlatılacak!",
                                                  L"say §c[Notice] Server will perform a scheduled restart in 5 minutes!"));
                                AppLog(Tx(L"[Otomasyon] 5 dakika uyarısı verildi.", L"[Automation] 5-minute warning sent."));
                            } else if (diff == 1 && g_restartWarningStep < 2) {
                                g_restartWarningStep = 2;
                                WriteToConsole(Tx(L"say §c[Duyuru] Sunucu 1 dakika içinde yeniden başlatılıyor!",
                                                  L"say §c[Notice] Server is restarting in 1 minute!"));
                                AppLog(Tx(L"[Otomasyon] 1 dakika uyarısı verildi.", L"[Automation] 1-minute warning sent."));
                            } else if (diff == 0 && curMinutes != g_lastRestartMinute) {
                                g_lastRestartMinute = curMinutes;
                                g_restartWarningStep = 0;
                                WriteToConsole(Tx(L"say §c[Duyuru] Sunucu planlı olarak yeniden başlatılıyor...",
                                                  L"say §c[Notice] Server is performing a scheduled restart..."));
                                AppLog(Tx(L"[Otomasyon] Zamanlanmış yeniden başlatma yürütülüyor...",
                                          L"[Automation] Executing scheduled restart..."));
                                g_schedRestartPending = true;
                                StopServer(true);
                            } else if (diff > 5 || diff < 0) {
                                g_restartWarningStep = 0;
                            }
                        }
                    }
                }
            } else if (wp == TID_TIME) {
                if (g_state == SrvState::Running) QueryTime();
            } else if (wp == TID_RESTART) {
                KillTimer(hwnd, TID_RESTART);
                bool trigSched = g_scheduledRestartTrigger;
                g_scheduledRestartTrigger = false;
                if ((g_autoRestart || trigSched) && g_state == SrvState::Stopped && !g_quitPending) StartServer();
            } else if (wp == TID_STOPWATCH) {
                KillTimer(hwnd, TID_STOPWATCH);
                if (g_state == SrvState::Stopping && g_hProc) {
                    int r = Msg(L().msgTimeoutKill, L().titleTimeout, MB_YESNO | MB_ICONWARNING);
                    if (g_state == SrvState::Stopping && g_hProc) {
                        if (r == IDYES) TerminateProcess(g_hProc, 1);
                        else SetTimer(hwnd, TID_STOPWATCH, 60000, NULL);
                    }
                }
            }
            return 0;

        case WM_SC_EVENT: HandleServerEvent((SrvEvent*)lp); return 0;
        case WM_SC_SHOW:  ShowMain(); return 0;

        case WM_TRAYICON:
            switch (LOWORD(lp)) {
                case WM_LBUTTONUP: case WM_LBUTTONDBLCLK: ShowMain(); break;
                case WM_RBUTTONUP: ShowTrayMenu(); break;
            }
            return 0;

        case WM_SYSCOMMAND:
            if ((wp & 0xFFF0) == SC_CLOSE || (wp & 0xFFF0) == SC_MINIMIZE) { HideMain(); return 0; }
            break;
        case WM_CLOSE: HideMain(); return 0;

        case WM_QUERYENDSESSION: return TRUE;
        case WM_ENDSESSION:
            if (wp && g_state != SrvState::Stopped && g_hProc) {
                WriteToConsole(L"stop");
                WaitForSingleObject(g_hProc, 10000);
                CloseHandle(g_hProc); g_hProc = NULL;
            }
            if (wp && g_hStdinWr) {
                CloseHandle(g_hStdinWr); g_hStdinWr = NULL;
            }
            return 0;

        case WM_DESTROY: {
            KillTimer(hwnd, TID_STATS); KillTimer(hwnd, TID_TIME); KillTimer(hwnd, TID_STOPWATCH); KillTimer(hwnd, TID_RESTART); KillTimer(hwnd, TID_ANIM);
            HMODULE hMm = GetModuleHandleW(L"winmm.dll");
            if (hMm) {
                typedef MMRESULT (WINAPI *PfnTimeEndPeriod)(UINT);
                PfnTimeEndPeriod pfn = reinterpret_cast<PfnTimeEndPeriod>(reinterpret_cast<void*>(GetProcAddress(hMm, "timeEndPeriod")));
                if (pfn) pfn(1);
            }
            SaveSettings();
            if (g_hProc) {
                if (WaitForSingleObject(g_hProc, 0) == WAIT_TIMEOUT) {
                    WriteToConsole(L"stop");
                    WaitForSingleObject(g_hProc, 15000);
                }
                CloseHandle(g_hProc);
                g_hProc = NULL;
            }
            if (g_hStdinWr) {
                CloseHandle(g_hStdinWr);
                g_hStdinWr = NULL;
            }
            MSG pendingMsg;
            while (PeekMessageW(&pendingMsg, NULL, WM_SC_EVENT, WM_SC_EVENT, PM_REMOVE)) {
                SrvEvent* ev = (SrvEvent*)pendingMsg.lParam;
                if (ev) delete ev;
            }
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, 0); SendMessageW(hwnd, WM_SETICON, ICON_SMALL, 0);
            for (HICON* ic : { &g_icoBig, &g_icoSmall, &g_trayIco[0], &g_trayIco[1], &g_trayIco[2], &g_trayIco[3] }) {
                if (*ic) { DestroyIcon(*ic); *ic = NULL; }
            }
            for (auto& kv : g_skinCache) if (kv.second) DeleteObject(kv.second);
            for (auto& kv : g_fallbackCache) if (kv.second) DeleteObject(kv.second);
            g_skinCache.clear(); g_fallbackCache.clear();
            PostQuitMessage(0);
            return 0;
        }

        default:
            if (g_msgTaskbarCreated && msg == g_msgTaskbarCreated) { TrayAdd(); TrayUpdate(); return 0; }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInst;

    HANDLE mtx = CreateMutexW(NULL, TRUE, L"Local\\RedstoneServerPanel_SingleInstance_v2");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND w = FindWindowW(L"RedstoneServerPanelWnd", NULL);
        if (w) PostMessageW(w, WM_SC_SHOW, 0, 0);
        if (mtx) CloseHandle(mtx);
        return 0;
    }

    InitializeCriticalSection(&g_csUuid);
    Gdiplus::GdiplusStartupInput gsi;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gsi, NULL);

    LoadAppPrefs();

    typedef BOOL(WINAPI * PFN_CTX)(HANDLE);
    HMODULE u32 = GetModuleHandleW(L"user32.dll");
    PFN_CTX setCtx = reinterpret_cast<PFN_CTX>(reinterpret_cast<void*>(GetProcAddress(u32, "SetProcessDpiAwarenessContext")));
    if (!(setCtx && setCtx((HANDLE)-2))) SetProcessDPIAware();
    HDC sdc = GetDC(NULL); g_dpi = GetDeviceCaps(sdc, LOGPIXELSX); ReleaseDC(NULL, sdc);

    g_msgTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainProc; wc.hInstance = hInst;
    wc.lpszClassName = L"RedstoneServerPanelWnd";
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    wc.hIconSm = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON,
                                   GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR | LR_SHARED);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    RECT r = { 0, 0, S(1040), S(700) + HeaderH() };
    AdjustWindowRect(&r, style, FALSE);
    RECT wa; SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int ww = r.right - r.left, wh = r.bottom - r.top;
    ww = std::min(ww, (int)(wa.right - wa.left)); wh = std::min(wh, (int)(wa.bottom - wa.top));
    int px = wa.left + ((wa.right - wa.left) - ww) / 2, py = wa.top + ((wa.bottom - wa.top) - wh) / 2;

    HWND hwnd = CreateWindowExW(0, L"RedstoneServerPanelWnd", L().wndTitle, style,
                                px, py, ww, wh, NULL, NULL, hInst, NULL);
    if (!hwnd) {
        if (g_gdiplusToken) Gdiplus::GdiplusShutdown(g_gdiplusToken);
        DeleteCriticalSection(&g_csUuid);
        if (mtx) { ReleaseMutex(mtx); CloseHandle(mtx); }
        return 1;
    }

    bool startHidden = wcsstr(GetCommandLineW(), L"--tray") != NULL || wcsstr(GetCommandLineW(), L"/tray") != NULL;
    if (startHidden) {
        g_trayHintShown = true;
        Balloon(L"Redstone Server Panel", L().balloonTrayStart);
    } else ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }

    for (HFONT f : { g_fUI, g_fSemi, g_fCap, g_fVal, g_fMono }) if (f) DeleteObject(f);
    for (HBRUSH b : { g_bBg, g_bField, g_bLog }) if (b) DeleteObject(b);
    if (g_gdiplusToken) Gdiplus::GdiplusShutdown(g_gdiplusToken);
    DeleteCriticalSection(&g_csUuid);
    if (mtx) { ReleaseMutex(mtx); CloseHandle(mtx); }
    return (int)msg.wParam;
}
