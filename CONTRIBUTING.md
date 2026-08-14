# Contributing to EyeKi

Thank you for helping improve EyeKi. The project is an early prototype, so correctness, clear scope, and reproducible evidence are more valuable than large changes.

Read [AGENTS.md](AGENTS.md), [Project context](docs/PROJECT_CONTEXT.md), and [Development](docs/DEVELOPMENT.md) before changing code. The policies below are proposed maintainer policy until a maintainer formally confirms them.

## Report bugs and propose features

Use the repository's issue templates for reproducible bugs and feature proposals. Search existing issues first. Include the EyeKi version or commit, OS/distribution, desktop environment, display server if known, reminder mode, interval, launch method, and minimal reproduction steps.

Share only EyeKi-related logs and the non-sensitive output of `eyeki --show-config`. Remove usernames, home paths, hostnames, session identifiers, unrelated journal entries, notification contents you consider private, and all credentials.

Do not file a public issue for a suspected vulnerability. Follow [SECURITY.md](SECURITY.md).

Feature proposals should explain the user problem and privacy/accessibility/platform effects before prescribing an implementation. Medical or health-data features need especially careful scope review.

## Development setup

Install a C toolchain, Make, `pkg-config`, and development files for GTK 3, libnotify, and libsystemd. Then:

```sh
pkg-config --modversion gtk+-3.0 libnotify libsystemd
make
./eyeki --help
```

Use [the disposable-home workflow](docs/DEVELOPMENT.md#safe-configuration-validation) for setting tests. Do not use `install.sh`.

## Branches and commits

Proposed policy:

- Branch from the maintainer-designated default branch and keep each branch focused on one concern.
- Use short, imperative commit subjects; explain rationale and user-visible effects in the body when needed.
- Do not mix formatting, generated artifacts, packaging, and behavior changes without a clear dependency.
- Do not rewrite another contributor's work or commit binaries, local configuration, logs, source archives, or credentials.

## Code style

Match nearby C: four-space indentation, same-line opening braces, `snake_case` identifiers, and focused comments explaining constraints rather than restating syntax. Keep `-Wall -Wextra` clean. Avoid mass reformatting because no formatter is configured.

Check every library/system call you add. Validate external/configuration values before arithmetic. Avoid new global state and hard-coded user-facing strings. If adding a second C translation unit, move function definitions out of `config.h` first to prevent duplicate symbols.

## Tests and verification

There is no automated test suite yet. That is a limitation, not permission to skip evidence. Run all available relevant checks and report exact results:

```sh
make clean
make
./eyeki --help
./eyeki --show-config
```

Use isolated configuration and a real desktop session for changes to reminders. Timer changes should cover threshold, idle/resume, invalid values, clock behavior, reload/restart behavior, and D-Bus failure. Notification/popup changes should cover initialization failure, dismissal/close, keyboard use, focus, scaling, and each claimed desktop/display server.

If the environment lacks dependencies or a graphical session, state what was not run and why. Never claim a pass based only on code inspection or an older binary.

## Pull requests

A pull request should:

- State the problem, scope, approach, and user-visible behavior.
- Link relevant issues and call out assumptions or decisions needing maintainer confirmation.
- List exact commands and manual scenarios run, with outcomes.
- Identify untested platforms and remaining risks.
- Include focused screenshots for visual changes with personal desktop details removed.
- Keep documentation, service/package metadata, privacy/security notes, and changelog synchronized.
- Contain no unrelated changes, generated binaries/archives, secrets, or machine-specific paths.

Small, reviewable pull requests are preferred. Architectural refactors should first define boundaries and regression tests.

## Cross-cutting review

- **Accessibility:** preserve keyboard dismissal, readable contrast/scaling, assistive-technology semantics, and user control. A forced fullscreen popup deserves explicit review.
- **Localization:** current Persian strings are hard-coded. Do not add more untranslatable UI; consider right-to-left layout, pluralization, and fallback language.
- **Privacy:** retain local-only operation and data minimization. Review config permissions, logs, session metadata, and lock-screen notification exposure.
- **Platform compatibility:** claim only tested Linux desktop/display-server combinations. systemd-logind is currently required.
- **Packaging:** align binary/unit paths, dependencies, version, license metadata, and sandbox permissions across every format.

## Contributor checklist

- [ ] I read `AGENTS.md` and task-relevant documentation.
- [ ] My change is focused and preserves unrelated work.
- [ ] I ran available builds/checks and recorded exact results.
- [ ] I tested relevant success, failure, timer, and desktop scenarios.
- [ ] I updated user, architecture, privacy/security, release, and changelog docs where needed.
- [ ] I made no unsupported platform/release claim.
- [ ] I added no secrets, personal data, machine paths, binaries, archives, or unrelated logs.
