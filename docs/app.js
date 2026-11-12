// Redstone Server Panel Web Demo Logic

const state = {
  lang: 'tr',
  theme: 'dark',
  page: 'panel',
  settingsSubTab: 'gen',
  isRunning: false,
  uptimeSeconds: 0,
  uptimeInterval: null,
  simTimeouts: [],
  memoryMB: 13,
  onlinePlayers: [],
  selectedPlayer: null,
  gamemode: 'survival',
  difficulty: 'normal',
  properties: {
    online: true,
    pvp: true,
    whitelist: false,
    flight: false,
    cmdblock: false
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
    heroDesc: "Electron veya hantal arayüzler yok. Tek dosyada derlenen ultra hafif saf Win32 mimarisi, Razer tarzı modern sekmeler, gerçek zamanlı renkli konsol ve otomatik yedekleme sistemi.",
    heroBtnDownload: "⚡ Windows İçin İndir (.exe)",
    heroBtnGithub: "⭐ GitHub'da İncele",
    statStartup: "Açılış Süresi",
    statRam: "RAM Tüketimi",
    statThemes: "Özel Tema",
    statLang: "Çift Dil",

    // Desktop Window Title & Tabs
    wndTitle: "Redstone Server Panel — Minecraft Sunucu Yöneticisi",
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
    cardPlayers: "OYUNCULAR • ",
    lblNoPlayers: "Çevrim içi oyuncu yok",
    btnKick: "At (Kick)",
    btnBan: "Banla",
    lblUnban: "Banı kaldır (oyuncu adı)",
    btnUnban: "Banı Kaldır",
    cardConsole: "KONSOL",
    btnSend: "Gönder",
    inputCmdPlaceholder: "Komut girin (ör: help, say Selam, op Steve, time set day)...",
    
    // Telemetry Tiles
    tileStatus: "DURUM",
    tileUptime: "ÇALIŞMA SÜRESİ",
    tileMemory: "BELLEK",
    tilePlayers: "OYUNCU",
    tileTime: "OYUN SAATİ",
    statusOffline: "KAPALI",
    statusOnline: "AKTİF",
    statusStarting: "BAŞLATILIYOR",

    // Page 2: Properties
    cardPropServer: "SUNUCU BİLGİLERİ (SERVER.PROPERTIES)",
    lblMotd: "Sunucu Açıklaması (MOTD)",
    lblMaxPlayers: "Maks. Oyuncu",
    lblViewDist: "Görüş Mesafesi",
    lblSpawnProt: "Spawn Koruması",
    btnSaveProps: "Özellikleri Kaydet",
    btnReload: "Yenile",
    descPropsNote: "Not: Bazı ayarların tam geçerli olması için sunucuyu yeniden başlatmanız gerekir.",
    cardGmDiff: "OYUN MODU & ZORLUK",
    lblDefGamemode: "Varsayılan Oyun Modu",
    lblDifficulty: "Zorluk Derecesi",
    gmSurvival: "Survival",
    gmCreative: "Creative",
    gmAdventure: "Adventure",
    gmSpectator: "Spectator",
    dfPeaceful: "Peaceful",
    dfEasy: "Easy",
    dfNormal: "Normal",
    dfHard: "Hard",
    cardRules: "KURALLAR & GÜVENLİK",
    chkOnline: "Orijinal Hesap Zorunlu (online-mode)",
    chkPvp: "Oyuncular Arası Hasar (PVP)",
    chkWhitelist: "Whitelist Açık (white-list)",
    chkFlight: "Uçuşa İzin Ver (allow-flight)",
    chkCmdBlock: "Komut Blokları Aktif (enable-command-block)",

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
    logSettingsSaved: "» [AYAR] Otomasyon ve sunucu ayarları güncellendi.",

    // Features Section
    featTag: "Neden Redstone Server Panel?",
    featMainTitle: "Saf Win32 Mimarisi ve Üstün Performans",
    feat1Title: "Sıfır Hantallık, ~10 MB RAM",
    feat1Desc: "Electron, Node.js veya WebView2 kütüphanelerine ihtiyaç duymaz. Tek dosyalık saf C++17 kodu sayesinde anında açılır ve minimum bellek tüketir.",
    feat2Title: "Razer Tarzı 7 Modern Sekme",
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
    heroDesc: "No Electron, no webview bloat. Single-file ultra-lightweight native Win32 architecture, Razer-style modern tabs, real-time colorized console, and automated backups.",
    heroBtnDownload: "⚡ Download for Windows (.exe)",
    heroBtnGithub: "⭐ View on GitHub",
    statStartup: "Startup Time",
    statRam: "RAM Usage",
    statThemes: "Custom Themes",
    statLang: "Bilingual",

    // Desktop Window Title & Tabs
    wndTitle: "Redstone Server Panel — Minecraft Server Manager",
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
    cardPlayers: "PLAYERS • ",
    lblNoPlayers: "No players online",
    btnKick: "Kick",
    btnBan: "Ban",
    lblUnban: "Unban player (player name)",
    btnUnban: "Unban",
    cardConsole: "CONSOLE",
    btnSend: "Send",
    inputCmdPlaceholder: "Enter command (e.g. help, say Hello, op Steve, time set day)...",
    
    // Telemetry Tiles
    tileStatus: "STATUS",
    tileUptime: "UPTIME",
    tileMemory: "MEMORY",
    tilePlayers: "PLAYERS",
    tileTime: "WORLD TIME",
    statusOffline: "OFFLINE",
    statusOnline: "ACTIVE",
    statusStarting: "STARTING",

    // Page 2: Properties
    cardPropServer: "SERVER INFO (SERVER.PROPERTIES)",
    lblMotd: "Server Description (MOTD)",
    lblMaxPlayers: "Max Players",
    lblViewDist: "View Distance (chunks)",
    lblSpawnProt: "Spawn Protection",
    btnSaveProps: "Save Properties",
    btnReload: "Reload",
    descPropsNote: "Note: The server must be restarted for some settings to take full effect.",
    cardGmDiff: "GAMEMODE & DIFFICULTY",
    lblDefGamemode: "Default Game Mode",
    lblDifficulty: "World Difficulty",
    gmSurvival: "Survival",
    gmCreative: "Creative",
    gmAdventure: "Adventure",
    gmSpectator: "Spectator",
    dfPeaceful: "Peaceful",
    dfEasy: "Easy",
    dfNormal: "Normal",
    dfHard: "Hard",
    cardRules: "RULES & SECURITY",
    chkOnline: "Require Premium Account (online-mode)",
    chkPvp: "Player vs Player Damage (PVP)",
    chkWhitelist: "Enforce Whitelist (white-list)",
    chkFlight: "Allow Flight (allow-flight)",
    chkCmdBlock: "Enable Command Blocks (enable-command-block)",

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
    logSettingsSaved: "» [SETTINGS] Automation and server settings updated.",

    // Features Section
    featTag: "Why Redstone Server Panel?",
    featMainTitle: "Pure Win32 Architecture & Extreme Performance",
    feat1Title: "Zero Bloat, ~10 MB RAM",
    feat1Desc: "No Electron, Node.js, or WebView2 required. Pure C++17 single-file code starts instantly in ~10ms with minimal memory footprint.",
    feat2Title: "Razer-Style 7 Modern Tabs",
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
const winTitleText = document.getElementById('winTitleText');
const subbarTitle = document.getElementById('subbarTitle');
const consoleBody = document.getElementById('consoleBody');
const consoleInput = document.getElementById('consoleInput');
const btnStartStop = document.getElementById('btnStartStop');
const tileStatus = document.getElementById('tileStatus');
const tileUptime = document.getElementById('tileUptime');
const tileMemory = document.getElementById('tileMemory');
const tilePlayers = document.getElementById('tilePlayers');
const langPillLabel = document.getElementById('langPillLabel');

// Simulation timers
function clearSimTimeouts() {
  state.simTimeouts.forEach(id => clearTimeout(id));
  state.simTimeouts = [];
}

function scheduleSim(fn, delay) {
  const id = setTimeout(() => {
    if (!state.isRunning) return;
    fn();
  }, delay);
  state.simTimeouts.push(id);
  return id;
}

// Append log line
function appendLog(line, type = '') {
  const div = document.createElement('div');
  div.className = type ? `console-line-${type}` : '';
  div.textContent = line;
  consoleBody.appendChild(div);
  consoleBody.scrollTop = consoleBody.scrollHeight;
}

// Format time
function formatUptime(sec) {
  const h = String(Math.floor(sec / 3600)).padStart(2, '0');
  const m = String(Math.floor((sec % 3600) / 60)).padStart(2, '0');
  const s = String(sec % 60).padStart(2, '0');
  return `${h}:${m}:${s}`;
}

// Tab navigation
document.querySelectorAll('.win-tab').forEach(tab => {
  tab.addEventListener('click', () => {
    const targetPage = tab.dataset.page;
    setPage(targetPage);
  });
});

function setPage(pageName) {
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
}

// Settings Sub-Tabs
function setSettingsSubTab(subtab) {
  state.settingsSubTab = subtab;
  document.querySelectorAll('.subtab-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.subtab === subtab);
  });
  const subGen = document.getElementById('subpage-gen');
  const subSrv = document.getElementById('subpage-srv');
  if (subtab === 'srv') {
    if (subGen) subGen.classList.remove('active');
    if (subSrv) subSrv.classList.add('active');
  } else {
    if (subGen) subGen.classList.add('active');
    if (subSrv) subSrv.classList.remove('active');
  }
}
window.setSettingsSubTab = setSettingsSubTab;

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
  if (tilePlayers) tilePlayers.textContent = String(count);

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
    div.innerHTML = `
      <div class="skin-head" style="background: ${p.color || '#4ade80'};"></div>
      <strong>${p.name}</strong>
    `;
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

  // Update Start/Stop button
  if (state.isRunning) {
    btnStartStop.textContent = t.btnStop;
    tileStatus.textContent = t.statusOnline;
  } else {
    btnStartStop.textContent = t.btnStart;
    tileStatus.textContent = t.statusOffline;
  }

  // Update initial console greeting if only 1 line exists
  const initialMsg = document.getElementById('consoleInitialMsg');
  if (initialMsg && consoleBody.children.length === 1) {
    initialMsg.textContent = t.consoleReady;
  }

  renderDashboardPlayers();
  setPage(state.page);
}
window.setLanguage = setLanguage;

// Start / Stop Server Simulator
btnStartStop.addEventListener('click', () => {
  if (state.isRunning) {
    stopServer();
  } else {
    startServer();
  }
});

function startServer() {
  clearSimTimeouts();
  state.isRunning = true;
  state.onlinePlayers = [];
  state.selectedPlayer = null;
  renderDashboardPlayers();

  const t = i18n[state.lang];
  tileStatus.textContent = t.statusStarting;
  tileStatus.className = 'tile-val tile-status';
  btnStartStop.textContent = t.btnStop;
  btnStartStop.className = 'w-btn w-btn-stop';

  appendLog(t.consoleStarting, 'sys');
  appendLog(`[${now()}] [Server thread/INFO]: Loading libraries, please wait...`, 'info');

  scheduleSim(() => {
    appendLog(`[${now()}] [Server thread/INFO]: Starting minecraft server version 1.20.4`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Loading properties`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Default game type: SURVIVAL`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Generating keypair`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Starting Minecraft server on *:25565`, 'info');
  }, 600);

  scheduleSim(() => {
    appendLog(`[${now()}] [Server thread/INFO]: Preparing level "world"`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Preparing start region for dimension minecraft:overworld`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Time elapsed: 1420 ms`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Done (2.104s)! For help, type "help"`, 'sys');
    
    tileStatus.textContent = t.statusOnline;
    tileStatus.className = 'tile-val tile-status active';
    state.memoryMB = 3719;
    tileMemory.textContent = `${state.memoryMB} MB`;

    state.uptimeSeconds = 0;
    state.uptimeInterval = setInterval(() => {
      state.uptimeSeconds++;
      tileUptime.textContent = formatUptime(state.uptimeSeconds);
    }, 1000);
  }, 1500);

  // Player 1 joins (Alex)
  scheduleSim(() => {
    appendLog(`[${now()}] [User Authenticator #1/INFO]: UUID of player Alex is c06f8906-4c3e-4b47-a4dd-24d1a55eb19e`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Alex[local] logged in with entity id 142 at (-42.5, 68.0, 112.3)`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Alex joined the game`, 'sys');

    state.onlinePlayers.push({ name: 'Alex', color: '#4ade80' });
    renderDashboardPlayers();
  }, 3200);

  // Player 2 joins (Steve)
  scheduleSim(() => {
    appendLog(`[${now()}] [User Authenticator #2/INFO]: UUID of player Steve is 853c80ef-3c37-49fd-aa49-938b674adae6`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Steve[local] logged in with entity id 189 at (-38.2, 69.0, 105.1)`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Steve joined the game`, 'sys');

    state.onlinePlayers.push({ name: 'Steve', color: '#38bdf8' });
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

function stopServer() {
  state.isRunning = false;
  clearSimTimeouts();
  clearInterval(state.uptimeInterval);

  const t = i18n[state.lang];
  btnStartStop.textContent = t.btnStart;
  btnStartStop.className = 'w-btn w-btn-start';
  tileStatus.textContent = t.statusOffline;
  tileStatus.className = 'tile-val tile-status';
  state.memoryMB = 13;
  tileMemory.textContent = `${state.memoryMB} MB`;
  tileUptime.textContent = '00:00:00';

  if (state.onlinePlayers.length > 0) {
    state.onlinePlayers.forEach(p => {
      appendLog(`[${now()}] [Server thread/INFO]: ${p.name} lost connection: Server closed`, 'warn');
      appendLog(`[${now()}] [Server thread/INFO]: ${p.name} left the game`, 'warn');
    });
  }
  state.onlinePlayers = [];
  state.selectedPlayer = null;
  renderDashboardPlayers();

  appendLog(`[${now()}] [Server thread/INFO]: Stopping the server`, 'warn');
  appendLog(`[${now()}] [Server thread/INFO]: Saving players`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: Saving worlds`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: Saving chunks for level 'ServerLevel[world]'/minecraft:overworld`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: ThreadedAnvilChunkStorage: All dimensions are saved`, 'info');
  appendLog(t.consoleStopping, 'sys');
}

// Console command handler
function sendCommand() {
  const cmd = consoleInput.value.trim();
  if (!cmd) return;
  consoleInput.value = '';

  appendLog(`> ${cmd}`, 'chat');

  const t = i18n[state.lang];
  if (!state.isRunning) {
    appendLog(t.consoleNotRunning, 'warn');
    return;
  }

  const parts = cmd.split(' ');
  const base = parts[0].toLowerCase();

  setTimeout(() => {
    switch (base) {
      case 'help':
        appendLog(`[${now()}] [Server thread/INFO]: --- Showing help page 1 of 1 ---`, 'info');
        appendLog(`[${now()}] [Server thread/INFO]: /help, /list, /say <message>, /stop, /op <player>, /time set <day|night>`, 'info');
        break;
      case 'list':
        const names = state.onlinePlayers.map(p => p.name).join(', ');
        appendLog(`[${now()}] [Server thread/INFO]: There are ${state.onlinePlayers.length} of a max of 20 players online${names ? ': ' + names : ''}`, 'info');
        break;
      case 'say':
        const msg = parts.slice(1).join(' ') || 'Hello from Server!';
        appendLog(`[${now()}] [Server thread/INFO]: [Server] ${msg}`, 'chat');
        break;
      case 'op':
        const pl = parts[1] || 'Player';
        appendLog(`[${now()}] [Server thread/INFO]: Made ${pl} a server operator`, 'sys');
        break;
      case 'time':
        appendLog(`[${now()}] [Server thread/INFO]: Set the time to 1000`, 'info');
        break;
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
  if (e.key === 'Enter') sendCommand();
});

// Unban button interaction
const btnUnban = document.getElementById('btnUnban');
const inputUnban = document.getElementById('inputUnban');
if (btnUnban && inputUnban) {
  btnUnban.addEventListener('click', () => {
    const val = inputUnban.value.trim() || 'Alex';
    const t = i18n[state.lang];
    appendLog(`[${now()}] [Server thread/INFO]: Unbanned player ${val}`, 'sys');
    inputUnban.value = '';
  });
}

// Kick & Ban simulator
const btnKickPlayer = document.getElementById('btnKickPlayer');
if (btnKickPlayer) {
  btnKickPlayer.addEventListener('click', () => {
    if (!state.isRunning || state.onlinePlayers.length === 0) return;
    const target = state.selectedPlayer || state.onlinePlayers[0].name;
    state.onlinePlayers = state.onlinePlayers.filter(p => p.name !== target);
    appendLog(`[${now()}] [Server thread/INFO]: Kicked ${target} from the game: "Kicked by an operator"`, 'warn');
    appendLog(`[${now()}] [Server thread/INFO]: ${target} left the game`, 'warn');
    state.selectedPlayer = state.onlinePlayers.length ? state.onlinePlayers[0].name : null;
    renderDashboardPlayers();
  });
}

const btnBanPlayer = document.getElementById('btnBanPlayer');
if (btnBanPlayer) {
  btnBanPlayer.addEventListener('click', () => {
    if (!state.isRunning || state.onlinePlayers.length === 0) return;
    const target = state.selectedPlayer || state.onlinePlayers[0].name;
    state.onlinePlayers = state.onlinePlayers.filter(p => p.name !== target);
    appendLog(`[${now()}] [Server thread/INFO]: Banned ${target}: "Banned by an operator"`, 'sys');
    appendLog(`[${now()}] [Server thread/INFO]: ${target} left the game`, 'warn');
    state.selectedPlayer = state.onlinePlayers.length ? state.onlinePlayers[0].name : null;
    renderDashboardPlayers();
  });
}

// Whitelist & Ops actions
document.querySelectorAll('[data-i18n="btnAddWl"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const card = btn.closest('.w-card');
    const input = card ? card.querySelector('input') : null;
    const name = (input && input.value.trim()) ? input.value.trim() : 'NewPlayer';
    appendLog(`[${now()}] [Server thread/INFO]: Added ${name} to the whitelist`, 'sys');
    if (input) input.value = '';
  });
});
document.querySelectorAll('[data-i18n="btnRemWl"]').forEach(btn => {
  btn.addEventListener('click', () => {
    appendLog(`[${now()}] [Server thread/INFO]: Removed Steve from the whitelist`, 'warn');
  });
});
document.querySelectorAll('[data-i18n="btnOpAdd"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const card = btn.closest('.w-card');
    const input = card ? card.querySelector('input') : null;
    const name = (input && input.value.trim()) ? input.value.trim() : 'Player';
    appendLog(`[${now()}] [Server thread/INFO]: Made ${name} a server operator`, 'sys');
    if (input) input.value = '';
  });
});
document.querySelectorAll('[data-i18n="btnOpRem"]').forEach(btn => {
  btn.addEventListener('click', () => {
    appendLog(`[${now()}] [Server thread/INFO]: Made Steve no longer a server operator`, 'warn');
  });
});

// Reload buttons
document.querySelectorAll('[data-i18n="btnReload"]').forEach(btn => {
  btn.addEventListener('click', () => {
    appendLog(`» [SİSTEM] Liste güncellendi.`, 'info');
  });
});

// Gamemode / Difficulty toggles in Props page
document.querySelectorAll('.btn-sel').forEach(btn => {
  btn.addEventListener('click', () => {
    const parent = btn.parentElement;
    parent.querySelectorAll('.btn-sel').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
  });
});

// Checkboxes toggle
document.querySelectorAll('.chk-row').forEach(row => {
  row.addEventListener('click', () => {
    const box = row.querySelector('.chk-box');
    if (!box) return;
    box.classList.toggle('checked');
    box.textContent = box.classList.contains('checked') ? '✓' : '';
  });
});

// Generic List Item selection (outside dashboard player list)
document.querySelectorAll('.tab-page:not(#page-panel) .player-list').forEach(list => {
  list.addEventListener('click', (e) => {
    const item = e.target.closest('.player-item');
    if (item && list.contains(item)) {
      list.querySelectorAll('.player-item').forEach(it => it.classList.remove('selected'));
      item.classList.add('selected');
    }
  });
});

// Backup creation simulator
document.querySelectorAll('[data-i18n="btnNewBackup"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    const ts = now().replace(/:/g, '-');
    appendLog(`${t.logBackupDone}backup_${ts}_survival.zip`, 'sys');
  });
});

// Save Profile simulator
document.querySelectorAll('[data-i18n="btnSaveProfile"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    const input = btn.parentElement.querySelector('input');
    const name = (input && input.value.trim()) ? input.value.trim() : 'Custom Server';
    appendLog(`${t.logProfileSaved}"${name}"`, 'sys');
  });
});

// Load Profile simulator
document.querySelectorAll('[data-i18n="btnLoadProfile"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    appendLog(`${t.logProfileLoaded}"26.3 Survival" (5G, :25565)`, 'info');
  });
});

// Save server.properties simulator
document.querySelectorAll('[data-i18n="btnSaveProps"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    appendLog(t.logPropsSaved, 'sys');
  });
});

// General Save buttons simulator
document.querySelectorAll('.tab-page button[data-i18n="btnSave"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const t = i18n[state.lang];
    appendLog(t.logSettingsSaved, 'sys');
  });
});

// Toggle Plugin/Mod Active-Disabled
document.querySelectorAll('[data-i18n="btnTogglePlugin"]').forEach(btn => {
  btn.addEventListener('click', () => {
    const card = btn.closest('.w-card');
    if (!card) return;
    const selected = card.querySelector('.player-item.selected') || card.querySelector('.player-item');
    if (!selected) return;
    const badge = selected.querySelector('span[class^="badge-status-"]');
    if (!badge) return;
    const t = i18n[state.lang];
    const isNowActive = badge.classList.contains('badge-status-disabled');
    if (isNowActive) {
      badge.className = 'badge-status-active';
      badge.textContent = t.badgeEnabled;
      badge.style.color = 'var(--w-green)';
      selected.style.opacity = '1';
    } else {
      badge.className = 'badge-status-disabled';
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

// Initial state
setLanguage('tr');
setTheme('dark');
setPage('panel');
setSettingsSubTab('gen');
renderDashboardPlayers();
