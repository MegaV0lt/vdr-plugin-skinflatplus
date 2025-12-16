**Repository:** `vdr-plugin-skinflatplus` — guidance to help AI agents be productive quickly

This file gives concise, actionable guidance for coding agents working on this VDR skin/plugin.

**Big picture**:
- The repo is a VDR skin/plugin (C/C++) centered around drawing OSD elements. Core rendering lives in `baserender.c`.
- UI components are split across `displaymenu.c`, `displayreplay.c`, `displaychannel.c` (menu, replay, channel OSDs).
- `flat.c` contains helper utilities including `CreatePixmap()` and other shared helpers.
- `fontcache.c`, `imagecache.c`, and `imgloader.*` implement resource caching patterns used per OSD lifecycle.

**Key patterns & constraints (must-read before edits)**
- Pixmaps are OSD-specific: a `cPixmap` belongs to a single `cOsd` instance and becomes invalid when that OSD is destroyed. Never assume pixmaps survive across OSD destruction.
- Resource creation should use the project helper `CreatePixmap(m_Osd, "Name", layer, view, draw)` from `flat.c` (do not call `m_Osd->CreatePixmap` directly).
- Fonts are provided by the global `FontCache` (see `fontcache.c`) — use it instead of constructing fonts directly.
- Image/icon loading uses `ImgLoader.LoadIcon()` and `ImgLoader.LoadLogo()` — always check for `nullptr` before use.

**Pixmap lifecycle notes**
- Because OSD instances are often recreated, a global Pixmap cache is only useful if the OSD persists. If you attempt a global cache, always store the `cOsd*` with each cache entry and call `RemovePixmapsForOsd(m_Osd)` (or `Clear()`) before `delete m_Osd`.
- Example (safe destructor order):
```cpp
// in destructor of renderer
PixmapCache::Instance().RemovePixmapsForOsd(m_Osd); // removes and destroys pixmaps for this OSD
delete m_Osd; // safe after cache removal
```

**Weather data flow**
- Weather widget reads small files under `WIDGETOUTPUTPATH` like `weather.0.temp`, `weather.0.icon-act`, `weather.N.tempMax` etc. See `baserender.c` for reader functions. Cache at the data level (simple struct per widget) instead of caching pixmaps across OSDs.

**Build and developer workflows**
- The project contains a `Makefile` at repo root — standard build is `make` in the repo root. (Plugin packaging / install is controlled by that Makefile.)
- For single-file quick compile the workspace provides a VS Code task: `C/C++: gcc Aktive Datei kompilieren`. Prefer `make` for consistent builds.
- Debugging segfaults: check `syslog`/`dmesg` for backtraces from the plugin and reproduce under `gdb` with `gdb --args vdr` or attach to running VDR.

**Project-specific conventions**
- Naming: follow `AGENTS.md` conventions in this repo — PascalCase for functions/variables and ALL_CAPS for constants. (See `AGENTS.md` in repo root.)
- Avoid heavy global caches unless you explicitly track `cOsd*` ownership per entry. Prefer small per-widget in-memory caches for data (e.g., weather structs) and re-create pixmaps per OSD lifecycle.

**Files to inspect for context / examples**
- `baserender.c` — central renderer and many examples of safe pixmap/font/icon usage
- `flat.c` — helpers including `CreatePixmap()` (use this for creating pixmaps)
- `fontcache.c` — how fonts are cached (pattern to follow)
- `displaymenu.c`, `displayreplay.c`, `displaychannel.c` — concrete uses of renderer helpers and defensive checks

**Common pitfalls to avoid**
- Do not call `m_Osd->DestroyPixmap(p)` on a pixmap that is expected to remain in a cache shared elsewhere. If you add a cache, centralize destroy semantics in the cache and remove all other `DestroyPixmap` calls for cached items.
- Always null-check resources returned from loaders: fonts, images, pixmaps.
- If changing code that touches pixmap lifetime, run scenarios where VDR recreates the OSD (menu/skin changes) and confirm no use-after-free or double-free occurs.

**Quick examples**
- Use `CreatePixmap`:
```cpp
auto px = CreatePixmap(m_Osd, "MenuPixmap", 1, MenuViewPort, MenuViewPort);
if (!px) return; // defensive
```
- Use `FontCache`:
```cpp
auto font = FontCache.GetFont(Setup.FontOsd, Setup.FontOsdSize);
if (!font) return;
int fh = font->Height();
```

**Where to search for more patterns**
- Use repo-wide search for `CreatePixmap(`, `FontCache.`, `LoadIcon`, `DestroyPixmap(` to find relevant examples and destructor patterns.

If any section is unclear or you want more examples (constructor/destructor patterns, pixmap cache sample), tell me which area to expand or provide a code sample for and I will iterate.
