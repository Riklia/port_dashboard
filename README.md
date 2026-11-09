# Port Dashboard

Port Dashboard is a lightweight C++20 local web server that lists all TCP ports currently listening on `127.0.0.1`. For each open port, it identifies the owning process (PID, name, and command/path) and sends a fast parallel HTTP probe to detect running web servers, status codes, `Server` headers, and HTML page titles.

## Features

- Scans IPv4 and IPv6 TCP listening ports on Windows (IP Helper API) and Linux (`/proc/net/tcp` and `/proc/<pid>/fd`).
- Probes all discovered ports in parallel to keep page loads fast (~150 ms).
- Displays clickable port links, HTTP status badges, page titles, and process details in a web interface.
- Filters ports by search query, hides default system/ephemeral ports, and lets you mute specific ports (saved in browser `localStorage`).

## Requirements

- C++20 compiler
- CMake 3.8 or newer
- Ninja build system

## How to Build

### Visual Studio (Windows and WSL)

1. Open the project folder in Visual Studio (`File -> Open -> Folder`).
2. Select the target system (`Local Machine` or `WSL`) and configuration (`x64 Debug`, `x64 Release`, `Linux Debug`, or `Linux Release`) from the top toolbar.
3. Click `Build -> Build All`.

### Command Line (Windows)

Run from a Visual Studio x64 Native Tools Command Prompt:

```bat
cmake --preset x64-release
cmake --build out/build/x64-release
```

### Command Line (Linux / WSL)

```bash
cmake --preset linux-release
cmake --build out/build/linux-release
```

## How to Run

Start the executable:

- **Windows**: `.\out\build\x64-release\port_dashboard.exe`
- **Linux**: `./out/build/linux-release/port_dashboard`

The server binds to `127.0.0.1:80` by default. If port `80` is already in use or requires elevated privileges (such as running without `sudo` on Linux), it automatically falls back to `127.0.0.1:8080`. Check the terminal output for the active URL:

```text
Port Dashboard listening on http://127.0.0.1:80
```

Open that URL in your browser.

> Note: Running as Administrator (Windows) or with `sudo` (Linux) is optional, but allows the scanner to read process names and paths for system-owned ports.

## Adding to PATH

Because the binary embeds a fallback path to `dashboard.html` at compile time, you can run `port_dashboard` from any working directory.

### Windows (PowerShell)

Run from the project root directory to add the release build folder to your user `PATH`:

```powershell
[Environment]::SetEnvironmentVariable("Path", [Environment]::GetEnvironmentVariable("Path", "User") + ";" + (Resolve-Path ".\out\build\x64-release").Path, "User")
```

Open a new terminal and run `port_dashboard`.

### Linux / WSL

Run from the project root directory to create a symlink in `~/.local/bin` (or `/usr/local/bin`):

```bash
mkdir -p ~/.local/bin
ln -sf "$(pwd)/out/build/linux-release/port_dashboard" ~/.local/bin/port_dashboard
```

Make sure `~/.local/bin` is in your `PATH`, then run `port_dashboard`.
