# Plan: Cache image filename path resolution to reduce disk access

## Goal
Cache file-path resolution (including negative results) so repeated logo/icon/poster lookups avoid redundant `stat()`/`LastModifiedTime` disk operations, improving performance and reducing disk I/O.

## Root-cause findings
- `ImgCache` (global `cImageCache`, `flat.cpp:57`) already caches decoded+scaled images by `(fullpath, width, height)` with O(1) hash lookup — good, but only for the *decoded image*.
- `ImgLoader` is a **per-instance member of `cFlatBaseRender`** (`baserender.h:84`), recreated each time an OSD opens. Any cache stored inside `cImageLoader` is lost on OSD reopen, so the resolution cache must live in the **global `ImgCache`**.
- Path/existence probing is NOT cached; on every miss the disk is hit:
  - `cImageLoader::GetLogo` (`imageloader.cpp:40`): up to **3× `stat()`** (plain → lowercase → `~` for `/`).
  - `cImageLoader::GetIcon` (`imageloader.cpp:119`): up to **2× `stat()`** (theme path → `default`).
  - `cImageLoader::SearchRecordingPoster` → `CheckImageExistence` (`imageloader.cpp:266/294`): up to **4 names × 3 paths = 12× `stat()`** per recording.
- Hot paths: missing channel logos (channels without logos re-probe on every channel switch / menu redraw), fallback icons, recording poster lookups.
- Concurrency: image cache/loader are main-thread-only (verified `RecCountThread` does not touch them; existing `m_ImageIndex`/`m_IconIndex` use no mutex). Follow the same assumption — no new mutex.

## Scope (confirmed by user)
Cache resolution for **logos + icons + posters**, and **add hit/miss dsyslog** guarded by `DEBUGIMAGELOADTIME`.

## Design

### 1. `imagecache.h` — add resolution cache API + storage
Add near the existing index members:
```cpp
// File-path resolution cache. Key -> resolved full path; empty value = negative (not found).
mutable std::unordered_map<std::string, cString> m_PathCache;
static constexpr std::size_t kMaxPathCache {2048};
```
Add public methods:
```cpp
// Returns true on cache hit and writes resolved path to Out.
// Out may be empty (cached negative result: path does not exist).
bool ResolvePath(const std::string &Key, cString &Out) const;
void CacheResolvedPath(const std::string &Key, const cString &Path);
void ClearPathCache();
```
Note: keep the map keyed by a composite, namespaced string built by the caller (see keys below) so logos/icons/posters cannot collide.

### 2. `imagecache.cpp` — implement + lifecycle
- `ResolvePath`: lookup in `m_PathCache`; on hit set `Out` and return `true`.
- `CacheResolvedPath`: if `m_PathCache.size() >= kMaxPathCache`, clear it first (simple amortized cap); then insert/assign.
- `ClearPathCache`: `m_PathCache.clear();`.
- `Clear()`: call `ClearPathCache()` alongside the existing index clears.
- `Create()`: already calls `Clear()`, so it is covered.
- `RemoveFromCache(Name)` (SVDRP `RLFC`, `skinflatplus.cpp:166`): also erase matching logo resolution entries so a re-added logo is re-probed. Match by base filename (reuse `BaseNameFromCacheName`) against the stored resolved path, or simply `ClearPathCache()` — simplest and safe. Recommend clearing the logo key(s) whose resolved basename equals `Name`; if that is awkward, clear the whole path cache on RLFC (rare command).

### 3. `imageloader.cpp` — consult the cache
Keys (composite, namespaced; use `\x1f` separator):
- Logo: `"L" + Config.LogoPath + "\x1f" + logo`
- Icon: `"I" + Config.IconPath + "\x1f" + Setup.OSDTheme + "\x1f" + cIcon`
- Poster: `"P" + Config.TVScraperSearchLocalPosters(as char) + "\x1f" + RecPath`

**`GetLogo`**: build key; `if (ImgCache.ResolvePath(key, resolved)) { if (resolved empty) return nullptr; File = resolved; goto load; }` — on hit skip the 3-variant loop entirely. On miss run the existing loop, then `ImgCache.CacheResolvedPath(key, foundPathOrEmpty)` before returning. Keep the existing `GetImage`/load behavior at the resolved path.
- If resolution returns empty (negative): return `nullptr` with no disk access (still honor `Quiet` logging).

**`GetIcon`**: same pattern around the theme→default fallback. On hit with a non-empty path, `GetImage` then `LoadImage`; on negative hit return `nullptr` immediately.

**`SearchRecordingPoster`/`CheckImageExistence`**: build key once in `SearchRecordingPoster`; on `ResolvePath` hit copy cached value into `found` and return `!empty`. On miss run the existing loop, then `CacheResolvedPath(key, foundOrEmpty)`.

Add guarded diagnostics:
- On hit: `#ifdef DEBUGIMAGELOADTIME dsyslog("... resolve cache HIT for %s -> %s", key, path); #endif`
- On miss: `#ifdef DEBUGIMAGELOADTIME dsyslog("... resolve cache MISS for %s", key); #endif`

### 4. Bounding / invalidation notes (document in code comments)
- `kMaxPathCache` cap with clear-on-overflow (amortized O(1)).
- Cleared on `Clear()`/`Create()` (plugin stop / restart) and on `RLFC`.
- Keys embed `Config.IconPath`, `Setup.OSDTheme`, `Config.LogoPath`, `Config.TVScraperSearchLocalPosters`, so runtime changes are naturally isolated (no stale hits). `Config.LogoPath`/`IconPath` are set at startup only; `Setup.OSDTheme` / `TVScraperSearchLocalPosters` changes simply produce different keys.
- Main-thread-only assumption documented.

## Files to modify
- `imagecache.h` — add `m_PathCache`, `kMaxPathCache`, `ResolvePath`/`CacheResolvedPath`/`ClearPathCache` declarations.
- `imagecache.cpp` — implement the three methods, wire into `Clear()`, invalidate in `RemoveFromCache()`.
- `imageloader.cpp` — add cache lookups in `GetLogo`, `GetIcon`, `SearchRecordingPoster` (+ `CheckImageExistence` path), with `DEBUGIMAGELOADTIME` hit/miss logs.

## Validation
1. `make` — must compile cleanly (repo uses `.clang-format` Microsoft style; keep ≤120 cols, 4-space indent).
2. Manual / log-based check with `DEBUGIMAGELOADTIME`:
   - Open channel list / switch channels: confirm "resolve cache HIT" lines appear for repeated logos and that `cImageMagickWrapper::LoadImage` `LastModifiedTime` calls drop on repeats.
   - Navigate a menu with many icons: confirm icon resolution HITs (theme→default no longer re-`stat`ed).
   - Open recordings list / replay: confirm `SearchRecordingPoster` no longer repeats 12 stat calls per recording.
3. Functional regression: channel logos (with/without file, uppercase/lowercase, `/` in name), theme icons + default fallback, recording posters (banner/poster/fanart/cover) still render correctly.
4. `RLFC` (SVDRP) still removes a logo and forces re-probe/reload.

## Risks / mitigations
- Stale resolution if a logo/poster file is added/removed at runtime: mitigated for logos by `RLFC` clearing resolution; posters are static during playback. Low risk.
- Unbounded growth: capped + clear-on-overflow.
- Key collision between logos/icons/posters: prevented by distinct prefixes and included context.