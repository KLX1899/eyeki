# Project context

## Product purpose and intended users

EyeKi is a local Linux desktop reminder for people who want periodic prompts to apply eye drops. It counts time while a desktop session appears active, then uses either a desktop notification or a fullscreen acknowledgement popup. The repository specifies no diagnostic, treatment, dosage, clinical-validation, or medical-device functionality.

Likely users are Linux desktop users whose healthcare guidance already tells them when and how to use eye drops. The initial release target is Ubuntu. Other Linux distributions and macOS are later possibilities, not current support claims. This intended audience is a product interpretation consistent with the code; the repository contains no formal user research or product specification.

## Confirmed current capabilities

- A single C process provides CLI handling, polling, persistence, scheduling, and presentation through a small set of C modules.
- The configurable interval defaults to 60 minutes and accepts whole decimal values from 10 through 300 minutes.
- The default mode is a GTK 3 fullscreen popup; notification mode uses libnotify.
- The loop polls every ten seconds and uses monotonic elapsed time while reported idle time is below 60 seconds. Reaching the idle threshold or losing a trustworthy idle-state result resets accumulated active time.
- Settings persist as `interval=<integer>` and `mode=popup|notification` under `$HOME/.config/eye_reminder/config`.
- CLI operations are `--help`, `--version`, `--show-config`, `--set-interval <min>`, `--set-mode n|p`, and `--daemon`.
- systemd-logind supplies idle metadata through system D-Bus.
- A systemd user unit and preliminary Debian metadata exist locally.

The repository provides scheduler and interval/config unit tests but no broader test suite, settings UI, snooze, reminder history, tray icon, network service, telemetry, localization catalog, automatic update mechanism, release automation, or verifiable published release.

EyeKi is intentionally configured through the CLI. A graphical settings application is not a product goal. Presentation remains an exclusive choice: notification XOR popup. The two modes must never be emitted together for one reminder.

## Non-goals for the current implementation

These are boundaries inferred from absent code and the present architecture, not permanent product decisions:

- Medical decision support, dosage tracking, adherence records, or health-data storage.
- Mobile, web, macOS, or Windows support.
- Cloud synchronization, user accounts, analytics, or remote notifications.
- Monitoring screen content, applications, keyboard input, or detailed activity history.

Changing any of these boundaries requires explicit product, privacy, security, and architecture review.

## Current technical state

The project is an early Linux prototype with separate configuration and scheduler modules plus preliminary local build/packaging files. It has no release tags or CI (automated builds and checks that run on repository changes). Scheduler and interval/config tests exist but have no current pass evidence because the active environment lacks a compiler and Make. The existing ignored binary demonstrates the older one-shot CLI but is not a trustworthy release artifact. A one-shot CLI command performs one operation, such as changing or printing a setting, and then exits; it does not enter the long-running reminder loop. The source archive and Debian metadata are not release-ready.

Important correctness gaps include fragile session selection, silent persistence/notification failures, delayed settings reload, permissive mode parsing, and unsafe runtime switching into popup mode. “Fragile session selection” means EyeKi accepts the first session returned by logind instead of proving that it is the current user's relevant graphical session. On a machine with two signed-in users, an SSH session, or multiple seats, the first result may belong to somebody else, so EyeKi may reset or advance the timer using the wrong person's idle state. Elapsed scheduling and logind idle timestamps use a monotonic clock, and interval input is now range-checked before overflow-safe conversion, but the implementation still needs build and runtime evidence on supported Ubuntu sessions.

See [Architecture](ARCHITECTURE.md), [Repository audit](REPOSITORY_AUDIT.md), and [Roadmap](ROADMAP.md).

## Terminology

- **Polling:** periodically waking the process (currently every ten seconds), asking logind for the idle state, and updating the scheduler from that sample. EyeKi does not receive a continuous stream of input events.
- **Active time:** elapsed time accumulated between polling samples while `get_idle_seconds()` reports less than 60 seconds of idle time. This is an approximation, not verified keyboard/mouse or screen-on time.
- **Idle threshold:** the hard-coded 60-second boundary that discards accumulated active time.
- **Reminder interval:** configured whole minutes of accumulated active time before a prompt.
- **Notification mode:** a libnotify message with a requested ten-second timeout.
- **Popup mode:** a fullscreen GTK window whose button sets a dismissal flag.
- **Daemon mode:** the foreground infinite loop. EyeKi does not daemonize itself; systemd supervises it when installed.
- **Configuration:** the local two-line plain-text file, not compile-time settings.
- **Presentation backend:** the local implementation used to present a reminder—currently GTK for popup or libnotify for notification. “Backend” here does not mean a web server or remote service.
- **XDG configuration path:** the freedesktop.org convention that uses `$XDG_CONFIG_HOME` when set and otherwise defaults to `$HOME/.config`. Adopting it would give EyeKi a standard, overrideable config location and requires a migration plan for the current file.
- **Atomic owner-private write:** write a complete new config to a temporary file accessible only to its owner, flush/check it, and atomically rename it over the old file. Readers then see either the old complete file or the new complete file, not a half-written file; symlink and permission checks are still required.

## Confirmed design decisions

- Linux desktop integration currently depends on systemd-logind, GTK 3, and libnotify.
- Reminder presentation is always exclusive: exactly one of notification or popup is stored and used. Combining them is not a future product goal.
- Reaching the idle threshold means the user is considered away; accumulated active time is discarded and the next active period starts from zero.
- Settings can be changed through one-shot CLI invocations while another process is running.
- Every successful settings change must be observed promptly by the running process and reset accumulated active time to zero. Counting then restarts under the complete new configuration; elapsed time is not preserved or reduced modulo the new interval.
- Accepted production intervals are whole minutes from 10 through 300 (five hours), inclusive. Faster automated tests should inject time or use a test-only scheduler interface rather than expose sub-minute production settings.
- If idle-state lookup fails, discard accumulated active time, restart counting from zero after state can be determined again, and emit a rate-limited diagnostic. Repeated failures must not cause reminder storms or noisy logs.
- Product settings remain CLI-only; no graphical settings UI is planned.
- Persian is the initial product language. Other languages and user-defined reminder text are later work.
- The first public release target is Ubuntu. Other Linux distributions and macOS require separate future platform work and evidence.
- The application remains local-only, with no telemetry, cloud synchronization, or persisted activity/reminder history planned.
- The authoritative upstream development version is `0.1.0`, exposed by `eyeki --version`. It remains unreleased until a verified tag and release artifacts exist; Debian packaging maps it to the unreleased revision `0.1.0-1`.
- The systemd unit restarts the process on failure.
- The application is MIT-licensed according to the root `LICENSE` file.

## Recommendations, not commitments

- Establish a reliable monotonic scheduler and current-session resolver before polishing packages.
- Separate configuration, scheduling, platform integration, and presentation into testable modules.
- Adopt XDG paths, validation, atomic owner-private writes, and explicit errors.
- Add a localization boundary before adding languages or customizable text: move Persian strings out of control flow, support RTL layout, define fallback behavior, and make strings extractable. Accessibility work means keyboard operation, screen-reader labels, focus order, scaling, contrast, and a reliable dismissal path for reminder UI; it does not imply adding a graphical settings screen.
- Keep the application local-only and minimize persisted information.
- Treat each distro package, Flatpak, and Snap as a separate deliverable after a tested upstream release exists. Each format has different metadata, dependency, sandbox-permission, update, signing, and store-review requirements, so one passing package does not prove the others work.

## Open questions

- Which Ubuntu releases, desktop environments, display servers, and architectures are included in the first support matrix?
- What should the eventual fallback language be when additional locales are introduced?
- How should “strong acknowledgement” balance interruption with accessibility and user control? The product wants one fullscreen window per monitor and dismissal through the acknowledgement button, but a normal desktop application cannot reliably or appropriately disable compositor/OS shortcuts such as Super+Tab. Absolute lock-in would require an explicitly managed kiosk session, not ordinary Ubuntu desktop behavior.
- What maintainer contact and supported-version policy should security documentation publish?

## Constraints

- GTK 3 and the current global/manual event-loop design make it harder to replace or independently test the two local presentation backends (popup and notification), handle multiple monitor windows, or port presentation to another OS. This is unrelated to a web backend.
- logind session idle reporting depends on desktop/session integration and may not represent actual input activity.
- Distribution sandboxes such as Snap and Flatpak may restrict logind queries on the system bus. The initial Ubuntu `.deb` can avoid that sandbox mismatch; later sandboxed packages need narrowly reviewed permissions or a different activity provider and must be tested against current store policy.
- Scheduler regression tests describe injected short intervals, threshold, idle reset, unknown-state reset, and clock-discontinuity behavior. Interval/config tests describe production boundaries, malformed values, and seconds conversion. They have not run in the active environment and are not enforced by CI; other behavior still lacks repeatable automated evidence.
- At the documentation audit, some packaging artifacts were ignored or untracked. Such files may be missing from a clone, may contain stale/generated machine-specific content, and cannot be reliably reviewed or reproduced. Authoritative packaging source must be tracked; generated binaries and archives must remain excluded and be rebuilt from a clean tagged checkout.

## Start here for a new AI session

1. Read root `AGENTS.md`, this document, and the task-relevant document under `docs/`.
2. Inspect `git status`, then read `eyeki.c`, `config.h`, `Makefile`, and affected packaging files.
3. Verify available dependencies and commands; do not reuse historical pass claims.
4. State uncertain product/platform assumptions before they influence behavior.
5. Make a scoped change, validate success and failure paths, update documentation, and review the final diff.

Reusable task prompts and handoff format are in [AI workflow](AI_WORKFLOW.md).
