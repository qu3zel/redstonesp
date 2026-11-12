// Redstone Server Panel Web Demo Logic

const state = {
  lang: 'tr',
  theme: 'dark',
  page: 'panel',
  isRunning: false,
  uptimeSeconds: 0,
  uptimeInterval: null,
  memoryMB: 13,
  players: ['Alex', 'Steve'],
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

const i18n = {
  tr: {
    wndTitle: "Redstone Server Panel — Minecraft Sunucu Yöneticisi",
    tabDashboard: "Panel",
    tabProperties: "Özellikler",
    tabAccess: "Yetkiler",
    tabBackups: "Yedekler",
    tabPlugins: "Eklenti & Mod",
    tabProfiles: "Profiller",
    tabSettings: "Ayarlar",
    
    cardSettings: "SUNUCU AYARLARI",
    lblJar: "Sunucu .jar dosyası",
    btnBrowse: "Seç",
    lblIp: "IP adresi (isteğe bağlı)",
    lblPort: "Port",
    lblRam: "Bellek (ör: 4G, 2048M)",
    btnStart: "Başlat",
    btnStop: "Güvenle Durdur",
    
    cardPlayers: "OYUNCULAR • ",
    btnKick: "At (Kick)",
    btnBan: "Banla",
    btnUnban: "Banı Kaldır",
    
    cardConsole: "KONSOL",
    btnSend: "Gönder",
    inputCmdPlaceholder: "Komut girin (ör: help, say Selam, op Steve, time set day)...",
    
    tileStatus: "DURUM",
    tileUptime: "ÇALIŞMA SÜRESİ",
    tileMemory: "BELLEK",
    tilePlayers: "OYUNCU",
    tileTime: "OYUN SAATİ",
    statusOffline: "KAPALI",
    statusOnline: "AKTİF",
    statusStarting: "BAŞLATILIYOR",

    // Properties
    cardPropServer: "SUNUCU BİLGİLERİ (SERVER.PROPERTIES)",
    lblMotd: "Sunucu Açıklaması (MOTD)",
    lblMaxPlayers: "Maks. Oyuncu",
    lblViewDist: "Görüş Mesafesi",
    lblSpawnProt: "Spawn Koruması",
    btnSaveProps: "Özellikleri Kaydet",
    btnReload: "Yenile",
    cardGmDiff: "OYUN MODU & ZORLUK",
    cardRules: "KURALLAR & GÜVENLİK",
    chkOnline: "Orijinal Hesap Zorunlu (online-mode)",
    chkPvp: "Oyuncular Arası Hasar (PVP)",
    chkWhitelist: "Whitelist Açık (white-list)",
    chkFlight: "Uçuşa İzin Ver (allow-flight)",
    chkCmdBlock: "Komut Blokları Aktif (enable-command-block)",

    // Access
    cardWl: "WHITELIST LİSTESİ (WHITELIST.JSON)",
    cardOps: "OPERATÖRLER / OP LİSTESİ (OPS.JSON)",
    btnAddWl: "Whitelist Ekle",
    btnRemWl: "Seçileni Çıkar",
    btnOpAdd: "OP Ver",
    btnOpRem: "OP Al (Deop)",

    // Settings
    cardGeneral: "GENEL AYARLAR",
    cardTheme: "TEMA SEÇİMİ",
    lblAutostart: "Windows ile birlikte başlat",
    themeDark: "Koyu Tema (Dark)",
    themeLight: "Açık Tema (White)",
    themeRedstone: "Redstone Teması",
    themeMinecraft: "Minecraft Teması"
  },
  en: {
    wndTitle: "Redstone Server Panel — Minecraft Server Manager",
    tabDashboard: "Dashboard",
    tabProperties: "Properties",
    tabAccess: "Access",
    tabBackups: "Backups",
    tabPlugins: "Plugins & Mods",
    tabProfiles: "Profiles",
    tabSettings: "Settings",

    cardSettings: "SERVER SETTINGS",
    lblJar: "Server .jar file",
    btnBrowse: "Browse",
    lblIp: "IP address (optional)",
    lblPort: "Port",
    lblRam: "Memory (e.g. 4G, 2048M)",
    btnStart: "Start",
    btnStop: "Stop Safely",

    cardPlayers: "PLAYERS • ",
    btnKick: "Kick",
    btnBan: "Ban",
    btnUnban: "Unban",

    cardConsole: "CONSOLE",
    btnSend: "Send",
    inputCmdPlaceholder: "Enter command (e.g. help, say Hello, op Steve, time set day)...",

    tileStatus: "STATUS",
    tileUptime: "UPTIME",
    tileMemory: "MEMORY",
    tilePlayers: "PLAYERS",
    tileTime: "WORLD TIME",
    statusOffline: "OFFLINE",
    statusOnline: "ACTIVE",
    statusStarting: "STARTING",

    // Properties
    cardPropServer: "SERVER INFO (SERVER.PROPERTIES)",
    lblMotd: "Server Description (MOTD)",
    lblMaxPlayers: "Max Players",
    lblViewDist: "View Distance (chunks)",
    lblSpawnProt: "Spawn Protection",
    btnSaveProps: "Save Properties",
    btnReload: "Reload",
    cardGmDiff: "GAMEMODE & DIFFICULTY",
    cardRules: "RULES & SECURITY",
    chkOnline: "Require Premium Account (online-mode)",
    chkPvp: "Player vs Player Damage (PVP)",
    chkWhitelist: "Enforce Whitelist (white-list)",
    chkFlight: "Allow Flight (allow-flight)",
    chkCmdBlock: "Enable Command Blocks (enable-command-block)",

    // Access
    cardWl: "WHITELIST (WHITELIST.JSON)",
    cardOps: "OPERATORS / OPS (OPS.JSON)",
    btnAddWl: "Add to Whitelist",
    btnRemWl: "Remove Selected",
    btnOpAdd: "Make OP",
    btnOpRem: "Remove OP (Deop)",

    // Settings
    cardGeneral: "GENERAL SETTINGS",
    cardTheme: "THEME SELECTION",
    lblAutostart: "Start with Windows",
    themeDark: "Dark Theme (Default)",
    themeLight: "Light Theme (White)",
    themeRedstone: "Redstone Theme",
    themeMinecraft: "Minecraft Theme"
  }
};

// UI Selectors
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
  subbarTitle.textContent = tabNames[pageName] || pageName.toUpperCase();
}

// Theme change
function setTheme(themeName) {
  state.theme = themeName;
  winWindow.className = `win-window theme-${themeName}`;
  document.querySelectorAll('.theme-card-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.theme === themeName);
  });
}

// Language change
function setLanguage(langCode) {
  state.lang = langCode;
  const t = i18n[langCode];
  
  winTitleText.textContent = t.wndTitle;
  document.querySelectorAll('[data-i18n]').forEach(el => {
    const key = el.dataset.i18n;
    if (t[key]) el.textContent = t[key];
  });
  document.querySelectorAll('[data-i18n-ph]').forEach(el => {
    const key = el.dataset.i18nPh;
    if (t[key]) el.placeholder = t[key];
  });

  // Update status tile text
  if (!state.isRunning) {
    tileStatus.textContent = t.statusOffline;
  } else {
    tileStatus.textContent = t.statusOnline;
  }
  setPage(state.page);
}

// Start / Stop Server Simulator
btnStartStop.addEventListener('click', () => {
  if (state.isRunning) {
    // Stop server
    stopServer();
  } else {
    // Start server
    startServer();
  }
});

function startServer() {
  state.isRunning = true;
  const t = i18n[state.lang];
  tileStatus.textContent = t.statusStarting;
  tileStatus.className = 'tile-val tile-status';
  btnStartStop.textContent = t.btnStop;
  btnStartStop.className = 'w-btn w-btn-stop';

  appendLog(`» [SERVER] Starting server with 4G RAM...`, 'sys');
  appendLog(`[${now()}] [Server thread/INFO]: Loading libraries, please wait...`, 'info');
  
  setTimeout(() => {
    appendLog(`[${now()}] [Server thread/INFO]: Starting minecraft server version 1.20.4`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Loading properties`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Default game type: SURVIVAL`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Generating keypair`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Starting Minecraft server on *:25565`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Preparing level "world"`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Preparing start region for dimension minecraft:overworld`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Time elapsed: 1420 ms`, 'info');
    appendLog(`[${now()}] [Server thread/INFO]: Done (2.104s)! For help, type "help"`, 'sys');
    
    tileStatus.textContent = t.statusOnline;
    tileStatus.className = 'tile-val tile-status active';
    state.memoryMB = 3719;
    tileMemory.textContent = `${state.memoryMB} MB`;
    tilePlayers.textContent = '2';

    state.uptimeSeconds = 0;
    state.uptimeInterval = setInterval(() => {
      state.uptimeSeconds++;
      tileUptime.textContent = formatUptime(state.uptimeSeconds);
    }, 1000);
  }, 1000);
}

function stopServer() {
  state.isRunning = false;
  const t = i18n[state.lang];
  clearInterval(state.uptimeInterval);
  btnStartStop.textContent = t.btnStart;
  btnStartStop.className = 'w-btn w-btn-start';
  tileStatus.textContent = t.statusOffline;
  tileStatus.className = 'tile-val tile-status';
  state.memoryMB = 13;
  tileMemory.textContent = `${state.memoryMB} MB`;
  tilePlayers.textContent = '0';
  tileUptime.textContent = '00:00:00';

  appendLog(`[${now()}] [Server thread/INFO]: Stopping the server`, 'warn');
  appendLog(`[${now()}] [Server thread/INFO]: Saving players`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: Saving worlds`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: Saving chunks for level 'ServerLevel[world]'/minecraft:overworld`, 'info');
  appendLog(`[${now()}] [Server thread/INFO]: ThreadedAnvilChunkStorage: All dimensions are saved`, 'info');
  appendLog(`» [SERVER] Server stopped safely.`, 'sys');
}

// Console command handler
function sendCommand() {
  const cmd = consoleInput.value.trim();
  if (!cmd) return;
  consoleInput.value = '';

  appendLog(`> ${cmd}`, 'chat');

  if (!state.isRunning) {
    appendLog(`» Sunucu çalışmıyor. Komut göndermek için önce "Başlat"a basın.`, 'warn');
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
        appendLog(`[${now()}] [Server thread/INFO]: There are 2 of a max of 20 players online: Alex, Steve`, 'info');
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
    box.classList.toggle('checked');
    box.textContent = box.classList.contains('checked') ? '✓' : '';
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
    document.querySelectorAll('.lang-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
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
