#!/usr/bin/env python3
"""Validate the stable PAR_TEST_* log contract emitted by host CI tests."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

RESULT_RE = re.compile(r"^PAR_TEST_RESULT\s+(PASS|FAIL)\s+suite=(\S+)(?:\s+.*)?$")
CASE_RE = re.compile(r"^PAR_TEST_CASE\s+(PASS|FAIL|SKIP)\s+suite=(\S+)\s+case=(\S+)(?:\s+.*)?$")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="PAR host-test log to validate")
    parser.add_argument("--require-result", action="append", default=[], help="Suite name that must emit PAR_TEST_RESULT PASS")
    parser.add_argument("--require-pass-suite", action="append", default=[],
                        help="Suite name that must contain at least one PASS case")
    parser.add_argument("--require-pass-case", action="append", default=[],
                        help="Case that must pass, either <case> or <suite>:<case>")
    return parser.parse_args()


def case_matches(requirement: str, suite: str, case: str) -> bool:
    if ":" in requirement:
        req_suite, req_case = requirement.split(":", 1)
        return (suite == req_suite) and (case == req_case)

    return case == requirement


def main() -> int:
    args = parse_args()
    text = args.log.read_text(encoding="utf-8", errors="replace").splitlines()
    results: list[tuple[str, str]] = []
    pass_cases_by_suite: dict[str, set[str]] = {}
    cases = 0

    for line in text:
        result_match = RESULT_RE.match(line)
        if result_match:
            results.append((result_match.group(1), result_match.group(2)))
            continue

        case_match = CASE_RE.match(line)
        if case_match:
            status = case_match.group(1)
            suite = case_match.group(2)
            case = case_match.group(3)
            cases += 1
            if status == "PASS":
                pass_cases_by_suite.setdefault(suite, set()).add(case)

    if not results:
        print(f"PAR_TEST_LOG_INVALID no PAR_TEST_RESULT in {args.log}", file=sys.stderr)
        return 1

    failing = [suite for status, suite in results if status != "PASS"]
    if failing:
        print(f"PAR_TEST_LOG_FAIL failing_suites={','.join(failing)}", file=sys.stderr)
        return 1

    if cases == 0:
        print(f"PAR_TEST_LOG_INVALID no PAR_TEST_CASE in {args.log}", file=sys.stderr)
        return 1

    passed_results = {suite for status, suite in results if status == "PASS"}
    for suite in args.require_result:
        if suite not in passed_results:
            print(f"PAR_TEST_LOG_INVALID missing_pass_result={suite} in {args.log}", file=sys.stderr)
            return 1

    for suite in args.require_pass_suite:
        if not pass_cases_by_suite.get(suite):
            print(f"PAR_TEST_LOG_INVALID no_pass_case suite={suite} in {args.log}", file=sys.stderr)
            return 1

    for requirement in args.require_pass_case:
        if not any(case_matches(requirement, suite, case) for suite, pass_cases in pass_cases_by_suite.items() for case in pass_cases):
            print(f"PAR_TEST_LOG_INVALID missing_pass_case={requirement} in {args.log}", file=sys.stderr)
            return 1

    print(f"PAR_TEST_LOG_PASS cases={cases} results={len(results)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
