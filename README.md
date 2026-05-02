# CS2 QuickLineups

Grenade lineup database for CS2 (smokes, molotovs, flashes). Find any lineup in under 5 seconds during a live match.

Built with **C++17 + Drogon** on the backend, plain HTML/CSS/JS on the frontend. No Node.js, no Python, no separate frontend server.

---

## Requirements

- **Windows**: Visual Studio 2022 (Community is fine), CMake ≥ 3.20, Git
- **Linux**: GCC/Clang, Ninja, CMake ≥ 3.20, Git, plus:
  ```bash
  sudo apt install -y build-essential ninja-build cmake pkg-config \
      libssl-dev uuid-dev zlib1g-dev libsqlite3-dev
  ```

### vcpkg (one-time setup)

vcpkg handles all C++ dependencies (Drogon, jsoncpp, OpenSSL, SQLite, etc.).

**Windows (PowerShell):**
```powershell
git clone https://github.com/microsoft/vcpkg C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
```
Then add `VCPKG_ROOT=C:\dev\vcpkg` to your **System Environment Variables** (search "Edit the system environment variables" in the Start menu).

**Linux / WSL:**
```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc && source ~/.bashrc
```

---

## Build

Run these from the **repo root** (the folder containing `CMakeLists.txt`).

### Windows
```powershell
# First time — installs Drogon and all deps (takes a few minutes):
cmake --preset windows-debug

# Every subsequent build:
cmake --build build --preset windows-debug
```

### Linux / WSL
```bash
cmake --preset linux-release
cmake --build build --preset linux-release
```

---

## Run

Always run from the **repo root**, not from inside `build/`:

**Windows:**
```powershell
.\build\Debug\quicklineups.exe
```

**Linux:**
```bash
./build/quicklineups
```

Then open **http://localhost:8080** in your browser.

The server logs which maps and how many lineups it loaded on startup.

---

## Usage

1. **Pick a map** from the grid on the home screen.
2. The search bar gets **auto-focused** immediately.
3. Type any keywords — site names, positions, call-outs (`ct a doors`, `b ramp post-plant`, `mid window`).
   - Results use **OR logic**: any lineup matching at least one keyword is shown, ranked by how many keywords it matches.
4. Click a result card to see the **full detail** (position photo + crosshair placement + throw instructions).

### Keyboard shortcuts

| Key | Action |
|-----|--------|
| `Escape` | Close modal / go back to map list |
| `Ctrl+K` | Re-focus the search bar |

---

## Adding & editing lineups

### Via the GUI (recommended)

Click **+ Add** in the search screen header to open the add form. Fill in:
- **Map** and **Type** (smoke / molotov / flash)
- **Title** — short descriptive name
- **Tags** — type a tag and press **Enter** or **,** to add it as a pill; click **✕** to remove
- **Position photo** and **Crosshair photo** — click the upload boxes to pick screenshots
- **Stance** and **Throw type**
- **Description** and **Video URL** (both optional)

To **edit** an existing lineup: click the card → detail modal → **✎ Edit**.  
To **delete** a lineup: click the card → detail modal → **🗑 Delete** → confirm.

All changes are saved instantly to `data/lineups/{map}.json` and survive restarts.

### Via JSON (bulk editing)

Edit files in `data/lineups/` directly — one file per map. See [`data/README.md`](data/README.md) for the schema and tag vocabulary. Restart the server to reload.

---

## Adding map thumbnails

Drop PNG files named after the map ID into `static/img/maps/`:

```
static/img/maps/dust2.png
static/img/maps/mirage.png
static/img/maps/inferno.png
static/img/maps/nuke.png
static/img/maps/ancient.png
static/img/maps/anubis.png
static/img/maps/vertigo.png
```

Recommended size: ~400×225 px. If no thumbnail exists the tile still works, just without an image.

---

## Project structure

```
fast-smoke-finder/
├── src/
│   ├── main.cc                        # entry point
│   ├── controllers/
│   │   ├── LineupController.cc/h      # GET /api/maps, GET /api/lineups/{map}
│   │   └── AdminController.cc/h       # POST/PUT/DELETE /api/lineups, POST /api/upload
│   ├── services/
│   │   └── LineupStore.cc/h           # in-memory store + inverted-index search
│   └── models/
│       └── Lineup.h                   # plain struct
├── data/
│   ├── README.md                      # schema + tag vocabulary
│   └── lineups/                       # one JSON file per map
├── static/                            # served directly by Drogon
│   ├── index.html
│   ├── css/app.css
│   ├── js/app.js
│   └── img/
│       ├── maps/                      # map thumbnails (add your own)
│       └── lineups/                   # uploaded lineup photos
├── CMakeLists.txt
├── CMakePresets.json                  # windows-debug + linux-release presets
├── vcpkg.json                         # dependency manifest
└── config.json                        # Drogon: port 8080, document root, gzip
```

---

## Configuration

`config.json` in the repo root controls the server. Key settings:

| Setting | Default | Description |
|---------|---------|-------------|
| `port` | `8080` | HTTP port |
| `threads_num` | `4` | IO threads |
| `use_gzip` | `true` | Gzip compress responses |
| `static_files_cache_time` | `86400` | Seconds to cache static files in RAM |

To change the port, edit `config.json` before running.
