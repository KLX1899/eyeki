# Repository audit

This snapshot records evidence found during the 2026-08-14 documentation audit. It is not a permanent statement of release status; update it when findings are resolved.

## Inventory and tracked state

The committed history contained only `.gitignore`, `LICENSE`, `README.md`, `config.h`, and `eyeki.c`. The worktree also contained an untracked Makefile, installer, generated source archive, and ignored compiled binary, systemd unit, and `debian/` directory. There were no tests, CI workflows, assets, scripts beyond the installer, localization files, lock files, release tags, or nested contributor/agent instructions.

The ignore file incorrectly hid the systemd unit and Debian source metadata and did not ignore the lowercase Makefile output. This audit changed it to ignore only project binaries and generated `*.orig.tar.gz` archives. Packaging source files now appear to Git and should be reviewed before being committed.

## Hygiene findings

| Check | Finding | Disposition |
| --- | --- | --- |
| Build artifacts | Ignored `EyeKi` is a non-stripped x86-64 ELF linked to GTK 3, libnotify, and libsystemd | Keep uncommitted; rebuild from source for every test/release |
| Source archive | Local archive contains binary and a zero-byte entry for itself, lacks versioned root, and embeds owner/time metadata | Ignore/discard and generate reproducibly from a tag |
| Machine-specific files | No tracked editor/IDE/cache files found; archive carries local ownership metadata | Normalize future archives |
| Potential secrets | Pattern/repository review found no obvious credentials in tracked application files | Add automated secret scanning; never treat this as exhaustive |
| Personal data | Preliminary Debian metadata contains maintainer identity/contact data | Maintainer must confirm it is intended for public packaging; do not copy it elsewhere |
| Ignore rules | Generic C ignores were mostly reasonable; project source packaging was hidden and lowercase output missing | Corrected during audit |
| License | Root MIT license is unambiguous; Debian copyright has different attribution and an incomplete MIT stanza | Reconcile before packaging; preserve valid legal attribution/history |
| Lock files | None; dependencies are native system libraries discovered by `pkg-config` | Expected for this build style, but define/test minimum versions and review advisories |
| Generated files | Binary and source archive are generated; no generated source/catalogs | Documented as non-editable/non-committable |
| Versions | Authoritative development version `0.1.0` exists, but no verified tag/release exists; the ignored `1.0` archive is stale | Keep source, CLI, package revision, tag, and release metadata synchronized |
| Documentation links | Old README described compile-time configuration and omitted actual behavior/setup | Replaced with source-backed docs; custom link check required until CI exists |
| Stale files/comments | `install.sh` uses nonexistent uppercase source/binary/unit; Debian claims CLI/Wayland support more strongly than evidence; some source comments overstate fallback/continuous activity | Installer/source left unchanged; claims flagged for focused fixes |
| Asset attribution | No bundled visual/audio/font assets found | Add inventory/attribution when assets appear |

## Architecture and maintenance risks

- `config.h` defines non-static functions; adding another translation unit that includes it causes duplicate definitions.
- `get_idle_seconds()` chooses the first system session rather than the current user's intended session.
- A D-Bus error returns zero idle time, so failures count toward reminders.
- The original realtime/wall-clock scheduler was replaced with a monotonic scheduler, but the change still requires clean-build and runtime evidence.
- Integer parsing, range, and multiplication are unchecked.
- Save creates only the final directory, silently fails without `$HOME/.config`, overwrites non-atomically, and reports success anyway.
- Live interval updates wait for the old threshold; live notification-to-popup updates can call uninitialized GTK.
- Popup WM-close is not wired to its loop exit; notification and GTK failures are mostly ignored.
- Every poll reconnects to system D-Bus; global popup state/manual event pumping limits maintainability.
- The systemd unit fixed path must stay aligned with packaging and can restart-loop failures.

## Accessibility and localization readiness

Readiness is low. Strings are hard-coded Persian with an embedded one-hour assumption; there is no translation catalog, locale selection, pluralization, or translator workflow. The UI uses a GTK button and high-contrast CSS, but keyboard, assistive technology, RTL layout, focus, scale, WM close, timeout, multi-monitor, and compositor behavior have no test evidence. A fullscreen acknowledgement can reduce user control and needs explicit accessibility/product review.

## Notification permissions and failure states

EyeKi does not check `notify_init()`/`notify_notification_show()` results, expose delivery state, guide users through OS notification policy, or account for notification-server timeout/lock-screen decisions. GTK initialization and styling errors are also inadequately handled. D-Bus failure is silently misclassified as activity. These are release blockers, not merely documentation gaps.

## Commands and environment evidence

- `git log`, branches, tags, tracked/ignored status, archive table, and every repository file were inspected.
- `pkg-config --modversion gtk+-3.0 libnotify dbus-1 libsystemd` failed because development `.pc` files were unavailable.
- Compiler, Make, shellcheck/Markdown lint, Debian build, and Debian lint tools were unavailable.
- The existing ignored ELF resolved runtime libraries with `ldd` and ran one-shot CLI paths in disposable homes.
- With a fresh disposable home lacking `.config`, setting commands printed success but did not persist. After creating `.config`, interval `5` and notification mode round-tripped; invalid mode and unknown option exited `1`.
- The daemon, popup, notification delivery, logind behavior, source build, install, Debian package, and source reproducibility were not executed/validated.

See the final working-tree diff and current command output rather than relying on this historical snapshot after subsequent changes.
