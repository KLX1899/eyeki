# Changelog

All notable changes should be documented here. The format follows Keep a Changelog. The current upstream development version is `0.1.0`; it remains unreleased until a verified tag and artifacts exist.

## [Unreleased]

### Added

- An authoritative source version and `--version` CLI command.
- Desktop-independent scheduler regression tests for threshold, idle, unknown-state, and clock-discontinuity behavior.
- Desktop-independent interval/config regression tests for production boundaries, malformed values, and checked conversion.
- User, contributor, AI-agent, architecture, development, roadmap, privacy, security, release, and repository-audit documentation.
- GitHub bug, feature, and pull-request templates.

### Changed

- Extracted configuration and monotonic scheduling into separate C modules.
- Enforced strict 10–300 minute interval input and overflow-safe conversion before scheduling.
- Changed away/unknown activity handling to discard accumulated active time instead of preserving or advancing it.
- Adopted the XDG config location with a non-destructive legacy fallback, automatic private parent creation, atomic `0600` writes, and CLI-visible save failures.
- Corrected the Makefile's D-Bus provider from `dbus-1` to `libsystemd`, matching the source's `sd-bus` API.
- Updated ignore rules so source packaging metadata can be tracked while build binaries and generated source archives remain ignored.

## Release history

Reliable historical releases cannot be reconstructed. The repository has no verified historical tags or published-release records in its local history. The ignored archive named with `1.0` is a stale local artifact and is not a release; no historical version entry is fabricated here.
