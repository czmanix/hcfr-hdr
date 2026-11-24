# Co je nového v HCFR HDR (oproti 3.5.4.4)

Přehled změn v tomto forku (`czmanix/hcfr-hdr`, verze **3.5.5.0**) oproti původní výchozí verzi [`arthur-liberman/hcfr-code`](https://github.com/arthur-liberman/hcfr-code) **3.5.4.4**.

## HDR10 a měření HDR

- **Funkční výstup HDR10 obrazců přes Direct3D 11** — v GDI režimech generátoru se měřicí obrazce vykreslují přes 10bitový BT.2020/PQ swap chain (`R10G10B10A2`, `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`). Řešení je nezávislé na výrobci GPU (vyžaduje zapnuté Windows HDR na cílovém displeji, `Win+Alt+B`) a posílá i statická HDR10 metadata z *Preferences → References* (mastering display, MaxCLL, MaxFALL). Speciální obrázky, mřížky a animace zůstávají v SDR.
- **Nastavitelná referenční bílá pro HDR barevné obrazce (ST 2084)** — nová volba v *Preferences → References* se standardními předvolbami (*HCFR default 94,38 cd/m²*, *sRGB / Windows 80 cd/m²*, *SDR reference 100 cd/m²*, *ITU-R BT.2408 203 cd/m²*, *Custom*) a tlačítkem *Reset*. Nastavení se ukládá do konfigurace i do souboru `.chc` (verze 18).
- **Popisky os jasu v cd/m²** — v HDR režimu jsou osy grafů přehledně popsané v cd/m² jak pro logaritmické, tak pro lineární zobrazení.

## Více monitorů, DPI a testovací obrazce

- **Správné škálování a vystředění obrazců na monitorech s různým DPI** — okno pro testovací obrazce (`CFullScreenWindow`) běží v režimu *Per-Monitor DPI Aware V2*, takže se obrazce vykreslují 1:1 v nativních fyzických pixelech cílového displeje (bez DWM škálování podle monitoru, na kterém běží hlavní okno HCFR). Obrazce jsou vždy přesně vystředěné a D3D11 HDR10 swap chain běží v plném nativním rozlišení panelu.
- **Skutečné názvy monitorů z EDID** — ve výběru cílového monitoru v nastavení generátoru se místo obecného *„Generic PnP Monitor“ / „Obecný monitor PnP“* zobrazuje reálný název panelu načtený přes Windows DisplayConfig API (s fallbackem na `EnumDisplayDevices`).
- **Opravy celoobrazovkových obrazců a *Additional Patterns*** — správné škálování testovacích obrázků na 1440p/4K displejích se zachováním poměru stran (`HALFTONE`), opravené skrývání/obnovování a fokus dialogu *Additional Patterns* a kompletní ovládání klávesnicí i myší (`+/-`, šipky, `PageUp`/`PageDown`, `Home`/`0`, `Space`, `Esc`/`Enter`/pravé tlačítko).

## Sondy (ArgyllCMS)

- **Vyšší limit saturace pro i1Display Pro (250 kHz)** — převzato z ArgyllCMS 3.x pro spolehlivé měření velmi jasných HDR displejů bez falešné saturace.
- **Přeskakování Bluetooth COM portů při detekci sond** — rychlejší spouštění a konec záseků při hledání připojené sondy (v případě potřeby lze vrátit volbou `[Argyll] ScanBluetoothPorts=1` v `ColorHCFR.ini`).
- **Odstranění informačního okna** *„Argyll Meter used during this session“*.
- **Srozumitelnější hlášky** při chybě polohy sondy (*„Meter is in incorrect position“*).

## Uživatelské rozhraní a opravy chyb

- **Prezentační rozvržení pro screenshoty** — nový režim v menu *View* (zkratka `F11`) s čistým rozložením a čtvercovými grafovými panely pro články a recenze.
- **Modernější tooltipy a plynulejší změna velikosti okna** — okamžité tooltipy bez problikávání a výrazně rychlejší překreslování hlavní mřížky i CIE grafu při změně velikosti okna.
- **Oprava dialogů za splash screenem** — hlášky zobrazené při startu aplikace už nezůstávají schované za úvodní obrazovkou.
- **Oprava poškození nastavení** — potvrzení vlastností generátoru před prvním otevřením *Preferences* už nepřepisuje uložené předvolby.
- **Opravy stability a správnosti dat** — pád grafu luminance po otevření `.chc` s jiným režimem osy Y (log/lineární), poškozené rozložení paměti hlavního okna v x64 (neuzavřené `#pragma pack` v hlavičkách madVR), možný pád při otevření menu během obrazce *Additional Patterns*, use-after-free při použití vlastních primárek, vlastní primárky přepisující referenci Rec.601 (SDTV) a přeházené hodnoty HiRes / integrační čas / Adapt sondy Argyll při načtení `.chc`.
- **Referenční bílá HDR podle generátoru dokumentu** — výjimka pro ruční DVD (výchozí bílá) se řídí generátorem aktivního dokumentu místo poslední volby v průvodci a do `.chc` se ukládá bílá, se kterou dokument skutečně měří.
- **HDR10 a obrazce** — černé zatemňovací okno na ostatních monitorech zůstává v SDR (žádné falešné varování o nedostupném HDR10), škálované testovací obrázky se převzorkují bikubicky (plynulé gradace a pleťové tóny) a *Reset prefs* maže `ColorHCFR.ini` vedle exe bez ohledu na pracovní složku.

## Build, 64bitová verze a instalátor

- **Nativní 64bitový (x64) build a Visual Studio 2022 (`v143`)** — aplikace se sestavuje pro x64 i Win32 při zachování plné binární kompatibility souborů `.chc` mezi oběma platformami.
- **Aktualizovaný instalátor pro Inno Setup 6** — podpora sestavení `HCFRSetup-x64.exe` i `HCFRSetup-Win32.exe`.
- **Verze 3.5.5.0** — sjednocené číslo verze napříč aplikací a všemi jazykovými DLL.
