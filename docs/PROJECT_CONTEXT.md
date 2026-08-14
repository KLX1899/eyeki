# Project context

## Product purpose and intended users

EyeKi is a local Linux desktop reminder for people who want periodic prompts to apply eye drops. It counts time while a desktop session appears active, then uses either a desktop notification or a fullscreen acknowledgement popup. The repository specifies no diagnostic, treatment, dosage, clinical-validation, or medical-device functionality.

Likely users are Linux desktop users whose healthcare guidance already tells them when and how to use eye drops. This intended audience is a product interpretation consistent with the code; the repository contains no formal user research or product specification.

## Confirmed current capabilities

- A single C process provides CLI handling, polling, persistence, and presentation.
- The configurable interval defaults to 60 minutes.
- The default mode is a GTK 3 fullscreen popup; notification mode uses libnotify.
- The loop polls every ten seconds and accumulates elapsed wall time when reported idle time is below 60 seconds.
- Settings persist as `interval=<integer>` and `mode=popup|notification` under `$HOME/.config/eye_reminder/config`.
- CLI operations are `--help`, `--show-config`, `--set-interval <min>`, `--set-mode n|p`, and `--daemon`.
- systemd-logind supplies idle metadata through system D-Bus.
- A systemd user unit and preliminary Debian metadata exist locally.

The repository provides no test suite, settings UI, snooze, reminder history, tray icon, network service, telemetry, localization catalog, automatic update mechanism, release automation, or verifiable published release.

## Non-goals for the current implementation

These are boundaries inferred from absent code and the present architecture, not permanent product decisions:

- Medical decision support, dosage tracking, adherence records, or health-data storage.
- Mobile, web, macOS, or Windows support.
- Cloud synchronization, user accounts, analytics, or remote notifications.
- Monitoring screen content, applications, keyboard input, or detailed activity history.

Changing any of these boundaries requires explicit product, privacy, security, and architecture review.

## Current technical state

The project is an early Linux prototype with two tracked source files and preliminary local build/packaging files. It has no release tags or CI. The existing ignored binary demonstrates the one-shot CLI but is not a trustworthy release artifact. The source archive and Debian metadata are not release-ready. Important correctness gaps include unvalidated intervals, fragile session selection, silent persistence/notification failures, wall-clock timing, and unsafe runtime switching into popup mode.

See [Architecture](ARCHITECTURE.md), [Repository audit](REPOSITORY_AUDIT.md), and [Roadmap](ROADMAP.md).

## Terminology

- **Active time:** elapsed polling time added while `get_idle_seconds()` returns less than 60. This is not verified keyboard/mouse or screen-on time.
- **Idle threshold:** the hard-coded 60-second boundary that pauses accumulation.
- **Reminder interval:** configured whole minutes of accumulated active time before a prompt.
- **Notification mode:** a libnotify message with a requested ten-second timeout.
- **Popup mode:** a fullscreen GTK window whose button sets a dismissal flag.
- **Daemon mode:** the foreground infinite loop. EyeKi does not daemonize itself; systemd supervises it when installed.
- **Configuration:** the local two-line plain-text file, not compile-time settings.

## Confirmed design decisions

- Linux desktop integration currently depends on systemd-logind, GTK 3, and libnotify.
- Only one presentation mode is stored and used at a time.
- Away time is intended not to count toward reminders.
- Settings can be changed through one-shot CLI invocations while another process is running.
- The systemd unit restarts the process on failure.
- The application is MIT-licensed according to the root `LICENSE` file.

## Recommendations, not commitments

- Establish a reliable monotonic scheduler and current-session resolver before polishing packages.
- Separate configuration, scheduling, platform integration, and presentation into testable modules.
- Adopt XDG paths, validation, atomic owner-private writes, and explicit errors.
- Add localization and accessibility design before expanding user-facing UI.
- Keep the application local-only and minimize persisted information.
- Treat distro, Flatpak, and Snap work as separate targets after a tested upstream release exists.

## Open questions

- What minimum/maximum interval should be accepted, and should sub-minute testing be supported separately?
- Should idle lookup failure pause reminders, continue counting, or enter a visible degraded state?
- Should popup and notification modes ever be combined?
- Must settings update immediately without restarting the service?
- Which Linux distributions, desktop environments, display servers, and architectures are release targets?
- Is Persian the initial product locale, and what is the fallback language?
- Is a forced fullscreen acknowledgement acceptable for accessibility and user-control goals?
- What maintainer contact and supported-version policy should security documentation publish?
- Which version is authoritative: the archive's `1.0` label or Debian metadata's `1.0.0-1`?

## Constraints

- GTK 3 and the current global/manual event-loop design limit backend evolution.
- logind session idle reporting depends on desktop/session integration and may not represent actual input activity.
- Distribution sandboxes may restrict system-bus session queries.
- There is no automated regression evidence.
- Existing packaging files were untracked or ignored at the documentation audit and may not reflect committed upstream state.

## Start here for a new AI session

1. Read root `AGENTS.md`, this document, and the task-relevant document under `docs/`.
2. Inspect `git status`, then read `eyeki.c`, `config.h`, `Makefile`, and affected packaging files.
3. Verify available dependencies and commands; do not reuse historical pass claims.
4. State uncertain product/platform assumptions before they influence behavior.
5. Make a scoped change, validate success and failure paths, update documentation, and review the final diff.

Reusable task prompts and handoff format are in [AI workflow](AI_WORKFLOW.md).
