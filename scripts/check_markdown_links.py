#!/usr/bin/env python3
"""Validate repository-local links and heading anchors in tracked Markdown."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit


LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
REFERENCE_RE = re.compile(r"^\s*\[[^\]]+\]:\s*(\S+)")
HEADING_RE = re.compile(r"^\s{0,3}#{1,6}\s+(.+?)\s*#*\s*$")
FENCE_RE = re.compile(r"^\s*(`{3,}|~{3,})")


def repository_root() -> Path:
    return Path(__file__).resolve().parent.parent


def tracked_markdown_files(root: Path) -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z", "--", "*.md"],
        cwd=root,
        check=True,
        capture_output=True,
    )
    return [root / path.decode() for path in result.stdout.split(b"\0") if path]


def markdown_lines(path: Path) -> list[tuple[int, str]]:
    lines: list[tuple[int, str]] = []
    fence_marker: str | None = None

    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        fence = FENCE_RE.match(line)
        if fence:
            marker = fence.group(1)[0]
            if fence_marker is None:
                fence_marker = marker
            elif marker == fence_marker:
                fence_marker = None
            continue
        if fence_marker is None:
            lines.append((number, line))

    return lines


def github_slug(heading: str) -> str:
    heading = re.sub(r"<[^>]+>", "", heading).strip().lower()
    heading = re.sub(r"[^\w\- ]", "", heading)
    return re.sub(r"\s+", "-", heading)


def heading_anchors(path: Path) -> set[str]:
    anchors: set[str] = set()
    occurrences: dict[str, int] = {}

    for _, line in markdown_lines(path):
        match = HEADING_RE.match(line)
        if not match:
            continue
        base = github_slug(match.group(1))
        count = occurrences.get(base, 0)
        occurrences[base] = count + 1
        anchors.add(base if count == 0 else f"{base}-{count}")

    return anchors


def link_target(raw_target: str) -> str:
    target = raw_target.strip()
    if target.startswith("<") and ">" in target:
        return target[1 : target.index(">")]
    return target.split(maxsplit=1)[0]


def local_links(path: Path) -> list[tuple[int, str]]:
    links: list[tuple[int, str]] = []

    for number, line in markdown_lines(path):
        targets = [match.group(1) for match in LINK_RE.finditer(line)]
        reference = REFERENCE_RE.match(line)
        if reference:
            targets.append(reference.group(1))
        links.extend((number, link_target(target)) for target in targets)

    return links


def validate_file(root: Path, source: Path) -> list[str]:
    errors: list[str] = []

    for line, target in local_links(source):
        parsed = urlsplit(target)
        if parsed.scheme or parsed.netloc or target.startswith("/"):
            continue

        relative_path = unquote(parsed.path)
        destination = source if not relative_path else source.parent / relative_path
        destination = destination.resolve()
        try:
            destination.relative_to(root)
        except ValueError:
            errors.append(f"{source.relative_to(root)}:{line}: link escapes repository: {target}")
            continue

        if not destination.exists():
            errors.append(f"{source.relative_to(root)}:{line}: missing target: {target}")
            continue

        if parsed.fragment and destination.suffix.lower() == ".md":
            fragment = unquote(parsed.fragment).lower()
            if fragment not in heading_anchors(destination):
                errors.append(
                    f"{source.relative_to(root)}:{line}: missing heading "
                    f"#{parsed.fragment} in {destination.relative_to(root)}"
                )

    return errors


def main() -> int:
    root = repository_root()
    files = tracked_markdown_files(root)
    errors = [error for path in files for error in validate_file(root, path)]

    if errors:
        print("Markdown link check failed:", file=sys.stderr)
        for error in errors:
            print(f"  {error}", file=sys.stderr)
        return 1

    print(f"Checked {len(files)} tracked Markdown files.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
