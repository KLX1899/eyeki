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

`make test` builds and runs the desktop-independent scheduler, runtime reload, config-watch, interval/config-persistence, and logind session-selection unit tests. Configuration coverage includes strict parsing, production boundaries, checked conversion, fresh-home creation, XDG precedence and legacy fallback, owner-only modes, atomic replacement, event observation, and failure cleanup/reporting. Runtime coverage verifies that interval and mode changes reset elapsed active time and that an invalid replacement cannot partially change state. Session-selection coverage verifies effective-UID ownership, process and primary-display precedence, local graphical eligibility, unique fallback, ambiguity, and distinct empty/error results. The scheduler tests inject shorter durations directly rather than weakening production validation. There is no linter configuration, formatter configuration, static-analysis target, broader integration suite, or CI workflow. C has no separate type-check command; compilation is the current type/syntax check. Do not report checks as passing unless they ran in the active environment.

For every C change, run at least:

```sh
make clean
make
./eyeki --help
./eyeki --show-config
```

Add focused automated coverage when extracting testable modules. Useful next targets are persisted mode diagnostics, concurrent configuration updates, threshold transitions, clock behavior, and activity-provider error policy.

At the 2026-08-14 documentation audit, the host lacked a compiler, Make, required development metadata, Debian packaging tools, and Markdown linters. Compilation was therefore not validated. An existing ignored x86-64 executable was used only for isolated CLI observations; it is not release evidence.

## Safe configuration validation

Use a disposable home so tests do not overwrite personal settings. Do not create `.config` in advance; the save command should create all missing parents:

```sh
test_home=$(mktemp -d)
env HOME="$test_home" ./eyeki --show-config
env HOME="$test_home" ./eyeki --set-interval 10
env HOME="$test_home" ./eyeki --set-mode n
env HOME="$test_home" ./eyeki --show-config
stat -c '%a %n' "$test_home/.config/eye_reminder" \
  "$test_home/.config/eye_reminder/config"
```

Expect modes `700` and `600`. Repeat with an absolute disposable `XDG_CONFIG_HOME` and confirm the file is created below that directory. If a legacy HOME config exists and the XDG file does not, `--show-config` should read the legacy values; the next successful setting change should create the XDG file without deleting or changing the legacy file.

For the invalid-input path, first record the disposable config contents, run a value such as `--set-interval 9`, and confirm the command exits nonzero and the file is unchanged. Repeat with `300` to exercise the upper accepted boundary without starting the daemon.

Inspect only this temporary directory. Remove it manually when finished. Do not paste a real home path or journal containing unrelated session data into issues.

## Safe timer, notification, and popup validation

Run UI checks only in a disposable desktop test account/session when possible.

1. Build and run the isolated configuration steps above with the minimum production interval of ten minutes.
2. Start `env HOME="$test_home" ./eyeki --daemon` inside the graphical session.
3. In notification mode, remain active until one reminder is expected; verify a single notification and record whether the server honors the timeout.
4. Restart in popup mode (`--set-mode p` before launch); verify button and keyboard activation, window-manager close, focus, scaling, right-to-left text, and multi-monitor behavior.
5. Test idle/resume, direct launch and user-service launch, another user's concurrent session, an SSH/TTY session, multiple active graphical sessions, and a missing/inaccessible logind service. Confirm that only the intended eligible session counts time and that missing, ambiguous, or failed lookup states reset progress without repeated logs.
6. While the notification-mode daemon is counting, change the interval and confirm the old progress does not produce a reminder; counting restarts from the successful command. The config-watch/runtime unit tests provide a fast deterministic version of this check.
7. Restart after a notification-to-popup mode change until runtime backend initialization is fixed.
8. Stop the process explicitly and inspect only EyeKi-related stderr/journal entries.

The production CLI rejects intervals below ten minutes. Use the seconds-based scheduler test interface to inject shorter durations for automated tests; do not weaken production validation to accelerate a desktop test.

## Debugging

- `systemctl --user status eyeki.service` shows unit state and recent output.
- `journalctl --user -u eyeki.service` shows service stderr, including the startup interval/mode.
- `ldd ./eyeki` shows resolved runtime libraries for a local ELF build.
- EyeKi distinguishes no eligible session, ambiguous eligible sessions, and session/idle query failure in stderr without printing identifiers. All three reset the timer; use `loginctl` only with sanitized output when diagnosing eligibility.
- Popup startup needs a working GTK display. Notification delivery needs the desktop's notification service/session bus even though idle lookup uses the system bus.

## Build and runtime variables

| Variable | Scope | Current behavior |
| --- | --- | --- |
| `CC`, `CPPFLAGS`, `CFLAGS`, `LDFLAGS`, `PKG_CONFIG` | Make | Standard tool/compiler overrides; warning flags are appended |
| `PREFIX`, `DESTDIR`, `BINDIR`, `SYSTEMD_USER_DIR` | Make | Installation/staging paths |
| `HOME` | Runtime | Base for the default and legacy config path |
| `XDG_CONFIG_HOME` | Runtime | Absolute override for the config base; relative values are ignored |
| `DISPLAY`, `WAYLAND_DISPLAY` | GTK environment | Selected by GTK; EyeKi does not read them directly |
| `DBUS_SESSION_BUS_ADDRESS` | libnotify environment | Usually inherited from the desktop session; not read directly |

No EyeKi-specific environment variables exist.

## Common failures

- **`pkg-config` cannot find a module:** install the matching development package and confirm the `.pc` search path.
- **Undefined `sd_bus_*`:** confirm the Makefile requests and links `libsystemd`, not `dbus-1`.
- **A setting command reports a filesystem error:** inspect ownership and permissions only for the disposable/active XDG config path; saves do not fall back to HOME when an absolute XDG override is selected.
- **Changed settings appear stale:** confirm the daemon can watch the selected XDG/HOME path. A filesystem watch failure is reported through stderr/the user journal and terminates the process rather than continuing with stale settings.
- **Popup after a live notification-to-popup change fails:** restart in popup mode so GTK initializes before use.
- **Installed service cannot find the binary:** keep `ExecStart` aligned with `BINDIR`/`PREFIX`.
- **Wayland behavior differs:** GTK can render through Wayland, but stacking/focus/fullscreen policy remains compositor-controlled and unverified.

## Platform notes

- Linux and systemd-logind are hard dependencies of the current activity provider.
- The bundled unit is a user service, not a system service.
- Popup behavior must be verified separately on each claimed desktop environment and display server.
- Sandboxed packages need deliberate system-bus and notification permissions; do not broaden access without review.
- macOS and Windows require replacement activity, notification, lifecycle, and packaging adapters—not just conditional compilation.
