// Redstone Server Panel Web Demo Logic

const state = {
  lang: 'tr',
  theme: 'dark',
  page: 'panel',
  settingsSubTab: 'gen',
  isRunning: false,
  serverPhase: 'stopped', // 'stopped' | 'starting' | 'running'
  serverGen: 0,
  restartPending: false,
  restartTimer: null,
  uptimeSeconds: 0,
  uptimeInterval: null,
  idleTimer: null,
  simTimeouts: [],
  memoryMB: 13,
  cpuPct: 2.4,
  systemCpuPct: 2.4,
  maxPlayers: 20,
  ramHist: Array(40).fill(0.3),
  cpuHist: Array.from({ length: 40 }, (_, i) => +(2.1 + Math.sin(i * 0.45) * 0.6).toFixed(1)),
  consoleFilter: 'all',
  worldTimeTick: 6000,
  worldTimeUserSet: false,
  onlinePlayers: [],
  selectedPlayer: null,
  cmdHistory: [],
  cmdHistoryIdx: -1,
  jarIndex: 0,
  toastTimer: null,
  gamemode: 'survival',
  difficulty: 'normal',
  properties: {
    motd: 'A Minecraft Server - Powered by Redstone SP',
    maxPlayers: '20',
    spawnProt: '16',
    viewDist: '10',
    simDist: '10',
    gamemode: 'survival',
    difficulty: 'normal',
    hardcore: false,
    forceGm: false,
    online: true,
    pvp: true,
    whitelist: false,
    flight: false,
    cmdblock: false,
    secureProf: true,
    hidePlayers: false,
    levelName: 'world',
    levelSeed: '',
    levelType: 'default',
    nether: true,
    monsters: true,
    animals: true,
    npcs: true
  },
  appSettings: {
    autoStart: false,
    splashAnim: true,
    crashGuard: true,
    dailyRestart: false,
    dailyRestartTime: '04:00',
    jvmArgs: '-XX:+UseG1GC -XX:+ParallelRefProcEnabled',
    schedBk: true,
    schedBkHours: '2'
  }
};

const chatSequence = [
  {
    delay: 2400,
    player: 'Alex',
    tr: 'selam steve, madene inelim mi?',
    en: 'hey steve, ready to go mining?'
  },
  {
    delay: 2800,
    player: 'Steve',
    tr: 'olur, elmas kazmayı alıp geliyorum hemen',
    en: 'yeah, grabbing my diamond pickaxe right now'
  },
  {
    delay: 3600,
    type: 'advancement',
    player: 'Steve',
    tr: 'Steve has made the advancement [Diamonds!]',
    en: 'Steve has made the advancement [Diamonds!]'
  },
  {
    delay: 2200,
    player: 'Steve',
    tr: 'oh be sonunda elmas buldum :D',
    en: 'lets go finally found diamonds :D'
  },
  {
    delay: 3000,
    player: 'Alex',
    tr: 'helal! bana da 3 tane versene zırh basayım',
    en: 'nice! share some so I can craft a chestplate'
  },
  {
    delay: 3200,
    player: 'Steve',
    tr: 'tamamdır sandığa bıraktım kanka',
    en: 'sure thing, left some in the spawn chest'
  }
];

const i18n = {
  tr: {
    // Navbar
    navFeatures: "Özellikler",
    navProjects: "Diğer Projeler",
    navDownload: "⬇️ İndir (.exe)",

    // Hero
    heroBadge: "Saf Win32 C++17 • 100% Yerel • Sıfır Bağımlılık • ~10 MB RAM",
    heroTitle: "Windows İçin Modern Minecraft Sunucu Yöneticisi",
    heroDesc: "Electron veya hantal arayüzler yok. Tek dosyada derlenen ultra hafif saf Win32 mimarisi, modern sekmeli arayüz, gerçek zamanlı renkli konsol ve otomatik yedekleme sistemi.",
    heroBtnDownload: "⚡ Windows İçin İndir (.exe)",
    heroBtnGithub: "⭐ GitHub'da İncele",
    statStartup: "Saf Win32",
    statRam: "RAM Tüketimi",
    statThemes: "Özel Tema",
    statLang: "Çift Dil",

    // Desktop Window Title & Tabs
    wndTitle: "RedstoneSP",
    tabDashboard: "Panel",
    tabProperties: "Özellikler",
    tabAccess: "Yetkiler",
    tabBackups: "Yedekler",
    tabPlugins: "Eklenti & Mod",
    tabProfiles: "Profiller",
    tabSettings: "Ayarlar",
    
    // Page 1: Dashboard
    cardSettings: "SUNUCU AYARLARI",
    lblJar: "Sunucu .jar dosyası",
    btnBrowse: "Seç",
    lblIp: "IP adresi",
    lblPort: "Port",
    lblRam: "Bellek",
    btnStart: "Başlat",
    btnStop: "Güvenle Durdur",
    btnRestart: "Yeniden Başlat",
    btnFolder: "Klasör",
    filtAll: "Tümü",
    filtWarn: "Uyarı",
    filtChat: "Sohbet",
    btnConSave: "Kaydet",
    btnConClear: "Temizle",
    tileCpuSys: "SİSTEM CPU",
    tileCpuSrv: "SUNUCU CPU",
    cardPlayers: "OYUNCULAR • ",
    lblNoPlayers: "Çevrim içi oyuncu yok",
    btnKick: "At (Kick)",
    btnBan: "Banla",
    lblUnban: "Banı kaldır (oyuncu adı)",
    phUnban: "oyuncu_adi",
    btnUnban: "Banı Kaldır",
    cardConsole: "KONSOL",
    btnSend: "Gönder",
    inputCmdPlaceholder: "Komut girin (ör: help, say Selam, op Steve, time set day)...",
    
    // Telemetry Tiles
    tileStatus: "DURUM",
    tileUptime: "ÇALIŞMA SÜRESİ",
    tileMemory: "BELLEK",
    tileMemoryChart: "BELLEK (RAM)",
    tileCpuChart: "İŞLEMCİ (CPU)",
    tilePlayers: "OYUNCU",
    tileTime: "OYUN SAATİ",
    statusOffline: "KAPALI",
    statusOnline: "AKTİF",
    statusStarting: "BAŞLATILIYOR",

    // Page 2: Properties
    cardPropServer: "SUNUCU BİLGİLERİ",
    lblMotd: "Sunucu Açıklaması (MOTD)",
    lblMaxPlayers: "Maks. Oyuncu (max-players)",
    lblViewDist: "Görüş Mesafesi (chunk)",
    lblSimDist: "Simülasyon Mesafesi",
    lblSpawnProt: "Başlangıç Koruması (blok)",
    btnSaveProps: "Ayarları Kaydet",
    btnReload: "Yeniden Yükle",
    btnRefresh: "Yenile",
    descPropsNote: "Not: Bazı ayarların tam geçerli olması için sunucuyu yeniden başlatmanız gerekir.",
    cardGmDiff: "OYUN MODU & ZORLUK",
    lblDefGamemode: "Varsayılan Oyun Modu (gamemode)",
    lblDifficulty: "Oyun Zorluğu (difficulty)",
    gmSurvival: "Hayatta Kalma",
    gmCreative: "Yaratıcı",
    gmAdventure: "Macera",
    gmSpectator: "İzleyici",
    dfPeaceful: "Barışçıl",
    dfEasy: "Kolay",
    dfNormal: "Normal",
    dfHard: "Zor",
    chkHardcore: "Hardcore Modu (hardcore — tek yaşam)",
    chkForceGm: "Oyun Modunu Zorla (force-gamemode)",
    cardRules: "KURALLAR & GÜVENLİK",
    chkOnline: "Orijinal Hesap Zorunlu (online-mode)",
    chkPvp: "Oyuncular Arası Hasar (pvp)",
    chkWhitelist: "Whitelist Zorunlu (white-list)",
    chkFlight: "Uçmaya İzin Ver (allow-flight)",
    chkCmdBlock: "Komut Bloklarını Etkinleştir (enable-command-block)",
    chkSecureProf: "Güvenli Sohbet İmzası (enforce-secure-profile)",
    chkHidePlayers: "Oyuncu Listesini Gizle (hide-online-players)",
    cardWorld: "DÜNYA & VARLIKLAR",
    lblLevelName: "Dünya Adı (level-name)",
    lblLevelSeed: "Dünya Tohumu (level-seed)",
    lblLevelType: "Dünya Oluşturma Türü (level-type)",
    lvlNormal: "Normal",
    lvlFlat: "Düz",
    lvlLarge: "Geniş Biyom",
    lvlAmp: "Yükseltilmiş",
    chkNether: "Nether Boyutuna İzin Ver (allow-nether)",
    chkMonsters: "Canavar Doğumu (spawn-monsters)",
    chkAnimals: "Hayvan Doğumu (spawn-animals)",
    chkNpcs: "Köylü / NPC Doğumu (spawn-npcs)",

    // Page 3: Access
    cardWl: "WHITELIST (WHITELIST.JSON)",
    cardOps: "OPERATÖRLER (OPS.JSON)",
    phPlayerName: "Oyuncu adı...",
    btnAddWl: "Ekle",
    btnRemWl: "Seçileni Çıkar",
    btnOpAdd: "OP Ver",
    btnOpRem: "OP Al (Deop)",

    // Page 4: Backups
    cardBackupCreate: "DÜNYA YEDEKLEME",
    descBackups: "Dünya klasörlerini (world, nether, end) .zip olarak sunucu klasöründeki 'backups' dizinine yedekler. Sunucu açıksa önce otomatik \"save-all\" gönderilir.",
    lblBackupTag: "İsteğe bağlı etiket (ör: nether-oncesi)",
    btnNewBackup: "⚡ Dünyayı Yedekle (.zip)",
    btnOpenBkFolder: "📁 Yedek Klasörünü Aç",
    cardBackups: "KAYITLI YEDEKLER (BACKUPS\\)",
    btnRestoreBackup: "Geri Yükle",
    btnDeleteBackup: "Yedeği Sil",

    // Page 5: Plugins & Mods
    cardPlugins: "EKLENTİLER (PLUGINS\\)",
    cardMods: "MODLAR (MODS\\)",
    badgeEnabled: "Aktif",
    badgeDisabled: "Devre Dışı",
    btnTogglePlugin: "Aktif / Devre Dışı Bırak",
    btnAddItem: "Ekle",
    btnDelItem: "Sil",
    btnOpenFolder: "Aç",

    // Page 6: Profiles
    cardSaveProfile: "YENİ PROFİL KAYDET",
    descProfileNote: "Panel'deki .jar yolu, IP, port, RAM ve JVM argümanlarını bir profil olarak saklar. Farklı sunucular arasında tek tıkla geçiş yapabilirsiniz.",
    lblProfileName: "Profil Adı (ör: 1.20 Survival)",
    btnSaveProfile: "💾 Yeni Profil Olarak Kaydet",
    cardProfiles: "KAYITLI SUNUCU PROFİLLERİ",
    btnLoadProfile: "Seçili Profili Yükle",
    btnDeleteProfile: "Profili Sil",

    // Page 7: Settings
    subtabGen: "GENEL",
    subtabSrv: "SUNUCU & OTOMASYON",
    cardStartup: "BAŞLANGIÇ",
    cardGeneral: "GENEL AYARLAR",
    cardTheme: "TEMA SEÇİMİ",
    lblAutostart: "Windows ile birlikte başlat",
    lblSplashAnim: "Açılış animasyonunu göster",
    cardLang: "DİL SEÇİMİ (LANGUAGE)",
    themeDark: "Koyu Tema (Dark)",
    themeLight: "Açık Tema (White)",
    themeRedstone: "Redstone Teması",
    themeMinecraft: "Minecraft Teması",
    cardAutoRestart: "OTOMATİK YENİDEN BAŞLATMA",
    lblCrashGuard: "Çökme Koruması",
    descAutoRestart: "Sunucu beklenmedik şekilde kapanırsa 10 saniye sonra yeniden başlatılır. Art arda 3 çökmeden sonra vazgeçilir.",
    cardDailyRestart: "GÜNLÜK YENİDEN BAŞLATMA",
    lblDailyRestart: "Planlı Yeniden Başlatma",
    descDailyRestart: "Her gün belirtilen saatte (SS:DD) oyunculara 5 ve 1 dakika kala duyuru yaparak sunucuyu güvenle yeniden başlatır.",
    btnSave: "Kaydet",
    cardLogs: "LOGLAR",
    btnOpenLogs: "📁 Log Klasörünü Aç",
    descLogs: "Konsol çıktısı console.log dosyasına kaydedilir.",
    cardJvm: "JVM ARGÜMANLARI",
    descJvm: "Sunucu başlatılırken kullanılacak ek Java argümanları. Değişiklik bir sonraki başlatmada geçerli olur.",
    cardSchedBk: "OTOMATİK ZAMANLANMIŞ YEDEKLEME",
    lblSchedBk: "Otomatik Yedekleme",
    descSchedBk: "Sunucu çalışırken belirlenen saat aralığında (saat cinsinden) arka planda otomatik olarak dünya yedeği alır.",

    // Console logs & Notifications
    consoleReady: "» Redstone Server Panel hazır. .jar dosyasını seçip \"Başlat\"a basın.",
    consoleNotRunning: "» Sunucu çalışmıyor. Komut göndermek için önce \"Başlat\"a basın.",
    consoleStarting: "» [SUNUCU] 4G RAM ile başlatılıyor...",
    consoleStopping: "» [SUNUCU] Sunucu güvenle durduruldu.",
    logUnban: "» [GÜVENLİK] Oyuncunun banı kaldırıldı: ",
    logKick: "» [YÖNETİM] Oyuncu sunucudan atıldı (Kick): ",
    logBan: "» [YÖNETİM] Oyuncu sunucudan yasaklandı (Ban): ",
    logBackupDone: "» [YEDEK] Dünya yedeği başarıyla oluşturuldu: backups/",
    logProfileSaved: "» [PROFİL] Yeni profil başarıyla kaydedildi: ",
    logProfileLoaded: "» [PROFİL] Sunucu profili yüklendi: ",
    logPropsSaved: "» [AYAR] server.properties dosyası kaydedildi.",
    logPropsReloaded: "» [AYAR] server.properties diskten yeniden yüklendi.",
    logSettingsSaved: "» [AYAR] Otomasyon ve sunucu ayarları güncellendi.",
    logListReloaded: "» [SİSTEM] Liste güncellendi.",
    logFolderOpened: "» [SİSTEM] Klasör dosya gezgininde açıldı.",
    logJarSelected: "» [AYAR] Sunucu JAR seçildi: ",
    logItemAdded: "» [YÖNETİM] Eklendi: ",
    logItemRemoved: "» [YÖNETİM] Kaldırıldı: ",
    logBackupRestored: "» [YEDEK] Seçili dünya yedeği geri yüklendi.",
    logBackupDeleted: "» [YEDEK] Yedek kalıcı olarak silindi.",
    logProfileDeleted: "» [PROFİL] Seçili profil silindi.",
    toastSaved: "Değişiklikler kaydedildi",
    toastPropsReloaded: "server.properties yeniden yüklendi",
    toastUpdated: "Liste güncellendi",
    toastFolderOpened: "Klasör açıldı",
    errInvalidRam: "Geçersiz bellek formatı (ör: 4G, 2048M)",
    errInvalidPort: "Port 1-65535 arasında olmalıdır",
    errInvalidTime: "Saat SS:DD formatında olmalıdır (ör: 04:00)",
    errInvalidHours: "Yedekleme aralığı 1-72 saat olmalıdır",
    errInvalidJvm: "JVM argümanlarında tırnak (\") kullanılamaz",
    errServerMustStop: "Bu işlem için önce sunucuyu durdurun",
    errEmptyName: "Lütfen geçerli bir oyuncu adı girin",
    errEmptyProfile: "Lütfen bir profil adı girin",
    errDuplicateItem: "Bu öğe zaten listede mevcut",
    errEmptyList: "Listede seçili öğe yok",
    consoleRestarting: "» [SUNUCU] Sunucu yeniden başlatılıyor...",

    // Features Section
    featTag: "Neden Redstone Server Panel?",
    featMainTitle: "Saf Win32 Mimarisi ve Üstün Performans",
    feat1Title: "Sıfır Hantallık, ~10 MB RAM",
    feat1Desc: "Electron, Node.js veya WebView2 kütüphanelerine ihtiyaç duymaz. Tek dosyalık saf C++17 kodu ile hafif ve akıcı çalışır, minimum bellek tüketir.",
    feat2Title: "7 Modern Sekmeli Arayüz",
    feat2Desc: "Dashboard, server.properties düzenleyici, Whitelist & Ops, otomatik yedekleme, eklenti/mod yöneticisi, çoklu sunucu profilleri ve ayarlar.",
    feat3Title: "Dört Özelleştirilmiş Tema",
    feat3Desc: "Koyu (Dark), Açık (Light), Netherite kırmızısı Redstone ve zümrüt yeşili Minecraft temaları tek tıkla canlı olarak değişir.",
    feat4Title: "Yerel Dünya Yedekleme",
    feat4Desc: "Windows'un dahili tar.exe aracı ile dünya klasörlerini .zip olarak sıkıştırır, otomatik \"save-all\" göndererek veri kaybını önler.",
    feat5Title: "Akıllı Otomasyon & Çökme Koruması",
    feat5Desc: "Sunucu çökerse 10 saniye içinde otomatik yeniden başlatılır. Günlük planlı yeniden başlatmalarda oyun içine 5 ve 1 dakika kala geri sayım anonsu geçer.",
    feat6Title: "Yüksek DPI & Sistem Tepsisi",
    feat6Desc: "4K ve yüksek ölçeklendirmeli monitörlerde bulanıklaşmaz. Arka planda çalışırken sistem tepsisine (Tray) küçülür ve durum balonu gösterir.",

    // Other Projects Section
    projTag: "Geliştiriciden Diğer Projeler",
    projMainTitle: "@qu3zel Tarafından Geliştirilen Diğer Araçlar",
    projMoriaSDesc: "Doors of Durin temalı, Windows sistem tepsisinde çalışan şık ve minimalist uyku / kapatma zamanlayıcısı.",
    projScopyDesc: "Discord sunucuları ve otomasyon süreçleri için geliştirilmiş çok amaçlı bot projesi.",
    projProfileTitle: "Tüm Depolar & Profil",
    projProfileDesc: "GitHub üzerindeki açık kaynak projelerimi, geliştirici araçlarını ve gelecekteki geliştirmeleri takip edin.",
    projViewGithub: "GitHub'da Gör →",
    projVisitProfile: "Profili Ziyaret Et →",

    // Footer
    footerLicensed: "Açık kaynaklı",
    footerLicenseLink: "MIT Lisansı",
    footerLicensedEnd: "ile korunmaktadır.",
    footerDev: "Geliştirici:",
    footerRepo: "GitHub Deposu",
    footerDownloadLatest: "Son Sürümü İndir"
  },
  en: {
    // Navbar
    navFeatures: "Features",
    navProjects: "Other Projects",
    navDownload: "⬇️ Download (.exe)",

    // Hero
    heroBadge: "Pure Native Win32 C++17 • 100% Native • Zero Dependencies • ~10 MB RAM",
    heroTitle: "Modern Minecraft Server Manager for Windows",
    heroDesc: "No Electron, no webview bloat. Single-file ultra-lightweight native Win32 architecture, modern tabbed interface, real-time colorized console, and automated backups.",
    heroBtnDownload: "⚡ Download for Windows (.exe)",
    heroBtnGithub: "⭐ View on GitHub",
    statStartup: "Pure Win32",
    statRam: "RAM Usage",
    statThemes: "Custom Themes",
    statLang: "Bilingual",

    // Desktop Window Title & Tabs
    wndTitle: "RedstoneSP",
    tabDashboard: "Dashboard",
    tabProperties: "Properties",
    tabAccess: "Access",
    tabBackups: "Backups",
    tabPlugins: "Plugins & Mods",
    tabProfiles: "Profiles",
    tabSettings: "Settings",
    
    // Page 1: Dashboard
    cardSettings: "SERVER SETTINGS",
    lblJar: "Server .jar file",
    btnBrowse: "Browse",
    lblIp: "IP address",
    lblPort: "Port",
    lblRam: "Memory",
    btnStart: "Start",
    btnStop: "Stop Safely",
    btnRestart: "Restart",
    btnFolder: "Folder",
    filtAll: "All",
    filtWarn: "Warn",
    filtChat: "Chat",
    btnConSave: "Save",
    btnConClear: "Clear",
    tileCpuSys: "SYSTEM CPU",
    tileCpuSrv: "SERVER CPU",
    cardPlayers: "PLAYERS • ",
    lblNoPlayers: "No players online",
    btnKick: "Kick",
    btnBan: "Ban",
    lblUnban: "Unban player (player name)",
    phUnban: "player_name",
    btnUnban: "Unban",
    cardConsole: "CONSOLE",
    btnSend: "Send",
    inputCmdPlaceholder: "Enter command (e.g. help, say Hello, op Steve, time set day)...",
    
    // Telemetry Tiles
    tileStatus: "STATUS",
    tileUptime: "UPTIME",
    tileMemory: "MEMORY",
    tileMemoryChart: "MEMORY (RAM)",
    tileCpuChart: "CPU LOAD",
    tilePlayers: "PLAYERS",
    tileTime: "WORLD TIME",
    statusOffline: "OFFLINE",
    statusOnline: "ACTIVE",
    statusStarting: "STARTING",

    // Page 2: Properties
    cardPropServer: "SERVER INFO",
    lblMotd: "Server Description (MOTD)",
    lblMaxPlayers: "Max Players (max-players)",
    lblViewDist: "View Distance (chunks)",
    lblSimDist: "Simulation Distance",
    lblSpawnProt: "Spawn Protection (blocks)",
    btnSaveProps: "Save Properties",
    btnReload: "Reload",
    btnRefresh: "Refresh",
    descPropsNote: "Note: The server must be restarted for some settings to take full effect.",
    cardGmDiff: "GAMEMODE & DIFFICULTY",
    lblDefGamemode: "Default Game Mode (gamemode)",
    lblDifficulty: "World Difficulty (difficulty)",
    gmSurvival: "Survival",
    gmCreative: "Creative",
    gmAdventure: "Adventure",
    gmSpectator: "Spectator",
    dfPeaceful: "Peaceful",
    dfEasy: "Easy",
    dfNormal: "Normal",
    dfHard: "Hard",
    chkHardcore: "Hardcore Mode (hardcore)",
    chkForceGm: "Force Gamemode (force-gamemode)",
    cardRules: "RULES & SECURITY",
    chkOnline: "Require Premium Account (online-mode)",
    chkPvp: "Player vs Player Damage (pvp)",
    chkWhitelist: "Enforce Whitelist (white-list)",
    chkFlight: "Allow Flight (allow-flight)",
    chkCmdBlock: "Enable Command Blocks (enable-command-block)",
    chkSecureProf: "Secure Profile (enforce-secure-profile)",
    chkHidePlayers: "Hide Online Players (hide-online-players)",
    cardWorld: "WORLD & ENTITIES",
    lblLevelName: "World Name (level-name)",
    lblLevelSeed: "World Seed (level-seed)",
    lblLevelType: "World Generation Type (level-type)",
    lvlNormal: "Normal",
    lvlFlat: "Flat",
    lvlLarge: "Large Biomes",
    lvlAmp: "Amplified",
    chkNether: "Allow Nether (allow-nether)",
    chkMonsters: "Spawn Monsters (spawn-monsters)",
    chkAnimals: "Spawn Animals (spawn-animals)",
    chkNpcs: "Spawn Villagers (spawn-npcs)",

    // Page 3: Access
    cardWl: "WHITELIST (WHITELIST.JSON)",
    cardOps: "OPERATORS (OPS.JSON)",
    phPlayerName: "Player name...",
    btnAddWl: "Add",
    btnRemWl: "Remove Selected",
    btnOpAdd: "Make OP",
    btnOpRem: "Remove OP (Deop)",

    // Page 4: Backups
    cardBackupCreate: "WORLD BACKUPS",
    descBackups: "Compresses world folders (world, nether, end) into .zip inside 'backups' directory. Automatically sends 'save-all' beforehand if server is running.",
    lblBackupTag: "Optional tag (e.g. pre-nether)",
    btnNewBackup: "⚡ Backup World (.zip)",
    btnOpenBkFolder: "📁 Open Backups Folder",
    cardBackups: "SAVED BACKUPS (BACKUPS\\)",
    btnRestoreBackup: "Restore",
    btnDeleteBackup: "Delete Backup",

    // Page 5: Plugins & Mods
    cardPlugins: "PLUGINS (PLUGINS\\)",
    cardMods: "MODS (MODS\\)",
    badgeEnabled: "Active",
    badgeDisabled: "Disabled",
    btnTogglePlugin: "Enable / Disable Selected",
    btnAddItem: "Add",
    btnDelItem: "Delete",
    btnOpenFolder: "Open",

    // Page 6: Profiles
    cardSaveProfile: "SAVE NEW PROFILE",
    descProfileNote: "Saves current .jar path, IP, port, RAM, and JVM arguments as a named profile for instant switching.",
    lblProfileName: "Profile Name (e.g. 1.20 Survival)",
    btnSaveProfile: "💾 Save as New Profile",
    cardProfiles: "SAVED SERVER PROFILES",
    btnLoadProfile: "Load Selected Profile",
    btnDeleteProfile: "Delete Profile",

    // Page 7: Settings
    subtabGen: "GENERAL",
    subtabSrv: "SERVER & AUTOMATION",
    cardStartup: "STARTUP",
    cardGeneral: "GENERAL SETTINGS",
    cardTheme: "THEME SELECTION",
    lblAutostart: "Start with Windows",
    lblSplashAnim: "Show startup splash animation",
    cardLang: "LANGUAGE SELECTION",
    themeDark: "Dark Theme (Default)",
    themeLight: "Light Theme (White)",
    themeRedstone: "Redstone Theme",
    themeMinecraft: "Minecraft Theme",
    cardAutoRestart: "AUTOMATIC RESTART",
    lblCrashGuard: "Crash Guard",
    descAutoRestart: "Automatically restarts the server 10 seconds after an unexpected crash. Aborts after 3 consecutive crashes.",
    cardDailyRestart: "SCHEDULED DAILY RESTART",
    lblDailyRestart: "Daily Auto-Restart",
    descDailyRestart: "Safely restarts the server daily at the specified time (HH:MM), broadcasting in-game 5m and 1m warnings.",
    btnSave: "Save",
    cardLogs: "LOGS",
    btnOpenLogs: "📁 Open Logs Folder",
    descLogs: "Console output is automatically saved to console.log.",
    cardJvm: "JVM ARGUMENTS",
    descJvm: "Additional Java arguments used when starting the server. Applied on next server launch.",
    cardSchedBk: "SCHEDULED AUTO-BACKUP",
    lblSchedBk: "Scheduled Backup",
    descSchedBk: "Automatically takes background world backups at specified hour intervals while the server is running.",

    // Console logs & Notifications
    consoleReady: "» Redstone Server Panel is ready. Select a .jar file and click \"Start\".",
    consoleNotRunning: "» Server is not running. Press \"Start\" first to send commands.",
    consoleStarting: "» [SERVER] Starting server with 4G RAM...",
    consoleStopping: "» [SERVER] Server stopped safely.",
    logUnban: "» [SECURITY] Player was unbanned: ",
    logKick: "» [MODERATION] Player kicked: ",
    logBan: "» [MODERATION] Player banned: ",
    logBackupDone: "» [BACKUP] World backup created successfully: backups/",
    logProfileSaved: "» [PROFILE] Profile saved successfully: ",
    logProfileLoaded: "» [PROFILE] Loaded profile: ",
    logPropsSaved: "» [PROPERTIES] server.properties has been saved.",
    logPropsReloaded: "» [PROPERTIES] server.properties reloaded from disk.",
    logSettingsSaved: "» [SETTINGS] Automation and server settings updated.",
    logListReloaded: "» [SYSTEM] List refreshed.",
    logFolderOpened: "» [SYSTEM] Opened folder in File Explorer.",
    logJarSelected: "» [SETTINGS] Selected server JAR: ",
    logItemAdded: "» [MANAGER] Added: ",
    logItemRemoved: "» [MANAGER] Removed: ",
    logBackupRestored: "» [BACKUP] Selected world backup restored.",
    logBackupDeleted: "» [BACKUP] Backup permanently deleted.",
    logProfileDeleted: "» [PROFILE] Selected profile deleted.",
    toastSaved: "Changes saved",
    toastPropsReloaded: "server.properties reloaded",
    toastUpdated: "List refreshed",
    toastFolderOpened: "Folder opened",
    errInvalidRam: "Invalid memory format (e.g. 4G, 2048M)",
    errInvalidPort: "Port must be between 1 and 65535",
    errInvalidTime: "Time must be in HH:MM format (e.g. 04:00)",
    errInvalidHours: "Backup interval must be 1-72 hours",
    errInvalidJvm: "JVM arguments cannot contain quotes (\")",
    errServerMustStop: "Stop the server before performing this action",
    errEmptyName: "Please enter a valid player name",
    errEmptyProfile: "Please enter a profile name",
    errDuplicateItem: "This item is already in the list",
    errEmptyList: "No item selected in list",
    consoleRestarting: "» [SERVER] Restarting server...",

    // Features Section
    featTag: "Why Redstone Server Panel?",
    featMainTitle: "Pure Win32 Architecture & Extreme Performance",
    feat1Title: "Zero Bloat, ~10 MB RAM",
    feat1Desc: "No Electron, Node.js, or WebView2 required. Pure C++17 single-file architecture runs natively with minimal memory footprint.",
    feat2Title: "7 Modern Tabs",
    feat2Desc: "Dashboard, server.properties editor, Whitelist & Ops, world backups, plugin/mod manager, multi-server profiles, and settings.",
    feat3Title: "Four Hand-Crafted Themes",
    feat3Desc: "Dark, Light, deep crimson Redstone, and emerald Minecraft themes change seamlessly in real-time.",
    feat4Title: "Native World Compression",
    feat4Desc: "Creates .zip backups using Windows built-in tar.exe, sending 'save-all flush' beforehand to ensure data integrity.",
    feat5Title: "Smart Automation & Crash Guard",
    feat5Desc: "Restarts automatically within 10 seconds if crashed. Scheduled daily restarts broadcast in-game 5-minute and 1-minute countdowns.",
    feat6Title: "High-DPI & System Tray",
    feat6Desc: "Pixel-crisp on 4K and scaled monitors. Minimizes quietly to the system tray with live status notifications.",

    // Other Projects Section
    projTag: "More Projects by Developer",
    projMainTitle: "Other Open-Source Tools Crafted by @qu3zel",
    projMoriaSDesc: "Doors of Durin themed, minimalist Windows system tray sleep & shutdown timer.",
    projScopyDesc: "Multi-purpose bot project for Discord servers and automation workflows.",
    projProfileTitle: "All Repositories & Profile",
    projProfileDesc: "Explore my open-source projects, developer tools, and future releases on GitHub.",
    projViewGithub: "View on GitHub →",
    projVisitProfile: "Visit Profile →",

    // Footer
    footerLicensed: "Open source protected under",
    footerLicenseLink: "MIT License",
    footerLicensedEnd: ".",
    footerDev: "Developer:",
    footerRepo: "GitHub Repo",
    footerDownloadLatest: "Download Latest"
  }
};

// Selectors
const winWindow = document.getElementById('winWindow');
const winTabbar = document.getElementById('winTabbar');
const tabIndicator = document.getElementById('tabIndicator');
const settingsSubtabs = document.getElementById('settingsSubtabs');
const subtabIndicator = document.getElementById('subtabIndicator');
const winTitleText = document.getElementById('winTitleText');
const subbarTitle = document.getElementById('subbarTitle');
const consoleBody = document.getElementById('consoleBody');
const consoleInput = document.getElementById('consoleInput');
const btnStartStop = document.getElementById('btnStartStop');
const btnRestartServer = document.getElementById('btnRestartServer');
const btnOpenSrvFolder = document.getElementById('btnOpenSrvFolder');
const chartCpuTitle = document.getElementById('chartCpuTitle');
const cpuScaleLabel = document.getElementById('cpuScaleLabel');
const btnConSave = document.getElementById('btnConSave');
const btnConClear = document.getElementById('btnConClear');
const tileStatus = document.getElementById('tileStatus');
const tileUptime = document.getElementById('tileUptime');
const tileMemory = document.getElementById('tileMemory');
const tileCpu = document.getElementById('tileCpu');
const tilePlayers = document.getElementById('tilePlayers');
const tileTime = document.getElementById('tileTime');
const inputJar = document.getElementById('inputJar');
const btnBrowse = document.getElementById('btnBrowse');
const inputIp = document.getElementById('inputIp');
const inputPort = document.getElementById('inputPort');
const inputRam = document.getElementById('inputRam');
const inputMaxPlayers = document.getElementById('inputMaxPlayers');
const simToast = document.getElementById('simToast');
const langPillLabel = document.getElementById('langPillLabel');

function parseRamStrict(raw) {
  const s = (raw || '').trim().toUpperCase();
  const m = s.match(/^(\d+)\s*([GM])$/);
  if (!m) return null;
  const v = parseInt(m[1], 10);
  if (!Number.isFinite(v) || v <= 0) return null;
  const mb = m[2] === 'G' ? v * 1024 : v;
  return (mb >= 256 && mb <= 262144) ? mb : null;
}

function parsePortStrict(raw) {
  const s = (raw || '').trim();
  if (!/^\d+$/.test(s)) return null;
  const v = parseInt(s, 10);
  return (v >= 1 && v <= 65535) ? v : null;
}

function isValidPlayerName(name) {
  return /^[a-zA-Z0-9_.*-]{1,32}$/.test((name || '').trim());
}

function isValidHHMM(raw) {
  return /^([01]\d|2[0-3]):[0-5]\d$/.test((raw || '').trim());
}

function syncDashboardControlLocks() {
  const stopped = state.serverPhase === 'stopped' && !state.restartPending;
  const running = state.serverPhase === 'running' && !state.restartPending;
  if (btnBrowse) btnBrowse.disabled = !stopped;
  if (inputIp) inputIp.disabled = !stopped;
  if (inputPort) inputPort.disabled = !stopped;
  if (inputRam) inputRam.disabled = !stopped;
  if (btnConSave) btnConSave.disabled = !running;
  if (btnStartStop) btnStartStop.disabled = state.restartPending;
  if (btnRestartServer) btnRestartServer.disabled = !running;
}

function showToast(msg) {
  if (!simToast || !msg) return;
  simToast.textContent = msg;
  simToast.classList.add('show');
  if (state.toastTimer) clearTimeout(state.toastTimer);
  state.toastTimer = setTimeout(() => {
    simToast.classList.remove('show');
  }, 2400);
}

function formatWorldTime(ticks) {
  const norm = ((ticks % 24000) + 24000) % 24000;
  const totalMinutes = Math.floor((norm / 1000) * 60 + 360) % 1440;
  const h = Math.floor(totalMinutes / 60);
  const m = totalMinutes % 60;
  return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}`;
}

function parseRamLimitMb() {
  const raw = (inputRam && inputRam.value) ? inputRam.value.trim() : '4G';
  return parseRamStrict(raw) || 4096;
}

function parseMaxPlayers() {
  const v = parseInt(state.properties.maxPlayers, 10);
  return (Number.isFinite(v) && v >= 1 && v <= 99999) ? v : 20;
}

function getTelemetryColor(pct, kind) {
  const isLight = state.theme === 'light';
  if (pct >= 90) return isLight ? '#dc2626' : '#ef4444';
  if (pct >= 75) return isLight ? '#d97706' : '#f59e0b';
  if (kind === 'ram') return isLight ? '#0284c7' : '#38bdf8';
  return isLight ? '#16a34a' : '#4ade80';
}

function renderSparklineSvg(prefix, hist, color, autoScale = false) {
  const svg = document.getElementById(`svg${prefix}Chart`);
  const line = document.getElementById(`${prefix.toLowerCase()}LinePath`);
  const area = document.getElementById(`${prefix.toLowerCase()}AreaPath`);
  const dot = document.getElementById(`${prefix.toLowerCase()}LiveDot`);
  const halo = document.getElementById(`${prefix.toLowerCase()}LiveHalo`);
  const stopTop = document.getElementById(`grad${prefix}Top`);
  const stopBot = document.getElementById(`grad${prefix}Bot`);
  if (!svg || !line || !area || !hist || hist.length < 2) return;

  svg.style.color = color;
  if (stopTop) stopTop.setAttribute('stop-color', color);
  if (stopBot) stopBot.setAttribute('stop-color', color);

  const yMax = autoScale ? Math.min(100, Math.max(20, Math.max(...hist) * 1.25)) : 100;
  if (autoScale && hist === state.cpuHist && cpuScaleLabel) {
    cpuScaleLabel.textContent = `0-${Math.round(yMax)}%`;
  }
  const W = 260, H = 54, padX = 4, padY = 5;
  const usableW = W - padX * 2;
  const usableH = H - padY * 2;
  const n = hist.length;
  const pts = hist.map((val, i) => {
    const clamped = Math.max(0, Math.min(yMax, val));
    const x = padX + (i / (n - 1)) * usableW;
    const y = (H - padY) - (clamped / yMax) * usableH;
    return [x, y];
  });

  const lineD = pts.map(([x, y], i) => `${i === 0 ? 'M' : 'L'} ${x.toFixed(1)} ${y.toFixed(1)}`).join(' ');
  const last = pts[pts.length - 1];
  const first = pts[0];
  const areaD = `${lineD} L ${last[0].toFixed(1)} ${H} L ${first[0].toFixed(1)} ${H} Z`;

  line.setAttribute('d', lineD);
  area.setAttribute('d', areaD);
  if (dot) {
    dot.setAttribute('cx', last[0].toFixed(1));
    dot.setAttribute('cy', last[1].toFixed(1));
  }
  if (halo) {
    halo.setAttribute('cx', last[0].toFixed(1));
    halo.setAttribute('cy', last[1].toFixed(1));
  }
}

function updateTelemetryVisuals(pushSample = false) {
  const isLight = state.theme === 'light';
  const maxMb = parseRamLimitMb();
  const ramPct = Math.max(0, Math.min(100, (state.memoryMB / maxMb) * 100));
  const cpuPct = Math.max(0, Math.min(100, state.cpuPct));

  if (pushSample) {
    state.ramHist.push(ramPct);
    if (state.ramHist.length > 40) state.ramHist.shift();
    state.cpuHist.push(cpuPct);
    if (state.cpuHist.length > 40) state.cpuHist.shift();
  }

  const ramColor = getTelemetryColor(ramPct, 'ram');
  const cpuColor = getTelemetryColor(cpuPct, 'cpu');

  if (tileMemory) tileMemory.textContent = `${Math.round(state.memoryMB)} MB • ${Math.round(ramPct)}%`;
  if (tileCpu) tileCpu.textContent = `${cpuPct.toFixed(1)}%`;

  const dotRam = document.getElementById('dotRam');
  const barRam = document.getElementById('barRam');
  if (dotRam) dotRam.style.backgroundColor = ramColor;
  if (barRam) {
    barRam.style.width = `${Math.max(ramPct > 0 ? 1.5 : 0, ramPct).toFixed(1)}%`;
    barRam.style.backgroundColor = ramColor;
  }

  const dotCpu = document.getElementById('dotCpu');
  const barCpu = document.getElementById('barCpu');
  if (dotCpu) dotCpu.style.backgroundColor = cpuColor;
  if (barCpu) {
    barCpu.style.width = `${Math.max(cpuPct > 0 ? 1.5 : 0, cpuPct).toFixed(1)}%`;
    barCpu.style.backgroundColor = cpuColor;
  }

  renderSparklineSvg('Ram', state.ramHist, ramColor, false);
  renderSparklineSvg('Cpu', state.cpuHist, cpuColor, true);

  // KPI 1: Status
  const dotStatus = document.getElementById('dotStatus');
  const barStatus = document.getElementById('barStatus');
  const isLive = state.serverPhase === 'running';
  const statusCol = state.isRunning
    ? (isLive ? 'var(--w-green)' : 'var(--w-amber)')
    : 'var(--w-red)';
  if (dotStatus) dotStatus.style.backgroundColor = statusCol;
  if (barStatus) {
    barStatus.style.backgroundColor = statusCol;
    barStatus.style.width = state.isRunning ? (isLive ? '100%' : '62%') : '35%';
  }

  // KPI 2: Uptime
  const dotUptime = document.getElementById('dotUptime');
  const barUptime = document.getElementById('barUptime');
  if (dotUptime) dotUptime.style.backgroundColor = isLive ? 'var(--w-green)' : 'var(--w-muted)';
  if (barUptime) {
    barUptime.style.backgroundColor = 'var(--w-accent)';
    barUptime.style.width = isLive ? '100%' : '0%';
  }

  // KPI 3: Players (online / max)
  state.maxPlayers = parseMaxPlayers();
  const plCount = state.onlinePlayers.length;
  const plPct = Math.min(100, (plCount / Math.max(1, state.maxPlayers)) * 100);
  const plCol = plPct >= 95 ? 'var(--w-red)' : (plPct >= 75 ? 'var(--w-amber)' : 'var(--w-green)');
  if (tilePlayers) tilePlayers.textContent = `${plCount} / ${state.maxPlayers}`;
  const dotPlayers = document.getElementById('dotPlayers');
  const barPlayers = document.getElementById('barPlayers');
  if (dotPlayers) dotPlayers.style.backgroundColor = plCount > 0 ? plCol : 'var(--w-muted)';
  if (barPlayers) {
    barPlayers.style.backgroundColor = plCol;
    barPlayers.style.width = `${plPct.toFixed(1)}%`;
  }

  // KPI 4: World Time (day gold sun / night indigo moon + 24h cycle bar)
  const dotTime = document.getElementById('dotTime');
  const barTime = document.getElementById('barTime');
  const hasWorld = isLive;
  if (hasWorld) {
    const normTick = ((state.worldTimeTick % 24000) + 24000) % 24000;
    const totalMinutes = Math.floor((normTick / 1000) * 60 + 360) % 1440;
    const hour = Math.floor(totalMinutes / 60);
    const isDay = hour >= 6 && hour < 18;
    const wtCol = isDay
      ? (isLight ? '#d97706' : '#fbbf24')
      : (isLight ? '#4f46e5' : '#818cf8');
    const cyclePct = (totalMinutes / 1440) * 100;
    if (dotTime) dotTime.style.backgroundColor = wtCol;
    if (barTime) {
      barTime.style.backgroundColor = wtCol;
      barTime.style.width = `${cyclePct.toFixed(1)}%`;
    }
  } else {
    if (dotTime) dotTime.style.backgroundColor = 'var(--w-muted)';
    if (barTime) barTime.style.width = '0%';
  }
}

function selectListItem(list, item) {
  if (!list || !item) return;
  list.querySelectorAll('.player-item').forEach(it => it.classList.remove('selected'));
  item.classList.add('selected');
}

// Simulation timers
function clearSimTimeouts() {
  state.simTimeouts.forEach(id => clearTimeout(id));
  state.simTimeouts = [];
}

function scheduleSim(fn, delay) {
  const gen = state.serverGen;
  const id = setTimeout(() => {
    state.simTimeouts = state.simTimeouts.filter(tId => tId !== id);
    if (!state.isRunning || state.serverGen !== gen) return;
    fn();
  }, delay);
  state.simTimeouts.push(id);
  return id;
}

// Append log line
function matchesConsoleFilter(lineText, logType, filter) {
  if (filter === 'all') return true;
  if (filter === 'warn') return logType === 'warn' || logType === 'err';
  if (filter === 'chat') return logType === 'chat' || (logType === 'sys' && lineText.includes('advancement'));
  return true;
}

function applyConsoleFilter() {
  if (!consoleBody) return;
  const children = consoleBody.children;
  for (let i = 0; i < children.length; i++) {
    const el = children[i];
    const logType = el.dataset.logType || (el.className.match(/console-line-(\w+)/)?.[1] || 'info');
    const show = matchesConsoleFilter(el.textContent || '', logType, state.consoleFilter);
    el.classList.toggle('is-hidden', !show);
  }
  consoleBody.scrollTop = consoleBody.scrollHeight;
}

function setConsoleFilter(filter) {
  state.consoleFilter = filter || 'all';
  document.querySelectorAll('.con-pill[data-filter]').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.filter === state.consoleFilter);
  });
  applyConsoleFilter();
}

function appendLog(line, type = '') {
  const logType = type || 'info';
  const div = document.createElement('div');
  div.className = type ? `console-line console-line-${type}` : 'console-line';
  div.dataset.logType = logType;
  div.textContent = line;
  if (!matchesConsoleFilter(line, logType, state.consoleFilter)) {
    div.classList.add('is-hidden');
  }
  consoleBody.appendChild(div);
  while (consoleBody.children.length > 500) {
    consoleBody.firstElementChild.remove();
  }
  if (!div.classList.contains('is-hidden')) {
    consoleBody.scrollTop = consoleBody.scrollHeight;
  }
}

// Format time
function formatUptime(sec) {
  const h = String(Math.floor(sec / 3600)).padStart(2, '0');
  const m = String(Math.floor((sec % 3600) / 60)).padStart(2, '0');
  const s = String(sec % 60).padStart(2, '0');
  return `${h}:${m}:${s}`;
}

// Monitor-Hz VSync animation engine (requestAnimationFrame adapts automatically to 60/120/144/240Hz+)
const reducedMotionMq = typeof window !== 'undefined' && window.matchMedia
  ? window.matchMedia('(prefers-reduced-motion: reduce)')
  : null;

const navAnim = {
  rafId: 0,
  lastTs: 0,
  reducedMotion: reducedMotionMq ? reducedMotionMq.matches : false,
  tab: { curX: -1, curW: 0, targetX: 0, targetW: 0, active: false },
  sub: { curX: -1, curW: 0, targetX: 0, targetW: 0, y: 0, h: 0, active: false }
};

if (reducedMotionMq) {
  const onMotionChange = (e) => {
    navAnim.reducedMotion = e.matches;
    if (e.matches) {
      syncTabIndicator(false);
      if (state.page === 'settings') syncSubtabIndicator(false);
    }
  };
  if (typeof reducedMotionMq.addEventListener === 'function') {
    reducedMotionMq.addEventListener('change', onMotionChange);
  } else if (typeof reducedMotionMq.addListener === 'function') {
    reducedMotionMq.addListener(onMotionChange);
  }
}

function applyTabIndicatorDom() {
  if (!tabIndicator || navAnim.tab.curX < 0) return;
  tabIndicator.style.transform = `translate3d(${navAnim.tab.curX.toFixed(2)}px, 0, 0)`;
  tabIndicator.style.width = `${Math.max(0, navAnim.tab.curW).toFixed(2)}px`;
}

function applySubtabIndicatorDom() {
  if (!subtabIndicator || navAnim.sub.curX < 0) return;
  subtabIndicator.style.transform = `translate3d(${navAnim.sub.curX.toFixed(2)}px, ${navAnim.sub.y.toFixed(2)}px, 0)`;
  subtabIndicator.style.width = `${Math.max(0, navAnim.sub.curW).toFixed(2)}px`;
  subtabIndicator.style.height = `${Math.max(0, navAnim.sub.h).toFixed(2)}px`;
}

function stepNavAnimations(ts) {
  const dt = navAnim.lastTs ? Math.min(0.05, Math.max(0.001, (ts - navAnim.lastTs) / 1000)) : 0.016;
  navAnim.lastTs = ts;
  const alpha = 1 - Math.exp(-24 * dt);
  let keepGoing = false;

  if (navAnim.tab.active) {
    const dx = navAnim.tab.targetX - navAnim.tab.curX;
    const dw = navAnim.tab.targetW - navAnim.tab.curW;
    if (Math.abs(dx) < 0.35 && Math.abs(dw) < 0.35) {
      navAnim.tab.curX = navAnim.tab.targetX;
      navAnim.tab.curW = navAnim.tab.targetW;
      navAnim.tab.active = false;
    } else {
      navAnim.tab.curX += dx * alpha;
      navAnim.tab.curW += dw * alpha;
      keepGoing = true;
    }
    applyTabIndicatorDom();
  }

  if (navAnim.sub.active) {
    const sx = navAnim.sub.targetX - navAnim.sub.curX;
    const sw = navAnim.sub.targetW - navAnim.sub.curW;
    if (Math.abs(sx) < 0.35 && Math.abs(sw) < 0.35) {
      navAnim.sub.curX = navAnim.sub.targetX;
      navAnim.sub.curW = navAnim.sub.targetW;
      navAnim.sub.active = false;
    } else {
      navAnim.sub.curX += sx * alpha;
      navAnim.sub.curW += sw * alpha;
      keepGoing = true;
    }
    applySubtabIndicatorDom();
  }

  if (keepGoing) {
    navAnim.rafId = requestAnimationFrame(stepNavAnimations);
  } else {
    navAnim.rafId = 0;
    navAnim.lastTs = 0;
  }
}

function ensureNavAnimRunning() {
  if (!navAnim.rafId) {
    navAnim.lastTs = 0;
    navAnim.rafId = requestAnimationFrame(stepNavAnimations);
  }
}

function syncTabIndicator(animate = true) {
  if (!tabIndicator) return;
  const activeTab = document.querySelector('.win-tab.active');
  if (!activeTab) return;
  const tx = activeTab.offsetLeft;
  const tw = activeTab.offsetWidth;
  if (tw <= 0) return;

  navAnim.tab.targetX = tx;
  navAnim.tab.targetW = tw;

  if (!animate || navAnim.reducedMotion || navAnim.tab.curX < 0) {
    navAnim.tab.curX = tx;
    navAnim.tab.curW = tw;
    navAnim.tab.active = false;
    applyTabIndicatorDom();
  } else {
    navAnim.tab.active = true;
    ensureNavAnimRunning();
  }
}

function syncSubtabIndicator(animate = true) {
  if (!subtabIndicator) return;
  const activeBtn = document.querySelector('.subtab-btn.active');
  if (!activeBtn) return;
  const bx = activeBtn.offsetLeft;
  const bw = activeBtn.offsetWidth;
  const by = activeBtn.offsetTop;
  const bh = activeBtn.offsetHeight;
  if (bw <= 0 || bh <= 0) return;

  navAnim.sub.targetX = bx;
  navAnim.sub.targetW = bw;
  navAnim.sub.y = by;
  navAnim.sub.h = bh;

  if (!animate || navAnim.reducedMotion || navAnim.sub.curX < 0) {
    navAnim.sub.curX = bx;
    navAnim.sub.curW = bw;
    navAnim.sub.active = false;
    applySubtabIndicatorDom();
  } else {
    navAnim.sub.active = true;
    ensureNavAnimRunning();
  }
}

// Tab navigation
document.querySelectorAll('.win-tab').forEach(tab => {
  tab.addEventListener('click', () => {
    const targetPage = tab.dataset.page;
    setPage(targetPage);
  });
});

function setPage(pageName) {
  const pageChanged = state.page !== pageName;
  state.page = pageName;
  document.querySelectorAll('.win-tab').forEach(t => {
    t.classList.toggle('active', t.dataset.page === pageName);
  });
  document.querySelectorAll('.tab-page').forEach(p => {
    p.classList.toggle('active', p.id === `page-${pageName}`);
  });
  const t = i18n[state.lang];
  const tabNames = {
    panel: t.tabDashboard,
    props: t.tabProperties,
    access: t.tabAccess,
    backups: t.tabBackups,
    plugins: t.tabPlugins,
    profiles: t.tabProfiles,
    settings: t.tabSettings
  };
  subbarTitle.textContent = (tabNames[pageName] || pageName).toUpperCase();

  syncTabIndicator(pageChanged);
  if (pageChanged && winTabbar) {
    const activeTab = document.querySelector('.win-tab.active');
    if (activeTab && typeof winTabbar.scrollTo === 'function') {
      const barRect = winTabbar.getBoundingClientRect();
      const tabRect = activeTab.getBoundingClientRect();
      const pad = 12;
      let targetLeft = null;
      if (tabRect.left < barRect.left + pad) {
        targetLeft = winTabbar.scrollLeft + (tabRect.left - barRect.left) - pad;
      } else if (tabRect.right > barRect.right - pad) {
        targetLeft = winTabbar.scrollLeft + (tabRect.right - barRect.right) + pad;
      }
      if (targetLeft !== null) {
        winTabbar.scrollTo({
          left: Math.max(0, targetLeft),
          behavior: navAnim.reducedMotion ? 'auto' : 'smooth'
        });
      }
    }
  }
  if (pageName === 'settings') {
    syncSubtabIndicator(false);
  } else {
    navAnim.sub.active = false;
  }
}

// Settings Sub-Tabs
function setSettingsSubTab(subtab) {
  const changed = state.settingsSubTab !== subtab;
  state.settingsSubTab = subtab;
  document.querySelectorAll('.subtab-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.subtab === subtab);
  });
  const subGen = document.getElementById('subpage-gen');
  const subSrv = document.getElementById('subpage-srv');
  if (subGen) subGen.style.display = '';
  if (subSrv) subSrv.style.display = '';
  if (subtab === 'srv') {
    if (subGen) subGen.classList.remove('active');
    if (subSrv) subSrv.classList.add('active');
  } else {
    if (subGen) subGen.classList.add('active');
    if (subSrv) subSrv.classList.remove('active');
  }
  syncSubtabIndicator(changed && state.page === 'settings');
}
window.setSettingsSubTab = setSettingsSubTab;

if (typeof ResizeObserver !== 'undefined') {
  const handleNavResize = () => {
    const activeTab = document.querySelector('.win-tab.active');
    if (activeTab && activeTab.offsetWidth > 0 && !navAnim.tab.active) {
      if (Math.abs(navAnim.tab.targetX - activeTab.offsetLeft) > 0.5 ||
          Math.abs(navAnim.tab.targetW - activeTab.offsetWidth) > 0.5) {
        syncTabIndicator(false);
      }
    }
    if (state.page === 'settings') {
      const activeSub = document.querySelector('.subtab-btn.active');
      if (activeSub && activeSub.offsetWidth > 0 && !navAnim.sub.active) {
        if (Math.abs(navAnim.sub.targetX - activeSub.offsetLeft) > 0.5 ||
            Math.abs(navAnim.sub.targetW - activeSub.offsetWidth) > 0.5) {
          syncSubtabIndicator(false);
        }
      }
    }
  };
  const navResizeObs = new ResizeObserver(handleNavResize);
  if (winTabbar) navResizeObs.observe(winTabbar);
  if (settingsSubtabs) navResizeObs.observe(settingsSubtabs);
} else if (typeof window !== 'undefined') {
  window.addEventListener('resize', () => {
    if (!navAnim.tab.active) syncTabIndicator(false);
    if (state.page === 'settings' && !navAnim.sub.active) syncSubtabIndicator(false);
  });
}

if (typeof window !== 'undefined') {
  window.addEventListener('load', () => {
    syncTabIndicator(false);
    if (state.page === 'settings') syncSubtabIndicator(false);
  });
  if (document.fonts && document.fonts.ready) {
    document.fonts.ready.then(() => {
      syncTabIndicator(false);
      if (state.page === 'settings') syncSubtabIndicator(false);
    });
  }
}

// Theme change
function setTheme(themeName) {
  state.theme = themeName;
  winWindow.className = `win-window theme-${themeName}`;
  document.querySelectorAll('.theme-card-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.theme === themeName);
  });
  document.querySelectorAll('.sim-btn-pill[data-theme]').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.theme === themeName);
  });
  updateTelemetryVisuals(false);
}
window.setTheme = setTheme;

// Toggle language
function toggleLanguage() {
  setLanguage(state.lang === 'tr' ? 'en' : 'tr');
}
window.toggleLanguage = toggleLanguage;

// Dashboard Player List Rendering
function renderDashboardPlayers() {
  const container = document.getElementById('dashboardPlayerList');
  const countEl = document.getElementById('playerCount');
  if (!container) return;

  const count = state.onlinePlayers.length;
  if (countEl) countEl.textContent = String(count);
  updateTelemetryVisuals(false);

  if (count === 0) {
    const t = i18n[state.lang];
    container.innerHTML = `<div class="player-empty" data-i18n="lblNoPlayers">${t.lblNoPlayers || 'Çevrim içi oyuncu yok'}</div>`;
    state.selectedPlayer = null;
    return;
  }

  container.innerHTML = '';
  state.onlinePlayers.forEach((p, idx) => {
    const div = document.createElement('div');
    const isSel = (state.selectedPlayer === p.name) || (!state.selectedPlayer && idx === 0);
    if (isSel) state.selectedPlayer = p.name;
    div.className = `player-item${isSel ? ' selected' : ''}`;
    div.dataset.player = p.name;
    const skinHead = document.createElement('div');
    skinHead.className = 'skin-head';
    skinHead.style.background = p.color || '#4ade80';
    const nameEl = document.createElement('strong');
    nameEl.textContent = p.name;
    div.appendChild(skinHead);
    div.appendChild(nameEl);
    div.addEventListener('click', () => {
      container.querySelectorAll('.player-item').forEach(it => it.classList.remove('selected'));
      div.classList.add('selected');
      state.selectedPlayer = p.name;
    });
    container.appendChild(div);
  });
}

// Language change
function setLanguage(langCode) {
  state.lang = langCode;
  document.documentElement.lang = langCode;
  const t = i18n[langCode];
  
  if (langPillLabel) {
    langPillLabel.textContent = (langCode === 'tr') ? 'English' : 'Türkçe';
  }

  // Update all [data-i18n] texts across the entire page & simulator
  document.querySelectorAll('[data-i18n]').forEach(el => {
    const key = el.dataset.i18n;
    if (t[key]) el.textContent = t[key];
  });

  // Update input placeholders
  document.querySelectorAll('[data-i18n-ph]').forEach(el => {
    const key = el.dataset.i18nPh;
    if (t[key]) el.placeholder = t[key];
  });

  // Update Settings tab language buttons
  document.querySelectorAll('.lang-btn').forEach(b => {
    b.classList.toggle('active', b.dataset.lang === langCode);
  });

  // Update Start/Stop button & telemetry tiles
  if (chartCpuTitle) {
    const cpuKey = state.isRunning ? 'tileCpuSrv' : 'tileCpuSys';
    chartCpuTitle.dataset.i18n = cpuKey;
    chartCpuTitle.textContent = t[cpuKey];
  }
  const isLive = state.serverPhase === 'running';
  if (state.isRunning) {
    btnStartStop.textContent = t.btnStop;
    tileStatus.textContent = isLive ? t.statusOnline : t.statusStarting;
    tileStatus.className = isLive ? 'tile-val tile-status active' : 'tile-val tile-status starting';
    if (tileTime && isLive) tileTime.textContent = formatWorldTime(state.worldTimeTick);
  } else {
    btnStartStop.textContent = t.btnStart;
    tileStatus.textContent = t.statusOffline;
    tileStatus.className = 'tile-val tile-status';
    if (tileTime) tileTime.textContent = '--:--';
  }
  syncDashboardControlLocks();

  // Update initial console greeting if only 1 line exists
  const initialMsg = document.getElementById('consoleInitialMsg');
  if (initialMsg && consoleBody.children.length === 1) {
    initialMsg.textContent = t.consoleReady;
  }

  renderDashboardPlayers();
  setPage(state.page);
  syncTabIndicator(false);
  if (state.page === 'settings') syncSubtabIndicator(false);
}
window.setLanguage = setLanguage;

function launchServerWithUi() {
  const t = i18n[state.lang];
  if (inputRam && !parseRamStrict(inputRam.value)) {
    showToast(t.errInvalidRam);
    appendLog(`» ${t.errInvalidRam}`, 'err');
    return false;
  }
  if (inputPort && !parsePortStrict(inputPort.value)) {
    showToast(t.errInvalidPort);
    appendLog(`» ${t.errInvalidPort}`, 'err');
    return false;
  }
  if (state.restartTimer) {
    clearTimeout(state.restartTimer);
    state.restartTimer = null;
  }
  state.restartPending = false;
  startServer();
  syncDashboardControlLocks();
  if (chartCpuTitle && state.isRunning) {
    chartCpuTitle.dataset.i18n = 'tileCpuSrv';
    chartCpuTitle.textContent = t.tileCpuSrv;
  }
  return true;
}

function restartServer() {
  if (state.serverPhase !== 'running' || state.restartPending) return;
  const t = i18n[state.lang];
  if (state.restartTimer) clearTimeout(state.restartTimer);
  state.restartPending = true;
  appendLog(t.consoleRestarting, 'warn');
  stopServer(true);
  state.restartTimer = setTimeout(() => {
    state.restartTimer = null;
    state.restartPending = false;
    launchServerWithUi();
  }, 750);
}

// Start / Stop Server Simulator
btnStartStop.addEventListener('click', () => {
  if (state.restartPending) return;
  if (state.isRunning) {
    stopServer();
  } else {
    launchServerWithUi();
  }
});

if (btnRestartServer) {
  btnRestartServer.addEventListener('click', restartServer);
}

if (btnOpenSrvFolder) {
  btnOpenSrvFolder.addEventListener('click', () => {
    const t = i18n[state.lang];
    appendLog(t.logFolderOpened, 'info');
    showToast(t.toastFolderOpened);
  });
}

document.querySelectorAll('.con-pill[data-filter]').forEach(btn => {
  btn.addEventListener('click', () => setConsoleFilter(btn.dataset.filter));
});

if (btnConClear) {
  btnConClear.addEventListener('click', () => {
    const t = i18n[state.lang];
    consoleBody.innerHTML = '';
    const initDiv = document.createElement('div');
    initDiv.id = 'consoleInitialMsg';
    initDiv.dataset.i18n = 'consoleReady';
    initDiv.dataset.logType = 'sys';
    initDiv.className = 'console-line console-line-sys';
    initDiv.textContent = t.consoleReady;
    consoleBody.appendChild(initDiv);
    applyConsoleFilter();
  });
}

if (btnConSave) {
  btnConSave.addEventListener('click', () => {
    const t = i18n[state.lang];
    if (state.serverPhase !== 'running') {
      appendLog(t.consoleNotRunning, 'warn');
      showToast(t.consoleNotRunning.replace('» ', ''));
      return;
    }
    appendLog('> save-all', 'cmd');
    appendLog(`[${now()}] [Server thread/INFO]: Saved the game`, 'sys');
    showToast(t.toastSaved);
  });
}

function startIdleCpuHeartbeat() {
  if (state.idleTimer) clearInterval(state.idleTimer);
  state.idleTimer = setInterval(() => {
    if (state.isRunning || state.restartPending) return;
    const wave = Math.sin(Date.now() / 2400) * 0.8 + Math.random() * 1.4;
    state.systemCpuPct = +Math.max(0.8, Math.min(8.5, 1.9 + wave)).toFixed(1);
    state.cpuPct = state.systemCpuPct;
    if (typeof updateTelemetryVisuals === 'function') {
      updateTelemetryVisuals(true);
    }
  }, 1000);
}

startIdleCpuHeartbeat();
setTimeout(() => {
  if (typeof updateTelemetryVisuals === 'function') {
    updateTelemetryVisuals(false);
  }
}, 0);

function startServer() {
  state.serverGen++;
  clearSimTimeouts();
  clearInterval(state.uptimeInterval);
  state.isRunning = true;
  state.serverPhase = 'starting';
  state.uptimeSeconds = 0;
  state.onlinePlayers = [];
  state.selectedPlayer = null;
  state.worldTimeTick = 6000;
  state.worldTimeUserSet = false;
  renderDashboardPlayers();
  syncDashboardControlLocks();

  const t = i18n[state.lang];
  const ramVal = (inputRam && inputRam.value.trim()) ? inputRam.value.trim() : '4G';
  const portVal = (inputPort && inputPort.value.trim()) ? inputPort.value.trim() : '25565';
  const maxMb = parseRamLimitMb();
  state.ramHist = Array(40).fill(0.3);
  state.cpuHist = Array(40).fill(0.2);
  tileStatus.textContent = t.statusStarting;
  tileStatus.className = 'tile-val tile-status starting';
  btnStartStop.textContent = t.btnStop;
  btnStartStop.className = 'w-btn w-btn-stop';
  if (tileTime) tileTime.textContent = '--:--';
  state.memoryMB = Math.round(Math.min(maxMb * 0.42, 1680));
  state.cpuPct = 52.4;
  updateTelemetryVisuals(true);

  appendLog(t.consoleStarting.replace('4G', ramVal), 'sys');
  appendLog(`[${now()}] [Server thread/INFO]: Loading libraries, please wait...`, 'info');

  scheduleSim(() => {
    state.memoryMB = Math.round(Math.min(maxMb * 0.62, 2480));
    state.cpuPct = 74.8;
    updateTelemetryVisuals(true);
    appendLog(`[${now()}] [Server thread/INFO]: Starting minecraft server version 1.20.4`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Loading properties`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Default game type: ${(state.properties.gamemode || 'survival').toUpperCase()}`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Generating keypair`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Starting Minecraft server on *:${portVal}`, 'info');
  }, 600);

  scheduleSim(() => {
    state.serverPhase = 'running';
    syncDashboardControlLocks();
    const curLangT = i18n[state.lang];
    appendLog(`[${now()}] [Server thread/INFO]: Preparing level "${state.properties.levelName || 'world'}"`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Preparing start region for dimension minecraft:overworld`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Time elapsed: 1420 ms`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Done (2.104s)! For help, type "help"`, 'sys');

    tileStatus.textContent = curLangT.statusOnline;
    tileStatus.className = 'tile-val tile-status active';
    const baseRamMb = Math.min(Math.round(maxMb * 0.68), 2820);
    state.memoryMB = baseRamMb;
    state.cpuPct = 18.6;
    if (!state.worldTimeUserSet) state.worldTimeTick = 6000;
    if (tileTime) tileTime.textContent = formatWorldTime(state.worldTimeTick);
    updateTelemetryVisuals(true);

    state.uptimeSeconds = 0;
    state.uptimeInterval = setInterval(() => {
      state.uptimeSeconds++;
      tileUptime.textContent = formatUptime(state.uptimeSeconds);
      if (state.uptimeSeconds % 2 === 0 && tileTime) {
        state.worldTimeTick = (state.worldTimeTick + 200) % 24000;
        tileTime.textContent = formatWorldTime(state.worldTimeTick);
      }

      // Realistic JVM heap sawtooth + live CPU fluctuation based on online players
      const curMaxMb = parseRamLimitMb();
      const targetLow = Math.round(curMaxMb * 0.60);
      const targetHigh = Math.round(curMaxMb * 0.78);
      if (state.memoryMB >= targetHigh) {
        state.memoryMB = targetLow + Math.round(Math.random() * 90);
      } else {
        state.memoryMB = Math.min(targetHigh, state.memoryMB + 10 + Math.round(Math.random() * 14));
      }

      const playerLoad = state.onlinePlayers.length * 5.2;
      const baseCpu = 11.5 + playerLoad + (Math.random() * 7.5 - 3.2);
      state.cpuPct = Math.max(3.2, Math.min(96.0, baseCpu));
      updateTelemetryVisuals(true);
    }, 1000);
  }, 1500);

  // Player 1 joins (Alex)
  scheduleSim(() => {
    appendLog(`[${now()}] [User Authenticator #1/INFO]: UUID of player Alex is c06f8906-4c3e-4b47-a4dd-24d1a55eb19e`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Alex[local] logged in with entity id 142 at (-42.5, 68.0, 112.3)`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Alex joined the game`, 'sys');

    state.onlinePlayers.push({ name: 'Alex', color: '#4ade80' });
    state.cpuPct = Math.min(95, state.cpuPct + 12.5);
    renderDashboardPlayers();
  }, 3200);

  // Player 2 joins (Steve)
  scheduleSim(() => {
    appendLog(`[${now()}] [User Authenticator #2/INFO]: UUID of player Steve is 853c80ef-3c37-49fd-aa49-938b674adae6`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Steve[local] logged in with entity id 189 at (-38.2, 69.0, 105.1)`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Steve joined the game`, 'sys');

    state.onlinePlayers.push({ name: 'Steve', color: '#38bdf8' });
    state.cpuPct = Math.min(95, state.cpuPct + 11.0);
    renderDashboardPlayers();

    // Start in-game chat conversation
    startChatConversation();
  }, 5200);
}

function startChatConversation() {
  let accumDelay = 0;
  chatSequence.forEach((step, idx) => {
    accumDelay += step.delay;
    scheduleSim(() => {
      // Check if player is still online
      const isOnline = state.onlinePlayers.some(p => p.name === step.player);
      if (!isOnline && step.type !== 'advancement') return;

      if (step.type === 'advancement') {
        appendLog(`[${now()}] [Server thread/INFO]: ${step[state.lang] || step.en}`, 'sys');
      } else {
        const text = step[state.lang] || step.en;
        appendLog(`[${now()}] [Async Chat Thread - #${idx % 2}/INFO]: <${step.player}> ${text}`, 'chat');
      }
    }, accumDelay);
  });
}

function stopServer(keepRestartPending = false) {
  if (!keepRestartPending && state.restartTimer) {
    clearTimeout(state.restartTimer);
    state.restartTimer = null;
    state.restartPending = false;
  }
  state.serverGen++;
  state.isRunning = false;
  state.serverPhase = 'stopped';
  clearSimTimeouts();
  clearInterval(state.uptimeInterval);
  state.uptimeSeconds = 0;

  const t = i18n[state.lang];
  btnStartStop.textContent = t.btnStart;
  btnStartStop.className = 'w-btn w-btn-start';
  if (chartCpuTitle) {
    chartCpuTitle.dataset.i18n = 'tileCpuSys';
    chartCpuTitle.textContent = t.tileCpuSys;
  }
  tileStatus.textContent = t.statusOffline;
  tileStatus.className = 'tile-val tile-status';
  state.memoryMB = 13;
  state.cpuPct = state.systemCpuPct || 2.4;
  tileUptime.textContent = '00:00:00';
  if (tileTime) tileTime.textContent = '--:--';
  syncDashboardControlLocks();
  updateTelemetryVisuals(true);

  if (state.onlinePlayers.length > 0) {
    state.onlinePlayers.forEach(p => {
      appendLog(`[${now()}] [Server thread/INFO]: ${p.name} lost connection: Server closed`, 'warn');
      appendLog(`[${now()}] [Server thread/INFO]: ${p.name} left the game`, 'sys');
    });
  }
  state.onlinePlayers = [];
  state.selectedPlayer = null;
  renderDashboardPlayers();

  appendLog(`[${now()}] [Server thread/INFO]: Stopping the server`, 'warn');
  appendLog(`[${now()}] [Server thread/INFO]: Saving players`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: Saving worlds`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: Saving chunks for level 'ServerLevel[${state.properties.levelName || 'world'}]'/minecraft:overworld`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: ThreadedAnvilChunkStorage: All dimensions are saved`, 'info');
  appendLog(t.consoleStopping, 'sys');
}

// Console command handler
const cmdSuggestions = ['help', 'list', 'say ', 'op ', 'deop ', 'whitelist ', 'time set day', 'time set night', 'gamemode ', 'stop'];

function sendCommand() {
  const rawCmd = consoleInput.value.trim();
  const cleanCmd = rawCmd.startsWith('/') ? rawCmd.slice(1).trim() : rawCmd;
  if (!cleanCmd) {
    consoleInput.value = '';
    return;
  }
  consoleInput.value = '';
  state.cmdHistory.push(rawCmd);
  state.cmdHistoryIdx = state.cmdHistory.length;

  appendLog(`> ${cleanCmd}`, 'cmd');

  const t = i18n[state.lang];
  const parts = cleanCmd.split(/\s+/);
  const base = parts[0].toLowerCase();

  if (state.serverPhase !== 'running' && !(base === 'stop' && state.isRunning)) {
    appendLog(t.consoleNotRunning, 'warn');
    return;
  }

  scheduleSim(() => {
    switch (base) {
      case 'help':
        appendLog(`[${now()}] [Server thread/INFO]: --- Showing help page 1 of 1 ---`, 'info');
        appendLog(`[${now()}] [Server thread/INFO]: /help, /list, /say <message>, /stop, /op <player>, /deop <player>, /whitelist <on|off|list|add|remove>, /time set <day|night>, /save-all`, 'info');
        break;
      case 'list': {
        const names = state.onlinePlayers.map(p => p.name).join(', ');
        appendLog(`[${now()}] [Server thread/INFO]: There are ${state.onlinePlayers.length} of a max of ${state.maxPlayers} players online${names ? ': ' + names : ''}`, 'info');
        break;
      }
      case 'say': {
        const msg = parts.slice(1).join(' ').trim();
        if (!msg) {
          appendLog(`[${now()}] [Server thread/INFO]: Unknown or incomplete command. Usage: /say <message>`, 'warn');
          break;
        }
        appendLog(`[${now()}] [Server thread/INFO]: [Server] ${msg}`, 'chat');
        break;
      }
      case 'op': {
        const pl = (parts[1] || '').trim();
        if (!isValidPlayerName(pl)) {
          appendLog(`[${now()}] [Server thread/INFO]: Unknown or incomplete command. Usage: /op <player>`, 'warn');
          break;
        }
        addAccessListItem('listOps', pl, true);
        appendLog(`[${now()}] [Server thread/INFO]: Made ${pl} a server operator`, 'sys');
        break;
      }
      case 'deop': {
        const pl = (parts[1] || '').trim();
        if (!isValidPlayerName(pl)) {
          appendLog(`[${now()}] [Server thread/INFO]: Unknown or incomplete command. Usage: /deop <player>`, 'warn');
          break;
        }
        const opsList = document.getElementById('listOps');
        if (opsList) {
          const targetLower = pl.toLowerCase();
          opsList.querySelectorAll('.player-item').forEach(it => {
            const span = it.querySelector('span');
            const txt = (span ? span.textContent : it.textContent).trim().toLowerCase();
            if (txt === targetLower || txt.startsWith(targetLower + ' ') || txt.startsWith(targetLower + '(')) {
              it.remove();
            }
          });
          if (!opsList.querySelector('.player-item.selected')) {
            const first = opsList.querySelector('.player-item');
            if (first) first.classList.add('selected');
          }
        }
        appendLog(`[${now()}] [Server thread/INFO]: Made ${pl} no longer a server operator`, 'warn');
        break;
      }
      case 'whitelist': {
        const sub = (parts[1] || '').toLowerCase();
        const targetPl = (parts[2] || '').trim();
        if (sub === 'on') {
          state.properties.whitelist = true;
          syncChkBox('chkWhitelistRow', true);
          appendLog(`[${now()}] [Server thread/INFO]: Whitelist is now turned on`, 'sys');
        } else if (sub === 'off') {
          state.properties.whitelist = false;
          syncChkBox('chkWhitelistRow', false);
          appendLog(`[${now()}] [Server thread/INFO]: Whitelist is now turned off`, 'warn');
        } else if (sub === 'list') {
          const wlItems = Array.from(document.querySelectorAll('#listWhitelist .player-item span')).map(s => s.textContent.trim());
          appendLog(`[${now()}] [Server thread/INFO]: There are ${wlItems.length} whitelisted player(s)${wlItems.length ? ': ' + wlItems.join(', ') : ''}`, 'info');
        } else if (sub === 'add' && isValidPlayerName(targetPl)) {
          addAccessListItem('listWhitelist', targetPl, false);
          appendLog(`[${now()}] [Server thread/INFO]: Added ${targetPl} to the whitelist`, 'sys');
        } else if (sub === 'remove' && isValidPlayerName(targetPl)) {
          const wlList = document.getElementById('listWhitelist');
          if (wlList) {
            wlList.querySelectorAll('.player-item').forEach(it => {
              const span = it.querySelector('span');
              if (span && span.textContent.trim().toLowerCase() === targetPl.toLowerCase()) it.remove();
            });
            if (!wlList.querySelector('.player-item.selected')) {
              const first = wlList.querySelector('.player-item');
              if (first) first.classList.add('selected');
            }
          }
          appendLog(`[${now()}] [Server thread/INFO]: Removed ${targetPl} from the whitelist`, 'warn');
        } else if (sub === 'reload') {
          appendLog(`[${now()}] [Server thread/INFO]: Reloaded the whitelist`, 'info');
        } else {
          appendLog(`[${now()}] [Server thread/INFO]: Unknown or incomplete command. Usage: /whitelist <on|off|list|add|remove|reload>`, 'warn');
        }
        break;
      }
      case 'time': {
        const sub = (parts[1] || 'query').toLowerCase();
        const arg = (parts[2] || 'day').toLowerCase();
        if (sub === 'set') {
          const preset = { day: 1000, noon: 6000, night: 13000, midnight: 18000 };
          const parsed = Number.isFinite(Number(arg)) ? Math.max(0, parseInt(arg, 10) % 24000) : preset[arg];
          state.worldTimeTick = Number.isFinite(parsed) ? parsed : 1000;
          state.worldTimeUserSet = true;
          if (tileTime) tileTime.textContent = formatWorldTime(state.worldTimeTick);
          updateTelemetryVisuals(false);
          appendLog(`[${now()}] [Server thread/INFO]: Set the time to ${state.worldTimeTick}`, 'info');
        } else if (sub === 'add') {
          const delta = parseInt(parts[2] || '0', 10) || 0;
          state.worldTimeTick = (((state.worldTimeTick + delta) % 24000) + 24000) % 24000;
          state.worldTimeUserSet = true;
          if (tileTime) tileTime.textContent = formatWorldTime(state.worldTimeTick);
          updateTelemetryVisuals(false);
          appendLog(`[${now()}] [Server thread/INFO]: Set the time to ${state.worldTimeTick}`, 'info');
        } else {
          appendLog(`[${now()}] [Server thread/INFO]: The time is ${state.worldTimeTick}`, 'info');
        }
        break;
      }
      case 'save-all': {
        appendLog(`[${now()}] [Server thread/INFO]: Saved the game`, 'sys');
        break;
      }
      case 'gamemode':
        appendLog(`[${now()}] [Server thread/INFO]: Set game mode to ${parts[1] || 'creative'}`, 'info');
        break;
      case 'stop':
        stopServer();
        break;
      default:
        appendLog(`[${now()}] [Server thread/INFO]: Unknown or incomplete command. Type "help" for list of commands.`, 'warn');
        break;
    }
  }, 200);
}

document.getElementById('btnConsoleSend').addEventListener('click', sendCommand);
consoleInput.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') {
    sendCommand();
  } else if (e.key === 'ArrowUp') {
    if (!state.cmdHistory.length) return;
    e.preventDefault();
    state.cmdHistoryIdx = Math.max(0, state.cmdHistoryIdx - 1);
    consoleInput.value = state.cmdHistory[state.cmdHistoryIdx] || '';
  } else if (e.key === 'ArrowDown') {
    if (!state.cmdHistory.length) return;
    e.preventDefault();
    state.cmdHistoryIdx = Math.min(state.cmdHistory.length, state.cmdHistoryIdx + 1);
    consoleInput.value = state.cmdHistoryIdx < state.cmdHistory.length ? state.cmdHistory[state.cmdHistoryIdx] : '';
  } else if (e.key === 'Tab') {
    e.preventDefault();
    const val = consoleInput.value.trimStart();
    if (!val) {
      consoleInput.value = cmdSuggestions[0];
      return;
    }
    const clean = val.startsWith('/') ? val.slice(1) : val;
    const prefix = val.startsWith('/') ? '/' : '';
    const spaceIdx = clean.indexOf(' ');
    if (spaceIdx === -1) {
      const match = cmdSuggestions.find(c => c.startsWith(clean.toLowerCase()));
      if (match) consoleInput.value = prefix + match;
    } else {
      const cmdHead = clean.slice(0, spaceIdx + 1);
      const argPart = clean.slice(spaceIdx + 1).toLowerCase();
      const playerMatch = state.onlinePlayers.find(p => p.name.toLowerCase().startsWith(argPart));
      if (playerMatch) consoleInput.value = prefix + cmdHead + playerMatch.name;
    }
  }
});

// Browse .jar simulator
const sampleJars = [
  'C:\\Minecraft\\paper-1.20.4.jar',
  'C:\\Minecraft\\purpur-1.20.4.jar',
  'C:\\Minecraft\\fabric-server-mc1.20.4.jar'
];
if (btnBrowse && inputJar) {
  btnBrowse.addEventListener('click', () => {
    const t = i18n[state.lang];
    if (state.serverPhase !== 'stopped') {
      showToast(t.errServerMustStop);
      return;
    }
    state.jarIndex = (state.jarIndex + 1) % sampleJars.length;
    inputJar.value = sampleJars[state.jarIndex];
    appendLog(`${t.logJarSelected}${inputJar.value}`, 'info');
    showToast(inputJar.value.split('\\').pop());
  });
}

// Unban button interaction
const btnUnban = document.getElementById('btnUnban');
const inputUnban = document.getElementById('inputUnban');
if (btnUnban && inputUnban) {
  btnUnban.addEventListener('click', () => {
    const t = i18n[state.lang];
    const val = inputUnban.value.trim();
    if (!isValidPlayerName(val)) {
      showToast(t.errEmptyName);
      return;
    }
    if (state.serverPhase !== 'running') {
      appendLog(t.consoleNotRunning, 'warn');
      showToast(t.consoleNotRunning.replace('» ', ''));
      return;
    }
    appendLog(`[${now()}] [Server thread/INFO]: Unbanned player ${val}`, 'sys');
    showToast(`${t.logUnban.replace('» ', '')}${val}`);
    inputUnban.value = '';
  });
}

// Kick & Ban simulator
const btnKickPlayer = document.getElementById('btnKickPlayer');
if (btnKickPlayer) {
  btnKickPlayer.addEventListener('click', () => {
    if (state.serverPhase !== 'running' || state.onlinePlayers.length === 0) return;
    const target = state.selectedPlayer || state.onlinePlayers[0].name;
    state.onlinePlayers = state.onlinePlayers.filter(p => p.name !== target);
    appendLog(`[${now()}] [Server thread/INFO]: Kicked ${target} from the game: "Kicked by an operator"`, 'warn');
    appendLog(`[${now()}] [Server thread/INFO]: ${target} left the game`, 'sys');
    state.selectedPlayer = state.onlinePlayers.length ? state.onlinePlayers[0].name : null;
    renderDashboardPlayers();
  });
}

const btnBanPlayer = document.getElementById('btnBanPlayer');
if (btnBanPlayer) {
  btnBanPlayer.addEventListener('click', () => {
    if (state.serverPhase !== 'running' || state.onlinePlayers.length === 0) return;
    const target = state.selectedPlayer || state.onlinePlayers[0].name;
    state.onlinePlayers = state.onlinePlayers.filter(p => p.name !== target);
    appendLog(`[${now()}] [Server thread/INFO]: Banned ${target}: "Banned by an operator"`, 'sys');
    appendLog(`[${now()}] [Server thread/INFO]: ${target} left the game`, 'sys');
    state.selectedPlayer = state.onlinePlayers.length ? state.onlinePlayers[0].name : null;
    renderDashboardPlayers();
  });
}

// Whitelist & Ops dynamic helpers
function addAccessListItem(listId, name, isOp) {
  const list = document.getElementById(listId);
  if (!list) return 'missing';
  const label = isOp ? `${name} (Level 4 OP)` : name;
  const existing = Array.from(list.querySelectorAll('.player-item')).find(el => {
    const span = el.querySelector('span');
    const txt = (span ? span.textContent : el.textContent).trim().toLowerCase();
    return txt === name.toLowerCase() || txt.startsWith(`${name.toLowerCase()} (`);
  });
  if (existing) {
    selectListItem(list, existing);
    return 'duplicate';
  }
  const item = document.createElement('div');
  item.className = 'player-item';
  const head = document.createElement('div');
  head.className = 'skin-head';
  if (isOp) head.style.background = '#ef4444';
  const span = document.createElement('span');
  span.textContent = label;
  item.appendChild(head);
  item.appendChild(span);
  list.appendChild(item);
  selectListItem(list, item);
  return 'added';
}

function removeSelectedAccessItem(listId) {
  const list = document.getElementById(listId);
  if (!list) return null;
  const sel = list.querySelector('.player-item.selected') || list.querySelector('.player-item');
  if (!sel) return null;
  const span = sel.querySelector('span');
  const raw = (span ? span.textContent : sel.textContent).trim();
  const cleanName = raw.replace(/\s*\(Level\s+\d+\s+OP\)$/i, '').trim();
  sel.remove();
  const next = list.querySelector('.player-item');
  if (next) next.classList.add('selected');
  return cleanName || null;
}

// Whitelist & Ops actions
document.querySelectorAll('[data-i18n="btnAddWl"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const card = btn.closest('.w-card');
    const input = card ? card.querySelector('input') : null;
    const name = input ? input.value.trim() : '';
    const t = i18n[state.lang];
    if (!isValidPlayerName(name)) {
      showToast(t.errEmptyName);
      return;
    }
    const status = addAccessListItem('listWhitelist', name, false);
    if (status === 'duplicate') {
      showToast(t.errDuplicateItem);
      return;
    }
    appendLog(`[${now()}] [Server thread/INFO]: Added ${name} to the whitelist`, 'sys');
    showToast(`${t.logItemAdded.replace('» ', '')}${name}`);
    if (input) input.value = '';
  });
});
document.querySelectorAll('[data-i18n="btnRemWl"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    const removed = removeSelectedAccessItem('listWhitelist');
    if (!removed) {
      showToast(t.errEmptyList);
      return;
    }
    appendLog(`[${now()}] [Server thread/INFO]: Removed ${removed} from the whitelist`, 'warn');
    showToast(`${t.logItemRemoved.replace('» ', '')}${removed}`);
  });
});
document.querySelectorAll('[data-i18n="btnOpAdd"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const card = btn.closest('.w-card');
    const input = card ? card.querySelector('input') : null;
    const name = input ? input.value.trim() : '';
    const t = i18n[state.lang];
    if (!isValidPlayerName(name)) {
      showToast(t.errEmptyName);
      return;
    }
    const status = addAccessListItem('listOps', name, true);
    if (status === 'duplicate') {
      showToast(t.errDuplicateItem);
      return;
    }
    appendLog(`[${now()}] [Server thread/INFO]: Made ${name} a server operator`, 'sys');
    showToast(`${t.logItemAdded.replace('» ', '')}${name} (OP)`);
    if (input) input.value = '';
  });
});
document.querySelectorAll('[data-i18n="btnOpRem"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    const removed = removeSelectedAccessItem('listOps');
    if (!removed) {
      showToast(t.errEmptyList);
      return;
    }
    appendLog(`[${now()}] [Server thread/INFO]: Made ${removed} no longer a server operator`, 'warn');
    showToast(`${t.logItemRemoved.replace('» ', '')}${removed}`);
  });
});

// Checkbox helpers & Property mapping
function syncChkBox(rowId, checked) {
  const row = document.getElementById(rowId);
  const box = row ? row.querySelector('.chk-box') : null;
  if (!box) return;
  box.classList.toggle('checked', !!checked);
  box.textContent = checked ? '✓' : '';
}

function isChkRowChecked(rowId) {
  const row = document.getElementById(rowId);
  const box = row ? row.querySelector('.chk-box') : null;
  return box ? box.classList.contains('checked') : false;
}

const PROP_CHK_MAP = [
  ['chkHardcoreRow', 'hardcore'],
  ['chkForceGmRow', 'forceGm'],
  ['chkOnlineRow', 'online'],
  ['chkPvpRow', 'pvp'],
  ['chkWhitelistRow', 'whitelist'],
  ['chkFlightRow', 'flight'],
  ['chkCmdBlockRow', 'cmdblock'],
  ['chkSecureProfRow', 'secureProf'],
  ['chkHidePlayersRow', 'hidePlayers'],
  ['chkNetherRow', 'nether'],
  ['chkMonstersRow', 'monsters'],
  ['chkAnimalsRow', 'animals'],
  ['chkNpcsRow', 'npcs']
];

function setGroupVal(groupId, val) {
  const grp = document.getElementById(groupId);
  if (!grp) return;
  grp.querySelectorAll('.btn-sel').forEach(b => {
    b.classList.toggle('active', b.dataset.val === val);
  });
}

function getGroupVal(groupId, fallback) {
  const grp = document.getElementById(groupId);
  const active = grp ? grp.querySelector('.btn-sel.active') : null;
  return (active && active.dataset.val) ? active.dataset.val : fallback;
}

function renderPropertiesFromState() {
  const p = state.properties;
  const setVal = (id, v) => {
    const el = document.getElementById(id);
    if (el) el.value = v;
  };
  setVal('inputMotd', p.motd);
  setVal('inputMaxPlayers', p.maxPlayers);
  setVal('inputSpawnProt', p.spawnProt);
  setVal('inputViewDist', p.viewDist);
  setVal('inputSimDist', p.simDist);
  setVal('inputLevelName', p.levelName);
  setVal('inputLevelSeed', p.levelSeed);
  setGroupVal('grpGamemode', p.gamemode);
  setGroupVal('grpDifficulty', p.difficulty);
  setGroupVal('grpLevelType', p.levelType);
  PROP_CHK_MAP.forEach(([rowId, propKey]) => syncChkBox(rowId, p[propKey]));
}

function syncSettingsFromState() {
  const s = state.appSettings;
  syncChkBox('chkAutoStartRow', s.autoStart);
  syncChkBox('chkSplashRow', s.splashAnim);
  syncChkBox('chkCrashGuardRow', s.crashGuard);
  syncChkBox('chkDailyRestartRow', s.dailyRestart);
  syncChkBox('chkSchedBkRow', s.schedBk);
  const rtEl = document.getElementById('inputRestartTime');
  if (rtEl) rtEl.value = s.dailyRestartTime;
  const jvmEl = document.getElementById('inputJvmArgs');
  if (jvmEl) jvmEl.value = s.jvmArgs;
  const bkEl = document.getElementById('inputSchedBkHours');
  if (bkEl) bkEl.value = s.schedBkHours;
}

function clampIntField(id, minV, maxV, fallback) {
  const el = document.getElementById(id);
  const raw = el ? el.value.trim() : '';
  const parsed = /^\d+$/.test(raw) ? parseInt(raw, 10) : fallback;
  const clamped = Math.max(minV, Math.min(maxV, Number.isFinite(parsed) ? parsed : fallback));
  if (el) el.value = String(clamped);
  return String(clamped);
}

// Refresh buttons (Access, Backups, Plugins & Mods)
document.querySelectorAll('[data-i18n="btnRefresh"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    appendLog(t.logListReloaded, 'info');
    showToast(t.toastUpdated);
  });
});

// Reload Properties button (restores saved snapshot)
const btnReloadProps = document.getElementById('btnReloadProps');
if (btnReloadProps) {
  btnReloadProps.addEventListener('click', () => {
    const t = i18n[state.lang];
    renderPropertiesFromState();
    appendLog(t.logPropsReloaded, 'info');
    showToast(t.toastPropsReloaded);
  });
}

// Button-group toggles (excluding lang buttons)
document.querySelectorAll('.btn-group .btn-sel:not(.lang-btn)').forEach(btn => {
  btn.addEventListener('click', () => {
    const parent = btn.parentElement;
    if (!parent) return;
    parent.querySelectorAll('.btn-sel').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
  });
});

// Checkboxes toggle + Settings sync
document.querySelectorAll('.chk-row').forEach(row => {
  row.addEventListener('click', () => {
    const box = row.querySelector('.chk-box');
    if (!box) return;
    const nextChecked = !box.classList.contains('checked');
    box.classList.toggle('checked', nextChecked);
    box.textContent = nextChecked ? '✓' : '';

    if (row.id === 'chkAutoStartRow') state.appSettings.autoStart = nextChecked;
    else if (row.id === 'chkSplashRow') state.appSettings.splashAnim = nextChecked;
    else if (row.id === 'chkCrashGuardRow') state.appSettings.crashGuard = nextChecked;
    else if (row.id === 'chkDailyRestartRow') state.appSettings.dailyRestart = nextChecked;
    else if (row.id === 'chkSchedBkRow') state.appSettings.schedBk = nextChecked;
  });
});

// Generic List Item selection (outside dashboard player list)
document.querySelectorAll('.tab-page:not(#page-panel) .player-list').forEach(list => {
  list.addEventListener('click', (e) => {
    const item = e.target.closest('.player-item');
    if (item && list.contains(item)) {
      selectListItem(list, item);
    }
  });
});

// Backup creation, restore, delete & folder simulator
document.querySelectorAll('[data-i18n="btnNewBackup"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    const tagInput = document.getElementById('inputBackupTag');
    const rawTag = tagInput ? tagInput.value.trim().replace(/[^a-zA-Z0-9_-]/g, '') : '';
    const tag = rawTag || 'survival';
    const d = new Date();
    const dateStr = `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, '0')}-${String(d.getDate()).padStart(2, '0')}`;
    const ts = now().replace(/:/g, '-');
    const fname = `backup_${dateStr}_${ts}_${tag}.zip`;

    if (state.serverPhase === 'running') {
      appendLog(`[${now()}] [Server thread/INFO]: Saved the game`, 'sys');
    }

    const list = document.getElementById('listBackups');
    if (list) {
      const item = document.createElement('div');
      item.className = 'player-item';
      item.textContent = `📦 ${fname}   •   142 MB`;
      list.prepend(item);
      selectListItem(list, item);
    }
    if (tagInput) tagInput.value = '';
    appendLog(`${t.logBackupDone}${fname}`, 'sys');
    showToast(fname);
  });
});

document.querySelectorAll('[data-i18n="btnRestoreBackup"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    if (state.serverPhase !== 'stopped') {
      showToast(t.errServerMustStop);
      return;
    }
    const bkList = document.getElementById('listBackups');
    const sel = bkList ? (bkList.querySelector('.selected') || bkList.firstElementChild) : null;
    if (!sel) {
      showToast(t.errEmptyList);
      return;
    }
    appendLog(t.logBackupRestored, 'sys');
    showToast(t.logBackupRestored.replace('» ', ''));
  });
});

document.querySelectorAll('[data-i18n="btnDeleteBackup"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    const list = document.getElementById('listBackups');
    const sel = list ? (list.querySelector('.player-item.selected') || list.querySelector('.player-item')) : null;
    if (!sel) {
      showToast(t.errEmptyList);
      return;
    }
    sel.remove();
    const next = list.querySelector('.player-item');
    if (next) next.classList.add('selected');
    appendLog(t.logBackupDeleted, 'warn');
    showToast(t.logBackupDeleted.replace('» ', ''));
  });
});

// Folder buttons (Backups, Plugins/Mods, Logs)
document.querySelectorAll('[data-i18n="btnOpenBkFolder"], [data-i18n="btnOpenFolder"], [data-i18n="btnOpenLogs"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    appendLog(t.logFolderOpened, 'info');
    showToast(t.logFolderOpened.replace('» ', ''));
  });
});

// Plugin/Mod Add & Delete simulator
const sampleAddons = {
  plugins: ['WorldEdit.jar', 'CoreProtect.jar', 'LuckPerms.jar', 'PlaceholderAPI.jar'],
  mods: ['Iris.jar', 'FerriteCore.jar', 'Indium.jar', 'ModMenu.jar']
};

document.querySelectorAll('[data-i18n="btnAddItem"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const card = btn.closest('.w-card');
    const list = card ? card.querySelector('.player-list') : null;
    if (!list) return;
    const t = i18n[state.lang];
    const isPlugin = list.id === 'listPlugins';
    const pool = isPlugin ? sampleAddons.plugins : sampleAddons.mods;
    const existingSet = new Set(
      Array.from(list.querySelectorAll('.player-item')).map(el => {
        const raw = el.firstChild ? el.firstChild.textContent : el.textContent;
        return raw.replace(/^[^\w.-]+\s*/, '').replace(/\.disabled\b/gi, '').trim().toLowerCase();
      })
    );
    const name = pool.find(n => !existingSet.has(n.toLowerCase()));
    if (!name) {
      showToast(t.errDuplicateItem);
      return;
    }

    const item = document.createElement('div');
    item.className = 'player-item';
    const icon = isPlugin ? '🧩 ' : '⚙️ ';
    item.appendChild(document.createTextNode(`${icon}${name} `));
    const badge = document.createElement('span');
    badge.style.marginLeft = 'auto';
    badge.style.color = 'var(--w-green)';
    badge.className = 'badge-status-active';
    badge.dataset.i18n = 'badgeEnabled';
    badge.textContent = t.badgeEnabled;
    item.appendChild(badge);
    list.appendChild(item);
    selectListItem(list, item);
    appendLog(`${t.logItemAdded}${name}`, 'sys');
    showToast(`${t.logItemAdded.replace('» ', '')}${name}`);
  });
});

document.querySelectorAll('[data-i18n="btnDelItem"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const card = btn.closest('.w-card');
    const list = card ? card.querySelector('.player-list') : null;
    if (!list) return;
    const t = i18n[state.lang];
    const sel = list.querySelector('.player-item.selected') || list.querySelector('.player-item');
    if (!sel) {
      showToast(t.errEmptyList);
      return;
    }
    const label = sel.firstChild ? sel.firstChild.textContent.trim() : 'item.jar';
    sel.remove();
    const next = list.querySelector('.player-item');
    if (next) next.classList.add('selected');
    appendLog(`${t.logItemRemoved}${label}`, 'warn');
    showToast(`${t.logItemRemoved.replace('» ', '')}${label}`);
  });
});

// Save Profile simulator
document.querySelectorAll('[data-i18n="btnSaveProfile"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    const input = document.getElementById('inputProfileName') || btn.parentElement.querySelector('input');
    const name = input ? input.value.trim() : '';
    if (!name) {
      showToast(t.errEmptyProfile);
      return;
    }
    const ramVal = (inputRam && inputRam.value.trim()) ? inputRam.value.trim() : '4G';
    if (!parseRamStrict(ramVal)) {
      showToast(t.errInvalidRam);
      return;
    }
    const portVal = (inputPort && inputPort.value.trim()) ? inputPort.value.trim() : '25565';
    if (!parsePortStrict(portVal)) {
      showToast(t.errInvalidPort);
      return;
    }
    const jarVal = (inputJar && inputJar.value.trim()) ? inputJar.value.trim() : 'C:\\Minecraft\\paper-1.20.4.jar';
    const ipVal = (inputIp && inputIp.value.trim()) ? inputIp.value.trim() : '';
    const jvmEl = document.getElementById('inputJvmArgs');
    const jvmVal = jvmEl ? jvmEl.value.trim() : state.appSettings.jvmArgs;

    const list = document.getElementById('listProfiles');
    if (list) {
      let item = Array.from(list.querySelectorAll('.player-item')).find(el =>
        (el.dataset.name || '').toLowerCase() === name.toLowerCase()
      );
      if (!item) {
        item = document.createElement('div');
        item.className = 'player-item';
        list.prepend(item);
      }
      item.dataset.name = name;
      item.dataset.jar = jarVal;
      item.dataset.ip = ipVal;
      item.dataset.port = portVal;
      item.dataset.ram = ramVal;
      item.dataset.jvm = jvmVal;
      item.textContent = `📂 ${name}   •   ${ramVal}   •   :${portVal}`;
      selectListItem(list, item);
    }
    if (input) input.value = '';
    appendLog(`${t.logProfileSaved}"${name}"`, 'sys');
    showToast(`${t.logProfileSaved.replace('» ', '')}"${name}"`);
  });
});

// Load Profile simulator
document.querySelectorAll('[data-i18n="btnLoadProfile"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    if (state.serverPhase !== 'stopped') {
      showToast(t.errServerMustStop);
      return;
    }
    const list = document.getElementById('listProfiles');
    const sel = list ? (list.querySelector('.player-item.selected') || list.querySelector('.player-item')) : null;
    if (!sel) {
      showToast(t.errEmptyList);
      return;
    }
    const profileName = sel.dataset.name || sel.textContent.replace(/^📂\s*/, '').split('•')[0].trim();
    if (sel.dataset.jar && inputJar) inputJar.value = sel.dataset.jar;
    if (inputIp) inputIp.value = sel.dataset.ip !== undefined ? sel.dataset.ip : '';
    if (sel.dataset.ram && inputRam) inputRam.value = sel.dataset.ram;
    if (sel.dataset.port && inputPort) inputPort.value = sel.dataset.port;
    if (sel.dataset.jvm !== undefined) {
      state.appSettings.jvmArgs = sel.dataset.jvm;
      const jvmEl = document.getElementById('inputJvmArgs');
      if (jvmEl) jvmEl.value = sel.dataset.jvm;
    }
    updateTelemetryVisuals(false);
    appendLog(`${t.logProfileLoaded}"${profileName}"`, 'info');
    showToast(`${t.logProfileLoaded.replace('» ', '')}${profileName}`);
  });
});

// Delete Profile simulator
document.querySelectorAll('[data-i18n="btnDeleteProfile"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    const list = document.getElementById('listProfiles');
    const sel = list ? (list.querySelector('.player-item.selected') || list.querySelector('.player-item')) : null;
    if (!sel) {
      showToast(t.errEmptyList);
      return;
    }
    sel.remove();
    const next = list.querySelector('.player-item');
    if (next) next.classList.add('selected');
    appendLog(t.logProfileDeleted, 'warn');
    showToast(t.logProfileDeleted.replace('» ', ''));
  });
});

// Save server.properties simulator
const btnSavePropsEl = document.getElementById('btnSaveProps');
if (btnSavePropsEl) {
  btnSavePropsEl.addEventListener('click', () => {
    const t = i18n[state.lang];
    const prevGm = state.properties.gamemode;
    const prevDiff = state.properties.difficulty;
    const prevWl = state.properties.whitelist;

    const motdEl = document.getElementById('inputMotd');
    const lvlNameEl = document.getElementById('inputLevelName');
    const lvlSeedEl = document.getElementById('inputLevelSeed');

    state.properties.motd = motdEl ? motdEl.value.trim() : state.properties.motd;
    state.properties.maxPlayers = clampIntField('inputMaxPlayers', 1, 99999, 20);
    state.properties.spawnProt = clampIntField('inputSpawnProt', 0, 256, 16);
    state.properties.viewDist = clampIntField('inputViewDist', 2, 32, 10);
    state.properties.simDist = clampIntField('inputSimDist', 2, 32, 10);
    state.properties.gamemode = getGroupVal('grpGamemode', 'survival');
    state.properties.difficulty = getGroupVal('grpDifficulty', 'normal');
    state.properties.levelType = getGroupVal('grpLevelType', 'default');

    const cleanLvl = lvlNameEl ? lvlNameEl.value.trim() : 'world';
    state.properties.levelName = cleanLvl || 'world';
    if (lvlNameEl && !cleanLvl) lvlNameEl.value = 'world';
    state.properties.levelSeed = lvlSeedEl ? lvlSeedEl.value.trim() : '';

    PROP_CHK_MAP.forEach(([rowId, propKey]) => {
      state.properties[propKey] = isChkRowChecked(rowId);
    });

    state.gamemode = state.properties.gamemode;
    state.difficulty = state.properties.difficulty;
    updateTelemetryVisuals(false);

    if (state.serverPhase === 'running') {
      if (state.properties.gamemode !== prevGm) {
        appendLog(`[${now()}] [Server thread/INFO]: Set the default game mode to ${state.properties.gamemode}`, 'info');
      }
      if (state.properties.difficulty !== prevDiff) {
        appendLog(`[${now()}] [Server thread/INFO]: The difficulty has been set to ${state.properties.difficulty}`, 'info');
      }
      if (state.properties.whitelist !== prevWl) {
        appendLog(`[${now()}] [Server thread/INFO]: Whitelist is now turned ${state.properties.whitelist ? 'on' : 'off'}`, state.properties.whitelist ? 'sys' : 'warn');
      }
    }

    appendLog(t.logPropsSaved, 'sys');
    showToast(t.toastSaved);
  });
}

// Settings -> Server & Automation save buttons with validation
const btnSaveDailyRestart = document.getElementById('btnSaveDailyRestart');
if (btnSaveDailyRestart) {
  btnSaveDailyRestart.addEventListener('click', () => {
    const t = i18n[state.lang];
    const el = document.getElementById('inputRestartTime');
    const val = el ? el.value.trim() : '';
    if (!isValidHHMM(val)) {
      showToast(t.errInvalidTime);
      return;
    }
    state.appSettings.dailyRestartTime = val;
    state.appSettings.dailyRestart = isChkRowChecked('chkDailyRestartRow');
    appendLog(t.logSettingsSaved, 'sys');
    showToast(t.toastSaved);
  });
}

const btnSaveJvm = document.getElementById('btnSaveJvm');
if (btnSaveJvm) {
  btnSaveJvm.addEventListener('click', () => {
    const t = i18n[state.lang];
    const el = document.getElementById('inputJvmArgs');
    const val = el ? el.value.trim() : '';
    if (val.includes('"')) {
      showToast(t.errInvalidJvm);
      return;
    }
    state.appSettings.jvmArgs = val;
    appendLog(t.logSettingsSaved, 'sys');
    showToast(t.toastSaved);
  });
}

const btnSaveSchedBk = document.getElementById('btnSaveSchedBk');
if (btnSaveSchedBk) {
  btnSaveSchedBk.addEventListener('click', () => {
    const t = i18n[state.lang];
    const el = document.getElementById('inputSchedBkHours');
    const raw = el ? el.value.trim() : '';
    const v = /^\d+$/.test(raw) ? parseInt(raw, 10) : 0;
    if (v < 1 || v > 72) {
      showToast(t.errInvalidHours);
      return;
    }
    state.appSettings.schedBkHours = String(v);
    state.appSettings.schedBk = isChkRowChecked('chkSchedBkRow');
    appendLog(t.logSettingsSaved, 'sys');
    showToast(t.toastSaved);
  });
}

// Toggle Plugin/Mod Active-Disabled
document.querySelectorAll('[data-i18n="btnTogglePlugin"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const card = btn.closest('.w-card');
    if (!card) return;
    const t = i18n[state.lang];
    const selected = card.querySelector('.player-item.selected') || card.querySelector('.player-item');
    if (!selected) {
      showToast(t.errEmptyList);
      return;
    }
    const badge = selected.querySelector('span[class^="badge-status-"]');
    if (!badge) return;
    const isNowActive = badge.classList.contains('badge-status-disabled');
    const textNode = selected.firstChild;
    if (textNode && textNode.nodeType === Node.TEXT_NODE) {
      const raw = textNode.nodeValue;
      if (isNowActive) {
        textNode.nodeValue = raw.replace(/\.disabled\b/i, '');
      } else if (!/\.disabled\b/i.test(raw)) {
        textNode.nodeValue = raw.trimEnd() + '.disabled ';
      }
    }
    if (isNowActive) {
      badge.className = 'badge-status-active';
      badge.dataset.i18n = 'badgeEnabled';
      badge.textContent = t.badgeEnabled;
      badge.style.color = 'var(--w-green)';
      selected.style.opacity = '1';
    } else {
      badge.className = 'badge-status-disabled';
      badge.dataset.i18n = 'badgeDisabled';
      badge.textContent = t.badgeDisabled;
      badge.style.color = 'var(--w-red)';
      selected.style.opacity = '0.6';
    }
  });
});

// Settings Theme buttons
document.querySelectorAll('.theme-card-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    setTheme(btn.dataset.theme);
  });
});

// Settings Lang buttons
document.querySelectorAll('.lang-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    setLanguage(btn.dataset.lang);
  });
});

function now() {
  const d = new Date();
  return `${String(d.getHours()).padStart(2, '0')}:${String(d.getMinutes()).padStart(2, '0')}:${String(d.getSeconds()).padStart(2, '0')}`;
}

if (inputRam) inputRam.addEventListener('input', () => updateTelemetryVisuals(false));

// Initial state
renderPropertiesFromState();
syncSettingsFromState();
setLanguage('tr');
setTheme('dark');
setPage('panel');
setSettingsSubTab('gen');
renderDashboardPlayers();
syncDashboardControlLocks();
updateTelemetryVisuals(false);
