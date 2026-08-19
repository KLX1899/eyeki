# Development

## Prerequisites

EyeKi currently targets Linux desktops with systemd-logind. Development requires:

- A C compiler, Make, and `pkg-config`
- Development files providing `gtk+-3.0`, `libnotify`, and `libsystemd`
- A graphical test session and notification daemon for manual UI checks
- system D-Bus and logind for idle behavior

The preliminary Debian metadata names `libgtk-3-dev`, `libnotify-dev`, `libsystemd-dev`, `pkg-config`, and `debhelper-compat (= 13)`. Treat those as Debian-oriented package names, not a tested cross-distribution install command.

Confirm discovery before building:

```sh
pkg-config --modversion gtk+-3.0 libnotify libsystemd
```

## Build and run

```sh
make
./eyeki --help
./eyeki --show-config
./eyeki --daemon
```

No-argument execution also enters the foreground daemon loop. Stop a development run with `Ctrl-C`. `make clean` removes the generated lowercase binary.

The Makefile enables `-Wall -Wextra` and honors standard build variables such as `CC`, `CPPFLAGS`, `CFLAGS`, `LDFLAGS`, and `PKG_CONFIG`. It has no debug/release profiles.

## Installation and packaging staging

Stage without writing system paths:

```sh
make DESTDIR="$PWD/package-root" install
find package-root -type f -print
```

With default variables, staged paths correspond to `/usr/bin/eyeki` and `/usr/lib/systemd/user/eyeki.service`. Remove the disposable staging directory manually after inspection. The systemd unit's `ExecStart` is fixed to `/usr/bin/eyeki`, so changing `PREFIX` requires a matching unit change.

Do not use `install.sh`. It compiles nonexistent `EyeKi.c`, creates an uppercase binary/unit, injects X11-specific environment values, and mutates the active user's service state.

## Tests, linting, formatting, and type checking

`make test` builds and runs the desktop-independent scheduler and interval/config unit tests. The interval tests cover strict parsing, the 10- and 300-minute production boundaries, invalid values, and checked seconds conversion. The scheduler tests inject shorter durations directly rather than weakening production validation. There is no linter configuration, formatter configuration, static-analysis target, broader integration suite, or CI workflow. C has no separate type-check command; compilation is the current type/syntax check. Do not report checks as passing unless they ran in the active environment.

For every C change, run at least:

```sh
make clean
make
./eyeki --help
./eyeki --show-config
```

Add focused automated coverage when extracting testable modules. Useful next targets are configuration round trips/failures, threshold transitions, clock behavior, and activity-provider error policy.

At the 2026-08-14 documentation audit, the host lacked a compiler, Make, required development metadata, Debian packaging tools, and Markdown linters. Compilation was therefore not validated. An existing ignored x86-64 executable was used only for isolated CLI observations; it is not release evidence.

## Safe configuration validation

Use a disposable home so tests do not overwrite personal settings. The explicit `.config` creation works around the current parent-directory bug:

```sh
test_home=$(mktemp -d)
mkdir -p "$test_home/.config"
env HOME="$test_home" ./eyeki --show-config
env HOME="$test_home" ./eyeki --set-interval 10
env HOME="$test_home" ./eyeki --set-mode n
env HOME="$test_home" ./eyeki --show-config
```

For the invalid-input path, first record the disposable config contents, run a value such as `--set-interval 9`, and confirm the command exits nonzero and the file is unchanged. Repeat with `300` to exercise the upper accepted boundary without starting the daemon.

Inspect only this temporary directory. Remove it manually when finished. Do not paste a real home path or journal containing unrelated session data into issues.

## Safe timer, notification, and popup validation

Run UI checks only in a disposable desktop test account/session when possible.

1. Build and run the isolated configuration steps above with the minimum production interval of ten minutes.
2. Start `env HOME="$test_home" ./eyeki --daemon` inside the graphical session.
3. In notification mode, remain active until one reminder is expected; verify a single notification and record whether the server honors the timeout.
4. Restart in popup mode (`--set-mode p` before launch); verify button and keyboard activation, window-manager close, focus, scaling, right-to-left text, and multi-monitor behavior.
5. Test idle/resume and a missing/inaccessible logind service. Current failure behavior is incorrect-prone, so do not leave rapid reminders running unattended.
6. Stop the process explicitly and inspect only EyeKi-related stderr/journal entries.

The production CLI rejects intervals below ten minutes. Use the seconds-based scheduler test interface to inject shorter durations for automated tests; do not weaken production validation to accelerate a desktop test.

## Debugging

- `systemctl --user status eyeki.service` shows unit state and recent output.
- `journalctl --user -u eyeki.service` shows service stderr, including the startup interval/mode.
- `ldd ./eyeki` shows resolved runtime libraries for a local ELF build.
- A D-Bus failure is currently indistinguishable from zero idle time at the call boundary; use a debugger or temporary, privacy-safe error instrumentation for investigation.
- Popup startup needs a working GTK display. Notification delivery needs the desktop's notification service/session bus even though idle lookup uses the system bus.

## Build and runtime variables

| Variable | Scope | Current behavior |
| --- | --- | --- |
| `CC`, `CPPFLAGS`, `CFLAGS`, `LDFLAGS`, `PKG_CONFIG` | Make | Standard tool/compiler overrides; warning flags are appended |
| `PREFIX`, `DESTDIR`, `BINDIR`, `SYSTEMD_USER_DIR` | Make | Installation/staging paths |
| `HOME` | Runtime | Sole base for config; no XDG override |
| `DISPLAY`, `WAYLAND_DISPLAY` | GTK environment | Selected by GTK; EyeKi does not read them directly |
| `DBUS_SESSION_BUS_ADDRESS` | libnotify environment | Usually inherited from the desktop session; not read directly |

No EyeKi-specific environment variables exist.

## Common failures

- **`pkg-config` cannot find a module:** install the matching development package and confirm the `.pc` search path.
- **Undefined `sd_bus_*`:** confirm the Makefile requests and links `libsystemd`, not `dbus-1`.
- **Settings do not persist:** ensure the disposable/real `$HOME/.config` parent exists; the application ignores directory/write errors.
- **Changed settings appear stale:** restart; reload happens only after the old timer threshold.
- **Popup after a live mode change fails:** restart in popup mode so GTK initializes before use.
- **Installed service cannot find the binary:** keep `ExecStart` aligned with `BINDIR`/`PREFIX`.
- **Wayland behavior differs:** GTK can render through Wayland, but stacking/focus/fullscreen policy remains compositor-controlled and unverified.

## Platform notes

- Linux and systemd-logind are hard dependencies of the current activity provider.
- The bundled unit is a user service, not a system service.
- Popup behavior must be verified separately on each claimed desktop environment and display server.
- Sandboxed packages need deliberate system-bus and notification permissions; do not broaden access without review.
- macOS and Windows require replacement activity, notification, lifecycle, and packaging adapters—not just conditional compilation.
