# AI-assisted development workflow

This guide complements root [AGENTS.md](../AGENTS.md). AGENTS defines repository-specific invariants and completion rules; this file provides a repeatable workflow and prompt templates.

## Read-first context

1. `AGENTS.md`
2. `docs/PROJECT_CONTEXT.md`
3. `git status` and the current diff
4. `src/eyeki.c`, `src/config.h`, and the task-relevant build/service/package file
5. Task-specific docs: architecture for flows, development for commands, privacy/security for sensitive changes, and release guidance for packaging

Do not trust summaries, an ignored binary, archive contents, stale installer, branch name, or preliminary package claim over the current source and configured build files.

## Inspection and planning sequence

1. Restate the requested outcome and non-goals.
2. Inventory affected files plus repository instructions; identify unrelated user changes.
3. Trace the behavior end-to-end: CLI/config → scheduler/activity → notification/popup → service/package.
4. Check available dependencies and establish a baseline with exact commands/results.
5. Separate confirmed facts, inferences, product decisions, and blocked verification.
6. Plan the smallest coherent change, including failure paths, tests/manual evidence, platform impact, and documentation.

Record an assumption before relying on it. Use: `Assumption: … Evidence: … Risk if wrong: … Confirmation needed: …`. If the risk materially changes product behavior, privacy, packaging permissions, or support scope, stop and request maintainer direction.

## Implementation and verification

- Preserve unrelated work and avoid opportunistic refactors.
- Change one architectural responsibility at a time; add characterization tests before fragile timer/config/UI refactors.
- Never use a real user config for testing. Follow [safe configuration validation](DEVELOPMENT.md#safe-configuration-validation).
- Do not claim UI/platform behavior without a real graphical-session test on the named environment.
- Run repository-defined commands before completion:

```sh
pkg-config --modversion gtk+-3.0 libnotify libsystemd
make clean
make
./eyeki --help
./eyeki --show-config
```

There is a scheduler unit-test target, but no automated lint/format/doc target or CI. If a task adds one, update AGENTS, Development, Contributing, and CI in the same change. If a command cannot run, capture the exact blocker; do not substitute an older binary as a build pass.

## Keep documentation synchronized

Update only documents affected by the change, but check these relationships:

- user commands/behavior/limitations → `README.md`;
- modules, lifecycle, data/error flow → `docs/ARCHITECTURE.md`;
- prerequisites, commands, debugging → `docs/DEVELOPMENT.md` and `AGENTS.md`;
- stored/processed/transmitted data or OS permissions → `docs/PRIVACY.md` and `SECURITY.md`;
- priorities/open work → `docs/ROADMAP.md`;
- version, packaging, dependencies, artifacts → `docs/RELEASE_AND_DISTRIBUTION.md`;
- notable user/developer change → `CHANGELOG.md`.

Use links instead of repeating long explanations. Label recommendations as recommendations and unresolved ownership with searchable `TODO(maintainer):` markers.

## Final review and handoff

Inspect `git diff`, `git status`, internal Markdown links, unsupported claims, executable examples, names/versions/paths, secrets/personal paths, and generated files. A handoff should be reproducible without hidden conversation context.

### Task handoff template

```text
Outcome:
Scope changed:
Key decisions and evidence:
Files changed:
Commands run and exact outcomes:
Manual scenarios run:
Not run and why:
Known risks/limitations:
Assumptions needing confirmation:
Recommended next step:
```

### Implementation-request template

```text
Goal: <user-visible outcome>
In scope: <specific behavior/files/platforms>
Out of scope: <explicit exclusions>
Current evidence: <source/docs/issues>
Acceptance criteria:
- <observable success>
- <failure/edge behavior>
- <tests and platform evidence>
Privacy/security/accessibility constraints: <requirements>
Documentation to update: <paths>
Unresolved maintainer decisions: <questions>
```

### Bug-investigation template

```text
Symptom:
Expected behavior:
Commit/version and launch method:
OS/distribution, desktop, display server:
Mode and interval (redacted/minimal):
Reproduction steps:
Observed EyeKi-only output and exit status:
Config load/save evidence from disposable HOME:
logind/D-Bus and notification/display availability:
Relevant source flow:
Leading hypotheses, evidence for/against each:
Smallest diagnostic next step:
Privacy redactions applied:
```
