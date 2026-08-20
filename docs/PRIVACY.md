# Privacy

This document separates verified current behavior from recommended future policy. It is an implementation summary, not a legal privacy notice.

## Verified current behavior

### Stored data

EyeKi stores two settings in plain text:

- reminder interval as an integer;
- mode as `popup` or `notification`.

The path is `$XDG_CONFIG_HOME/eye_reminder/config` when the override is absolute, or `$HOME/.config/eye_reminder/config` otherwise. The source does not store reminder history, timestamps, completed-dose records, medication names, screen content, input events, account identifiers, or analytics.

Missing config parents are created with owner-only access. The EyeKi directory is enforced as `0700`; complete settings are written and synced through a `0600` temporary file before atomic replacement. Symlinked directory components are rejected. Save failures reach the CLI without logging the selected path or file contents.

With an active XDG override and no XDG config, EyeKi reads the legacy HOME config. A later successful settings change writes the XDG file and leaves the legacy file untouched, so both copies can remain until the user deliberately removes the legacy one.

### Data processed but not intentionally retained

Every ten seconds, EyeKi asks systemd-logind on the local system bus for a session list. The reply includes session ID, numeric user ID, username, seat, and object path. Current code uses only the first session path to query idle state/time; it does not intentionally save or log the other fields.

The reminder interval and mode are printed to stderr at daemon startup. Under the supplied systemd user service, that output can be retained in the user journal according to OS policy.

### Data leaving the device

No source code opens a network connection, calls a remote service, loads remote content, sends telemetry, or checks for updates. Based on the repository, no application data is intentionally transmitted off-device.

EyeKi does communicate locally with:

- systemd-logind over system D-Bus for idle metadata;
- the desktop notification service through libnotify/session integration;
- the local display server/compositor through GTK in popup mode;
- the local filesystem for settings;
- the systemd user manager when run as the supplied service.

Those OS components may have their own logging, synchronization, crash-reporting, or lock-screen policies outside EyeKi's implementation.

### Permissions and visibility

There is no in-app permission prompt or permission-state UI. Access is governed by D-Bus, desktop, display-server, filesystem, and service-manager policy. Notification contents can appear in history or on a lock screen. The popup intentionally covers a screen, although compositor policy controls the actual result.

Reminder text is hard-coded in Persian and mentions artificial tears. This can reveal health-adjacent context to someone viewing the screen, notification history, or screenshots.

## Current privacy risks

- The first logind session may belong to another local user; even though fields are not retained, querying/acting on the wrong session is a privacy and correctness concern.
- Existing XDG parent directories and journal retention remain governed by OS/user policy; a legacy HOME config can retain its earlier permissions after non-destructive migration.
- Notifications may expose reminder content on locked/shared displays.
- Silent integration failures prevent users from knowing which session/data path is active.
- Bug reports can accidentally include usernames, paths, session IDs, unrelated journal records, or desktop screenshots.
- A fullscreen prompt may reveal context or disrupt other visible work.

## Recommended future policy

These are recommendations, not implemented features:

- Keep EyeKi local-only and telemetry-free by default.
- Collect/persist only settings needed for scheduling; do not add medication/adherence history without explicit product need, consent, retention, export/delete controls, and legal review.
- Select only the current user's intended session and minimize D-Bus fields processed.
- Keep XDG migration and private atomic writes covered by regression tests; do not expose paths or contents in failure diagnostics.
- Offer privacy-conscious notification wording and document how desktop lock-screen previews can be disabled.
- Rate-limit and redact logs; never log usernames, session IDs, home paths, environment blocks, or full config files by default.
- Make any diagnostic export explicit, previewable, minimal, and redact identifiers automatically.
- Document new networking, crash reporting, accounts, sync, update checks, or permissions before implementation and update this file in the same change.

## Items requiring future privacy review

- Exact target-desktop notification history and lock-screen behavior.
- Sandbox permissions for any Flatpak/Snap package.
- Config migration, deletion, and uninstall behavior.
- Journal retention and service crash-loop logging.
- Localization wording and whether default content should be less health-specific.
- Screenshots, crash dumps, diagnostics, and issue-template redaction guidance.
- Policies of downstream packages or OS components that may add reporting/update mechanisms.

Security-sensitive reporting guidance is in [Security policy](../SECURITY.md).
