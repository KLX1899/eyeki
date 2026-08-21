# EyeKi

EyeKi is a small Linux reminder daemon that counts active session time and prompts the user to apply eye drops. It can send a desktop notification or display a fullscreen GTK popup.

> **Status:** early, pre-release prototype. Scheduler, runtime-reload, and configuration unit tests exist, but there are no verifiable tagged releases, CI workflows, or production-ready packages. Session detection, presentation lifecycle, localization, and packaging need work before general distribution. See the [roadmap](docs/ROADMAP.md).

EyeKi only provides configurable reminders; the repository contains no clinical validation or medical guidance. Users should choose an interval appropriate to guidance from their healthcare professional.

## Implemented features

- Counts time in ten-second polling cycles and discards accumulated active time when the selected logind session reports at least 60 seconds of idle time.
- Uses either a ten-second libnotify desktop notification or a fullscreen GTK 3 popup that requires a button click.
- Stores the interval and reminder mode in a local plain-text configuration file.
- Observes atomic settings replacements with inotify and restarts active-time counting from zero under the complete new configuration.
- Provides command-line operations to show and change those settings.
- Includes a systemd user unit and preliminary Debian packaging metadata.

Notification and popup are intentionally mutually exclusive modes (XOR), not capabilities intended to be combined. Notification is the gentler prompt; popup is the stronger acknowledgement flow. The `--daemon` option runs the same foreground loop as starting without arguments—it does not fork or detach.

Configuration is intentionally CLI-only. A graphical settings application is not planned.

## Screenshots

No screenshots are committed yet.

- `TODO(maintainer): Add a notification screenshot with personal desktop details removed.`
- `TODO(maintainer): Add popup screenshots from supported display-server sessions.`

## Platform support

The first public-release target is Ubuntu. The current implementation is Linux-specific and requires a graphical desktop session, system D-Bus, and `systemd-logind`. The source uses GTK 3, libnotify, and libsystemd. X11 and Wayland behavior has not been systematically tested; fullscreen and keep-above requests may be compositor-dependent. Other Linux distributions and macOS are possible later targets but are not currently supported. Windows is not supported by the current architecture.

## End-user installation goal

The intended Ubuntu release must be ready to use through one package-manager command, with runtime dependencies resolved automatically. End users should not need to install a compiler, development headers, or individual libraries manually. The project does not yet publish such a package, so the commands below are currently developer/source-build instructions rather than the finished installation experience.

## Source-build requirements

- A C compiler and Make
- `pkg-config`
- GTK 3 development files (`gtk+-3.0`)
- libnotify development files (`libnotify`)
- libsystemd development files (`libsystemd`)
- At runtime: a notification service for notification mode and an available GTK display for popup mode

The preliminary Debian metadata names `libgtk-3-dev`, `libnotify-dev`, `libsystemd-dev`, `pkg-config`, and debhelper as build dependencies. Package names differ across distributions.

## Build and run

```sh
make
./eyeki --help
./eyeki --version
./eyeki --show-config
./eyeki --daemon
```

`make clean` removes the local `eyeki` build output. The build command is defined by the repository Makefile, but it is not yet exercised by CI.

`make test` builds and runs the desktop-independent scheduler, runtime reload, config-watch, and interval/config-persistence unit tests.

For a system-wide install, inspect `eyeki.service` first, then run with suitable privileges:

```sh
make
sudo make install
systemctl --user daemon-reload
systemctl --user enable --now eyeki.service
```

This installs the binary under `/usr/bin` and the user unit under `/usr/lib/systemd/user` with the default Makefile settings. For development, run the binary directly instead of installing it. The existing `install.sh` is stale—it refers to a nonexistent uppercase source filename and should not be used.

See [Development](docs/DEVELOPMENT.md) for setup, diagnostics, safe manual validation, and the exact limitations of the current toolchain.

## Configuration and reminder behavior

The default configuration is:

```text
interval=60
mode=popup
```

Use a whole number from 10 through 300 minutes (five hours). Values outside that range, signs, whitespace, trailing characters, and numbers that cannot be represented are rejected without changing the saved configuration:

```sh
./eyeki --set-interval 60
./eyeki --set-mode n
./eyeki --set-mode p
./eyeki --show-config
```

`n` selects desktop notifications and `p` selects the fullscreen popup. If `XDG_CONFIG_HOME` is non-empty and absolute, settings are stored at `$XDG_CONFIG_HOME/eye_reminder/config`; otherwise EyeKi uses `$HOME/.config/eye_reminder/config`. Missing parents are created. The EyeKi directory is restricted to `0700`, and each complete configuration is written to a `0600` temporary file before an atomic replacement. A save error is printed and the setting command exits nonzero instead of reporting success.

When an XDG override is active but its EyeKi config does not exist, EyeKi reads the legacy `$HOME/.config/eye_reminder/config` file. The next successful setting change writes the complete configuration to the XDG path and leaves the legacy file untouched. Relative `XDG_CONFIG_HOME` values are ignored.

Every successful settings command atomically replaces the configuration file. The running daemon observes that replacement through inotify, promptly loads the complete new configuration, resets accumulated active time to zero, and starts counting under the new interval and mode. Rapid consecutive replacements may be coalesced into one reload of the latest complete configuration, which has the same reset result.

Runtime switching from notification mode to popup mode remains unsafe because GTK is initialized only when the daemon starts in popup mode. Restart EyeKi after switching into popup mode until presentation-backend initialization is fixed. Interval-only changes are live and do not require a restart.

Persian is the initial product language. The current messages are hard-coded and mention one hour even when a different interval is configured; future localization work must separate text from program logic and handle RTL/accessibility before adding other languages or custom text. There is no snooze action, history, combined presentation mode, graphical settings screen, or automatic updater.

## Troubleshooting

- **A setting command exits with a filesystem error:** check ownership and permissions for the active XDG config path, or `$HOME/.config` when no absolute XDG override is set.
- **No notification appears:** confirm the desktop notification service is running and inspect stderr or the user journal. Delivery failures are currently not surfaced by EyeKi.
- **Popup cannot open:** start EyeKi inside the intended graphical session and check the display environment. GTK initialization can terminate when no display is available.
- **Timer behaves unexpectedly around session changes:** current D-Bus logic inspects the first logind session, not reliably the process owner's active session. D-Bus failures reset the timer because activity cannot be established reliably.
- **Service loops or fails:** use `systemctl --user status eyeki.service` and `journalctl --user -u eyeki.service`; the unit restarts failures after five seconds.
- **Build dependency errors:** verify all three `pkg-config` modules with `pkg-config --modversion gtk+-3.0 libnotify libsystemd`.

## Privacy and security

The source contains no network client or telemetry. It stores only the interval and selected mode locally, queries session idle metadata over the system bus, and sends reminder content to the local desktop notification service. Startup logs include the configured interval and mode. See [Privacy](docs/PRIVACY.md) and [Security](SECURITY.md) for limitations and reporting guidance.

## Known limitations

- Scheduler, runtime reload, config-watch, and interval/config-persistence unit tests exist, but there is no broader automated coverage, linting, formatting check, CI, or verified release process.
- Invalid persisted interval values are ignored so they cannot replace the default or a preceding valid value; other malformed configuration fields are not diagnosed.
- Idle detection can select the wrong login session; lookup errors reset progress until detection recovers.
- A live notification-to-popup switch can reach GTK without initialization; restart the daemon after selecting popup mode.
- Closing the popup through the window manager can leave its manual event loop running.
- Notification errors and configuration-read/parse errors are ignored; concurrent one-shot settings changes can still overwrite one another's fields.
- Accessibility, right-to-left layout, translations, and Wayland behavior are untested.
- Debian packaging, the source archive, and the systemd integration are preliminary.

## Project documentation

- [Project context](docs/PROJECT_CONTEXT.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Development](docs/DEVELOPMENT.md)
- [Contributing](CONTRIBUTING.md)
- [Roadmap](docs/ROADMAP.md)
- [Release and distribution](docs/RELEASE_AND_DISTRIBUTION.md)
- [AI-assisted workflow](docs/AI_WORKFLOW.md)
- [Repository audit](docs/REPOSITORY_AUDIT.md)
- [Changelog](CHANGELOG.md)

## License

EyeKi is available under the [MIT License](LICENSE).
