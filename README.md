# EyeKi

EyeKi is a small Linux reminder daemon that counts active session time and prompts the user to apply eye drops. It can send a desktop notification or display a fullscreen GTK popup.

> **Status:** early, pre-release prototype. There are no verifiable tagged releases, automated tests, CI workflows, or production-ready packages. Configuration validation, session detection, localization, and packaging need work before general distribution. See the [roadmap](docs/ROADMAP.md).

EyeKi only provides configurable reminders; the repository contains no clinical validation or medical guidance. Users should choose an interval appropriate to guidance from their healthcare professional.

## Implemented features

- Counts time in ten-second polling cycles and pauses accumulation when the selected logind session reports at least 60 seconds of idle time.
- Uses either a ten-second libnotify desktop notification or a fullscreen GTK 3 popup that requires a button click.
- Stores the interval and reminder mode in a local plain-text configuration file.
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
./eyeki --show-config
./eyeki --daemon
```

`make clean` removes the local `eyeki` build output. The build command is defined by the repository Makefile, but it is not yet exercised by CI.

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

Use a whole number from 10 through 300 minutes (five hours). This is the confirmed product range, although the current prototype does not enforce it yet:

```sh
./eyeki --set-interval 60
./eyeki --set-mode n
./eyeki --set-mode p
./eyeki --show-config
```

`n` selects desktop notifications and `p` selects the fullscreen popup. Settings are stored at `$HOME/.config/eye_reminder/config`; XDG configuration overrides are not supported. The parent `$HOME/.config` directory must already exist or the current save implementation silently fails.

The intended behavior is that every successful settings change takes effect promptly and resets accumulated active time to zero, after which counting restarts under the new complete configuration. The current prototype does not implement that behavior: it reloads only when the old interval reaches its trigger point, and switching from notification to popup while running is unsafe because GTK is initialized only for popup startup. Restart EyeKi after changing either setting until live reload is fixed.

Persian is the initial product language. The current messages are hard-coded and mention one hour even when a different interval is configured; future localization work must separate text from program logic and handle RTL/accessibility before adding other languages or custom text. There is no snooze action, history, combined presentation mode, graphical settings screen, or automatic updater.

## Troubleshooting

- **Settings report success but revert:** make sure `$HOME/.config` exists and check `$HOME/.config/eye_reminder/config`.
- **No notification appears:** confirm the desktop notification service is running and inspect stderr or the user journal. Delivery failures are currently not surfaced by EyeKi.
- **Popup cannot open:** start EyeKi inside the intended graphical session and check the display environment. GTK initialization can terminate when no display is available.
- **Timer advances while away:** current D-Bus logic inspects the first logind session, not reliably the process owner's active session. D-Bus failures are treated as active time.
- **Service loops or fails:** use `systemctl --user status eyeki.service` and `journalctl --user -u eyeki.service`; the unit restarts failures after five seconds.
- **Build dependency errors:** verify all three `pkg-config` modules with `pkg-config --modversion gtk+-3.0 libnotify libsystemd`.

## Privacy and security

The source contains no network client or telemetry. It stores only the interval and selected mode locally, queries session idle metadata over the system bus, and sends reminder content to the local desktop notification service. Startup logs include the configured interval and mode. See [Privacy](docs/PRIVACY.md) and [Security](SECURITY.md) for limitations and reporting guidance.

## Known limitations

- No automated tests, linting, formatting checks, CI, or verified release process.
- Numeric configuration is not validated; zero, negative, or overflowing intervals can cause incorrect behavior.
- Idle detection can select the wrong login session and treats lookup errors as activity.
- Wall-clock changes can distort reminder timing.
- Closing the popup through the window manager can leave its manual event loop running.
- Notification errors and configuration-write errors are ignored.
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
