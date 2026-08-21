# Roadmap

This roadmap is evidence-based planning guidance, not a commitment or release schedule. Priorities are **P0** (blocks trustworthy use/release), **P1** (important after P0 foundations), and **P2** (longer-term or product-dependent).

## Completed work

| Area and item | Priority | Completed | Evidence |
| --- | --- | --- | --- |
| Reliability — validate interval parsing/range and arithmetic | P0 | 2026-08-19 | Strict 10–300 minute parsing is shared by CLI/config loading; checked seconds conversion and boundary/invalid/test-injection regressions are included. |
| Settings — create XDG config parents and report/atomically handle writes | P0 | 2026-08-20 | Absolute XDG selection with non-destructive legacy fallback; private parent creation; synced atomic writes; CLI errors; round-trip, permission, migration, and failure regressions. |
| Reminder scheduling — use monotonic time and immediate, defined reload | P0 | 2026-08-20 | Monotonic runtime state plus inotify observation; each detected atomic settings replacement installs the complete config and resets active time; interval/mode/reset and watcher regressions are included. |

## Existing incomplete work

| Area and item | Motivation | Priority | Dependencies | Acceptance criteria |
| --- | --- | --- | --- | --- |
| Reliability — resolve the current user's active logind session | The first `ListSessions` result can belong to another user/session | P0 | Multiple-session policy | Correct UID/session selected in multi-session tests; empty/error states are distinguishable |
| Notifications — initialize both selectable backends safely and surface failures | Live notification-to-popup changes use GTK before initialization; notification errors are ignored | P0 | Reload policy | Every permitted mode transition is initialized/tested or rejected with clear restart guidance; delivery errors are diagnosable |
| User experience/accessibility — handle popup close and non-blocking lifecycle | Window-manager close can leave the manual event loop stuck | P0 | Presentation lifecycle design | Button, keyboard, WM close, process signal, and missing-display paths terminate predictably |
| Packaging — reconcile/track Makefile, user unit, Debian files, and installer | Packaging files were untracked/ignored; installer is stale; source archive is malformed | P0 | Version and supported-platform decisions | Authoritative tracked files build/package from clean checkout; stale installer removed or replaced in a reviewed change |
| Localization — replace hard-coded interval-specific Persian copy | Messages always say one hour and cannot be translated | P1 | Locale/fallback decision | Message reflects configured interval; strings are extractable; Persian RTL and fallback locale tested |

## Recommended short-term work

| Area and item | Motivation | Priority | Dependencies | Acceptance criteria |
| --- | --- | --- | --- | --- |
| Testing — isolate config parser and scheduler with unit tests | No automated regression evidence exists | P0 | Minimal module boundaries | Tests cover defaults, malformed files, ranges, thresholds, idle/resume, reload, and clock/error cases |
| CI/CD — build/test on a supported Linux baseline | Clean-checkout build is not continuously verified | P0 | Tests and declared baseline | CI installs declared dependencies, builds with warnings as errors in CI, runs tests/link checks, and rejects generated artifacts |
| Security/privacy — define private reporting channel and file permissions | Security contact is missing; config permissions depend on umask | P1 | Maintainer contact decision | `SECURITY.md` placeholder resolved; config/journal threat model tested; no unexpected data collection |
| Notifications — define unknown/permission/failure states | D-Bus and notification failures currently look like success/activity | P1 | Confirmed reset-on-idle-lookup-failure policy | Idle lookup failure resets active time; recovery restarts from zero; user receives rate-limited actionable diagnostics; no failure causes reminder storms |
| Settings — add safe service-aware update behavior | Concurrent CLI writes and daemon reload are unsynchronized | P1 | Atomic storage/reload | Concurrent updates do not lose fields; daemon observes a complete config under documented timing |
| Accessibility — audit fullscreen interaction, focus, scaling, contrast, assistive tech | Current popup is visually styled but unverified | P1 | Target desktops; UX decision | Keyboard/screen-reader/scale checks documented; always-available exit; no focus trap |
| Localization — add catalogs and translator workflow | No localization resources exist | P1 | Toolkit decision | Source locale, fallback, catalogs, translator instructions, and automated catalog validation exist |
| Packaging/distribution — produce one clean upstream source release | Distro work lacks a trustworthy upstream artifact/version | P1 | All P0 correctness; versioning | Signed/tagged version; reproducible source archive; checksum/SBOM/provenance; clean build instructions |
| Documentation — add sanitized screenshots and supported-platform matrix | Current UI/platform evidence is absent | P1 | Manual desktop tests | Screenshots contain no private data; matrix distinguishes tested, unsupported, and unknown combinations |
| Dependency maintenance — add automated update/advisory review | Native dependencies are unpinned and unreviewed | P1 | CI and policy | Regular advisory process, minimum versions if needed, and documented response workflow |

## Longer-term ideas

| Area and item | Motivation | Priority | Dependencies | Acceptance criteria |
| --- | --- | --- | --- | --- |
| Architecture — complete activity and presentation adapters | Scheduler/config extraction has started, but activity/presentation globals still impede testing and ports | P2 | Expanded regression suite | Core tests run without GTK/D-Bus; adapters have narrow interfaces; behavior remains documented |
| UX — optional snooze or other CLI controls | Could extend reminder control without adding a graphical settings application or combining presentation modes | P2 | Product research; accessibility; persistence schema | Explicit user stories, accessible design, migration, privacy review, and tests preserve CLI-only configuration and notification-XOR-popup behavior |
| Platform integration — evaluate modern GTK/desktop APIs | GTK 3 is mature but future maintenance may favor another path | P2 | Supported desktop matrix | Decision record compares lifecycle, accessibility, Wayland, packaging, and migration cost |
| Distribution — evaluate Flatpak/Snap or additional distro formats | Sandboxing may simplify delivery but conflicts with logind/system-bus access | P2 | Stable upstream release; policy verification | Minimal reviewed permissions, functional idle/notification flow, reproducible manifest, store policy compliance |
| Cross-platform — add non-Linux providers | Product background aspires to cross-platform use but code is Linux-specific | P2 | Confirmed product demand; modular boundaries | Native activity/notification/startup integrations, privacy review, packaging, and full platform tests |
| Privacy — optional diagnostic export | Structured diagnostics could improve bug reports | P2 | Redaction schema and consent UX | Explicit opt-in export contains no identifiers/paths by default and previews every included field |

Before starting a roadmap item, create a scoped issue and confirm product decisions marked as dependencies. Reprioritize using observed user impact and maintenance capacity.
