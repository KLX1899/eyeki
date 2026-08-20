# Release and distribution

The first public-release target is Ubuntu. The intended end-user experience is a single package-manager command that installs EyeKi and resolves runtime dependencies automatically; compiler and development packages are source-build concerns, not end-user prerequisites. Additional Linux distributions, Snap, Flatpak, and macOS are separate later targets.

## Readiness assessment

EyeKi is not ready for a public binary release or submission to a distribution repository. The source is pre-release, has no CI or Git tags, and still contains P0 settings-reload, session-selection, popup, notification, and packaging defects. Packaging work exists but was untracked or ignored at the documentation audit.

## Current packaging capability

- `Makefile` builds `eyeki` and can install/stage it under `/usr/bin` plus `eyeki.service` under `/usr/lib/systemd/user` by default.
- `eyeki.service` runs the foreground loop as a systemd user service and restarts failures.
- `debian/` contains preliminary debhelper 13 control, rules, install, copyright, and changelog files.
- `install.sh` is obsolete and nonfunctional against current lowercase source names; it also makes unreviewed user-service/environment changes.
- `eyeki_1.0.orig.tar.gz` is not a valid release source artifact: it has no versioned top-level directory, contains an x86-64 binary, embeds owner/timestamp metadata, and includes a zero-byte entry for itself.
- Branch names suggest APT/Snap exploration, but no Snap manifest or functional Snap packaging exists. A branch name is not release support.

The Makefile's pkg-config dependency was corrected during the documentation audit from `dbus-1` to `libsystemd`, matching the `sd-bus` API used by source. Compilation still needs verification in a complete toolchain.

## Missing or inconsistent release metadata

- The authoritative development version is `0.1.0` in `version.h` and is exposed by `eyeki --version`, but there is no verified release tag or release record yet.
- The ignored archive labeled `1.0` is stale and must not be treated as a release artifact. Preliminary Debian metadata now maps the development version to unreleased package revision `0.1.0-1`.
- Debian copyright attribution differs from the root license and contains an abbreviated license stanza; maintainers must reconcile it without rewriting valid legal history.
- No supported-platform/minimum-version matrix, icon, desktop/AppStream metadata, screenshots, release notes history, changelog tags, SBOM, checksums, signatures, provenance, or asset attribution inventory.
- No dependency minimum versions or documented third-party license review.
- No CI/release workflow, clean-checkout build evidence, reproducibility process, or vulnerability scan.

Because EyeKi is primarily a background user service rather than a conventional launcher-driven GUI, desktop/AppStream metadata requirements depend on the chosen repository and presentation model. Verify each target's current policy externally before submission.

## Versioning approach

Use the authoritative SemVer-like upstream version from `version.h`, starting at `0.1.0` while interfaces and behavior are unstable, and expose it through `eyeki --version`. Map it to package versions such as Debian revisions. Do not retroactively declare the preliminary `1.0` archive released without evidence.

Tags should be annotated and protected according to maintainer policy. The changelog should move reviewed entries from `Unreleased` to the exact upstream version/date. Package revisions must not masquerade as upstream versions.

## Release checklist

1. Resolve all P0 items in [the roadmap](ROADMAP.md), with automated regression tests.
2. Confirm supported Linux distributions, architectures, desktops, and display servers from actual tests.
3. Resolve every `TODO(maintainer):` relevant to support, security contact, screenshots, and release policy.
4. Reconcile version, binary/unit paths, license/copyright, maintainer metadata, dependency names, and user-facing claims.
5. Build/test from a clean checkout in CI with declared minimum and current dependency sets.
6. Run unit/integration tests plus manual notification, popup, idle/resume, restart, upgrade, and uninstall scenarios.
7. Review accessibility, RTL/localization, notification-on-lock-screen behavior, config permissions, logs, D-Bus access, and sandbox permissions.
8. Audit third-party dependency licenses/advisories; produce an SBOM and attribution material where required.
9. Generate a source-only archive from the tagged tree with a versioned top-level directory and normalized metadata; exclude binaries, nested archives, local config, logs, and VCS state.
10. Build artifacts in isolated builders; record toolchain/dependency inputs and produce checksums/provenance.
11. Sign the tag/artifacts with the maintainer-approved mechanism; protect signing keys outside CI logs/workspaces.
12. Install/upgrade/uninstall each artifact in a disposable VM and verify the user unit lifecycle.
13. Publish release notes, checksums, signatures/provenance, known limitations, privacy/security links, and supported-platform facts.
14. Submit downstream packages only after upstream artifacts are immutable and reproducible.

## Artifact generation and reproducibility

The current archive must be discarded and regenerated from version-controlled sources, not repacked. A reproducible process should:

- start from the exact signed tag in a clean checkout;
- use stable ordering, normalized ownership, and a documented timestamp policy;
- put files below `eyeki-<version>/`;
- exclude compiled output and the archive itself;
- capture compiler/build-container and dependency versions;
- separate upstream source from distro-specific generated artifacts when target policy requires it;
- verify rebuilding the same tag yields matching source checksums and investigate binary variance.

No runtime updater exists. Initially, updates should be delivered by the chosen package manager. Adding an updater would introduce networking, signing, rollback, privacy, and long-term infrastructure obligations and needs separate design.

## Platform-specific options

### Debian-family source package

The existing debhelper metadata is a starting point, not a passing package. Before using it:

- track and validate every `debian/` file from a clean source tree;
- use an authoritative upstream version/tarball and consistent copyright data;
- build with the declared dependency set and run current Debian policy/lint tools;
- verify user-service install/enable expectations—packages generally should not surprise-enable per-user services;
- add tests, hardening flags, watch/upstream metadata as appropriate, and complete long description/claims;
- test install, upgrade, purge, multi-user behavior, and graphical-session startup.

For the initial Ubuntu goal, a reviewed `.deb` installed with `apt` can resolve declared dependencies, while an Ubuntu repository or PPA can provide the eventual one-command install and normal updates. Publishing that repository is release infrastructure work and must follow clean-build, signing, supported-Ubuntu-version, and upgrade testing requirements.

Repository standards and tooling change; verify current Debian/Ubuntu or target-repository policy externally rather than treating this guide as authoritative submission policy.

### Snap or Flatpak

Neither format is implemented. Both require a deliberate permission model for system-bus/logind access, notification delivery, graphical display, autostart/background behavior, and configuration persistence. Broad system-bus permissions undermine sandbox value. Prototype in a separate reviewed manifest only after core behavior is testable, and verify current store/flathub policies externally.

### Other Linux repositories

Provide stable upstream source, deterministic build instructions, license metadata, an issue/security channel, and evidence for platform claims. Let downstream maintainers adapt package metadata unless the project can continuously test it. Verify each repository's naming, review, signing, service, and update rules at submission time.

macOS and Windows packages are out of scope until native replacements exist for logind activity, libnotify, systemd lifecycle, and GTK presentation assumptions.

## Signing, permissions, privacy, and licensing

- Decide who can sign releases and how key rotation/revocation works. Do not embed private signing material in the repository or general-purpose CI variables.
- Document why system-bus logind access is required and request only narrow interfaces in sandboxed formats.
- Notification text may appear on a lock screen; offer/document OS controls and avoid health-sensitive detail by default.
- Persist only settings, with owner-private permissions and no telemetry by default. Review [Privacy](PRIVACY.md) for every release.
- The root `LICENSE` is MIT and unambiguous. Confirm all source/packaging attribution and inspect actual dependency/bundled-asset licenses before distribution; do not assume compatibility from package names.
- No bundled image/font/audio assets currently require attribution. Add an inventory when assets are introduced.

## Suggested CI/CD workflow

1. **Pull requests:** secret scan, repository hygiene/link check, dependency install, warning-clean build, unit tests, static analysis, and staged-install verification.
2. **Scheduled:** supported dependency/platform matrix plus vulnerability/advisory review.
3. **Tag candidate:** require clean tree and matching version/changelog; build source archive and packages in isolated pinned environments; run package linters and VM smoke tests.
4. **Approval gate:** maintainer reviews test evidence, permissions, SBOM/licenses, checksums, and release notes.
5. **Publish:** sign and upload immutable artifacts, then submit/publish packages; never rebuild under the same version.
6. **Post-release:** verify downloads/signatures, monitor regressions/security reports, and document rollback/yank policy.

Exact CI provider and repository upload credentials remain maintainer decisions. Use least-privilege short-lived credentials and protected environments.
