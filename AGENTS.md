# AGENTS.md

Operational instructions for AI coding agents working in this repository. Read this file, `docs/PROJECT_CONTEXT.md`, and the task-relevant document before editing.

## Repository map

- `src/eyeki.c` — CLI parsing, timer loop, libnotify reminder, and GTK popup.
- `src/activity.c`, `src/activity.h` — current-user logind resolution and idle lookup.
- `src/activity_selection.c`, `src/activity_selection.h` — testable multi-session selection policy.
- `src/config.c`, `src/config.h` — `Config`, defaults, XDG/legacy path selection, loading, and atomic saving.
- `src/config_watch.c`, `src/config_watch.h` — inotify-based configuration replacement monitoring.
- `src/runtime.c`, `src/runtime.h` — installed runtime configuration and scheduler state.
- `src/scheduler.c`, `src/scheduler.h` — monotonic active-time state machine, independent of desktop libraries.
- `src/version.h` — authoritative upstream development version.
- `tests/` — desktop-independent scheduler, configuration, reload, watch, and session-selection regression tests.
- `Makefile` — build plus staged/system installation; output is `eyeki`.
- `eyeki.service` — systemd user unit expecting `/usr/bin/eyeki`.
- `install.sh` — stale installer; do not run or use as authoritative guidance.
- `debian/` — preliminary Debian metadata; not release-ready.
- `README.md`, `docs/`, `CONTRIBUTING.md`, `SECURITY.md` — user, contributor, architecture, policy, and release documentation.
- `.github/` — GitHub issue and pull-request templates.
- `EyeKi` and `*.orig.tar.gz` — local/generated artifacts; never edit or commit them.

There are no nested agent instructions, tests, CI workflows, localization catalogs, application assets, or dependency lock files.

## Architecture and data flow

EyeKi is a single-process, single-threaded C program. `main()` loads the XDG configuration path (falling back to the legacy `$HOME/.config/eye_reminder/config` when needed), executes a one-shot CLI command or initializes one presentation backend, then polls every ten seconds. `activity_get_idle_seconds()` resolves local systemd-logind metadata and queries the selected session over system D-Bus. Active monotonic elapsed time is accumulated until the configured threshold; the process then calls either `send_notification()` or blocking `show_popup()` and resets the counter.

Important invariants and fragility:

- Only one mode is active. Do not document or implement “both” without defining persistence, initialization, and tests.
- GTK is initialized only when startup mode is popup. Hot-switching from notification to popup currently violates that requirement.
- Settings reload only after the old threshold fires. Timer changes are not immediate.
- Idle detection accepts only an active, local, graphical user session owned by the process effective UID. A process-bound session is authoritative; user services prefer logind's primary display, use a sole eligible fallback, and reject unresolved ambiguity.
- Missing, ambiguous, or failed session/idle lookups reset accumulated active time and produce transition-only diagnostics without identifiers.
- The popup owns a manual GTK event loop and exits only when `popup_dismissed` changes.
- The timer uses `CLOCK_MONOTONIC`; idle or unknown activity state resets accumulated active time.

See `docs/ARCHITECTURE.md` for diagrams and detailed flows.

## Configured commands and audit evidence

Repository-defined workflow:

```sh
pkg-config --modversion gtk+-3.0 libnotify libsystemd
make
./eyeki --help
./eyeki --show-config
./eyeki --daemon
make clean
```

Installation/staging:

```sh
make DESTDIR="$PWD/package-root" install
sudo make install
systemctl --user daemon-reload
systemctl --user enable --now eyeki.service
```

At the 2026-08-14 documentation audit, the existing ignored x86-64 binary successfully ran `--help`, default `--show-config`, valid setting updates when an isolated `$HOME/.config` existed, and invalid-option paths. The host lacked a compiler, Make, development `.pc` files, Debian tools, and documentation linters, so compilation and packaging were not executed. Never convert this historical result into a current pass claim; run commands in the active environment.

`make test` runs scheduler, runtime, config-watch, interval/config-persistence, and activity-session-selection unit tests. There are no configured lint, format, type-check, documentation-check, integration-test, or CI targets. Do not invent a passing check. For C changes, at minimum build with warnings enabled by the Makefile, run available unit tests, and manually exercise affected CLI/desktop behavior.

## Code conventions

- C with four-space indentation, braces on the same line, descriptive comments, and `snake_case` functions/variables.
- Keep warnings enabled (`-Wall -Wextra`). Avoid broad formatting churn.
- Use bounded path construction and check return values. New code should not copy the current silent-error pattern.
- Keep public headers limited to types and declarations; place externally linked function bodies in `.c` files.
- User-facing text is currently hard-coded Persian. Do not add more hard-coded strings; plan a localization boundary.
- Preserve lowercase installed names: binary `eyeki`, unit `eyeki.service`, project display name `EyeKi`.

## Change rules

### Timers and idle detection

- Use a monotonic clock for elapsed duration; wall time is unsuitable.
- Select the session belonging to the current process/user and define behavior for multiple sessions.
- Decide explicitly whether lookup failure pauses or advances time; surface diagnosable errors without log spam.
- Validate interval range and overflow before persisting or converting minutes to seconds.
- Test threshold crossing, idle/resume, clock discontinuity, changed settings, and failure behavior.

### Popup and notifications

- Initialize each backend before it can be selected, including after a runtime config reload.
- Never assume fullscreen, focus, or keep-above hints are honored on every compositor.
- Handle popup window-manager close and notification initialization/show failures.
- Keep reminders dismissible and keyboard accessible. Avoid coercive UI changes without product review.
- Manual desktop tests must cover notification mode, popup button dismissal, window-manager close, X11/Wayland claims, and missing notification/display services.

### Settings and persistence

- Current format is two plain-text `key=value` lines at `$XDG_CONFIG_HOME/eye_reminder/config` when the override is absolute, or `$HOME/.config/eye_reminder/config` otherwise.
- Preserve unknown keys if evolving the format, or introduce an explicit migration/version strategy.
- Preserve the non-destructive legacy fallback when changing XDG path behavior; create parent directories safely and write atomically with owner-only permissions.
- Report save/parse failures. Never log unrelated environment values or config file contents.
- Restart the daemon during manual validation after setting changes until hot reload is fixed.

### Platform integration

- Current support boundary is Linux + systemd-logind + graphical desktop. Do not claim generic Unix, Windows, macOS, or verified Wayland support.
- Keep the Makefile, `eyeki.service`, Debian metadata, README, and release guide consistent on paths, dependencies, and version.
- `install.sh` references obsolete uppercase paths and must remain unused until deliberately redesigned.

## Security, privacy, and generated files

- The application should remain offline unless a separately reviewed feature explicitly requires networking.
- Treat session metadata and reminder content as local/private. Do not add telemetry, identifiers, medical history, or verbose session logging by default.
- Do not commit local configuration, logs, binaries, archives, credentials, private contact data, or machine-specific paths.
- Do not manually edit compiled `EyeKi`/`eyeki` binaries or generated source archives.
- Review OS permissions, notification content exposure, config permissions, and dependencies for security-impacting changes.

## Testing and documentation expectations

Before completion:

1. Inspect `git diff` and `git status`; preserve unrelated work.
2. Run dependency discovery and `make`; record exact failures if the environment blocks them.
3. Run available automated tests/checks. If none exist, say so and perform scoped manual validation.
4. For timer/notification/popup changes, test in a disposable config home and a real supported desktop session without exposing personal data.
5. Update README/user behavior, architecture/data flow, development commands, privacy/security, roadmap/release notes, and changelog as applicable.
6. Check internal Markdown links and ensure examples use supported options.
7. Confirm no generated artifacts, secrets, personal paths, or unsupported claims entered the diff.

A change is done only when code/build metadata, tests or explicit manual evidence, failure handling, platform impact, documentation, and clean repository hygiene all agree. Known blockers must be recorded rather than silently waived.
