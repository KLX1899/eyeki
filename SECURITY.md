# Security policy

## Supported versions

EyeKi has no verifiable tagged or published release, so no released version can currently be declared supported.

| Version | Security support |
| --- | --- |
| Unreleased development branch | Best-effort review only; no response-time commitment |
| Development `0.1.0` / Debian `0.1.0-1` | Unreleased; best-effort review only |
| Stale local `1.0` archive | Not released or supported |

`TODO(maintainer): Define supported release lines after the first verified release.`

## Reporting a vulnerability

Do not open a public issue or attach exploit details, private logs, home paths, session identifiers, or configuration from a real account.

`TODO(maintainer): Publish a private vulnerability-reporting channel. If private GitHub Security Advisories are enabled for this repository, designate that as the preferred channel; otherwise provide a maintainer-controlled private contact without inventing one here.`

Until that placeholder is resolved, avoid public disclosure of actionable details. A report should contain:

- affected commit/version and platform;
- concise impact and prerequisites;
- minimal reproduction using disposable data/account;
- whether notification, popup, config, D-Bus/logind, packaging, or service lifecycle is involved;
- a suggested mitigation, if known.

No acknowledgement or remediation deadline is promised yet. The maintainer should coordinate disclosure and credit preferences privately when a reporting channel exists.

## Security-sensitive scope

- Parsing and writing the XDG or legacy HOME config path, including migration, symlink/race, and permission behavior.
- Integer parsing/arithmetic that can create reminder storms or denial of service.
- system D-Bus queries to logind and selection/exposure of session metadata.
- GTK fullscreen/focus/dismissal behavior and user ability to exit.
- Notification content exposure, especially on lock screens or shared displays.
- systemd user-unit startup, executable path, environment inheritance, restart loops, and package scripts.
- Dependency/build/release integrity, archive provenance, and signing.

EyeKi source contains no network client, account system, telemetry, updater, or medical-history store. Introducing any of these materially expands security scope and requires threat modeling before implementation.

## Local data and OS integration

Current settings are plain text. EyeKi creates missing parents privately, enforces `0700` on its application directory, writes through an exclusive `0600` temporary file, and syncs before and after atomic rename. It rejects symlinked application-directory components, and CLI setting commands exit nonzero on save failure. Existing parent-directory policy, silent read/parse fallback, concurrent lost updates, and retained legacy files remain in security scope.

Idle lookup reads records returned by logind, including session metadata, but the code does not persist or intentionally log those records. D-Bus failures currently become “active” and can produce reminders at inappropriate times. The service logs interval/mode at startup to stderr/user journal.

Notification and popup delivery crosses into desktop components. Treat their content as visible to local users and lock-screen policy. Ensure reminders always have an accessible, reliable dismissal path and do not create restart/focus loops.

## Dependency expectations

- Build only from reviewed source in isolated, reproducible environments.
- Track GTK, libnotify, libsystemd, toolchain, and packaging advisories for supported distributions.
- Establish minimum supported versions only from testing/security needs.
- Review dependency and bundled-asset licenses before release.
- Generate checksums, SBOM/provenance, and signed artifacts for public releases.
- Never commit credentials, signing keys, private reports, local binaries, or generated archives.

See [Privacy](docs/PRIVACY.md) and [Release and distribution](docs/RELEASE_AND_DISTRIBUTION.md).
