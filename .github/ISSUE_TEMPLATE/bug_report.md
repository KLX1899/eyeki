---
name: Bug report
about: Report reproducible EyeKi behavior
title: "[Bug]: "
labels: ""
assignees: ""
---

## Summary

Describe what happened and the impact. Do not include vulnerabilities; use `SECURITY.md`.

## Environment

- EyeKi version, commit, or package version:
- Installation/launch method (direct binary or systemd user service):
- OS and distribution version:
- Desktop environment and version:
- Display server (X11/Wayland/unknown):
- Architecture:
- Reminder mode (notification/popup):
- Positive interval in minutes:

## Steps to reproduce

1.
2.
3.

## Expected behavior


## Actual behavior


## Timer and integration details

- Was the session active, idle, locked, suspended, or switching users?
- Did the problem involve a setting change while EyeKi was already running?
- For notifications: did other desktop notifications work, and was EyeKi allowed/shown in notification settings?
- For popup: could it be dismissed by button, keyboard, or window-manager close?
- Is the problem consistent after restarting EyeKi?

## Minimal diagnostics

Include EyeKi-only output, exit status, and—if relevant—a short redacted excerpt from `systemctl --user status eyeki.service` or `journalctl --user -u eyeki.service`. The output of `eyeki --show-config` is usually sufficient; do not attach the whole config/home directory.

Remove usernames, email addresses, home paths, hostnames, session IDs, unrelated journal entries, screenshots of private applications, notification history, and credentials. Reproduce with a disposable home/account where possible.

## Additional context

List workarounds, regression range, and focused screenshots with personal details removed.
