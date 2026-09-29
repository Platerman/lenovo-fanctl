# lenovo-fanctl

Lightweight Windows tray app to control Lenovo laptop fan speed.

A trimmed-down fork of [Lenovo-Fan-Control](https://github.com/jiarandiana0307/Lenovo-Fan-Control).
High / Normal fan modes only. No low-speed hack, no bloat.

## Features

- Tray icon with right-click menu
- Two fan modes: **High Speed** and **Normal Speed**
- Global hotkeys:
  - `Ctrl+Alt+F11` — High Speed
  - `Ctrl+Alt+F12` — Normal Speed
- CLI flags to pick the mode at startup:
  - `--high-speed` (default)
  - `--normal-speed`
- Single-instance guard (mutex)
- Auto-restores the fan to **Normal** on exit

## What's different from upstream

This is a fork, not a rewrite. Main changes:

- Removed **Low Speed** mode (upstream's `keep_fan_speed_low`)
- Removed `zh_CN` localization — English only
- Removed the custom tray icon resource; uses the default app icon
- Simplified the tray state handling and the worker-thread lifecycle
- Cleaned up `read_state()` to actually check `DeviceIoControl` return values
- Trimmed `fanctrl.h` to match the reduced API

If you want the original features (low-speed mode, localization, custom icon),
go use the upstream repo. Seriously.

## Requirements

- Windows 10 / 11
- Lenovo laptop with the **Lenovo ACPI-Compliant Virtual Power Controller**
  driver (`\\.\EnergyDrv`) installed and working
- MSYS2 with MinGW-w64 GCC, plus `pthread` support
- GNU Make

If the driver isn't there, the app will just yell at you and quit. That's expected.

## Building

From MSYS2 (MinGW 64-bit shell):

```sh
make
```

If you're building by hand:

```sh
gcc -O2 -municode -o lenovo-fanctl.exe \
    lenovo_fan_control.c fanctrl.c \
    -lpthread -lshell32 -luser32 -lgdi32
```

Adjust flags to taste. `-municode` matters — the entry point is `wWinMain`.

## Usage

Just run `lenovo-fanctl.exe`. It sits in the tray.

```sh
lenovo-fanctl.exe                 # start in High Speed mode (default)
lenovo-fanctl.exe --high-speed    # same thing
lenovo-fanctl.exe --normal-speed  # start in Normal mode
```

Tray menu:

- **State: …** — disabled label showing the current mode
- **High Speed** — spin the fan up
- **Normal Speed** — let the firmware do its thing
- **Exit** — quit (fan is reset to Normal)

## Disclaimer

This program pokes at an undocumented ACPI IOCTL through a vendor driver.
It might work. It might not. It might do something weird on your specific
model. **Use it at your own risk.** No warranty, no responsibility for
damage, thermal throttling, fan wear, or your laptop catching on fire.

Don't run High Speed mode 24/7 for no reason. You'll just wear out the fan.

## Credits

This is a fork. The real work was done by other people. Go star them.

- **jiarandiana0307** — original [Lenovo-Fan-Control](https://github.com/jiarandiana0307/Lenovo-Fan-Control)
- **bitrate16** — [FanControl](https://github.com/bitrate16/FanControl)
- **Soberia** — [Lenovo-IdeaPad-Z500-Fan-Controller](https://github.com/Soberia/Lenovo-IdeaPad-Z500-Fan-Controller)
- **[allstone.lt](https://www.allstone.lt/ideafan/)** — timing notes for the fan keep-alive trick

The `8980 ms` interval and the `NORMAL_MODE_EXPECTED_VALUE` comparison both
come from upstream. Not my idea. I just didn't delete them.

## License

MIT. See [LICENSE](LICENSE).

Original code Copyright (c) 2024 Kira Diana.
Fork modifications Copyright (c) 2025 Platerman.