#!/usr/bin/env python3
"""Focused source/input provenance test; all Git writes/builds use a temp repo.

Usage: python tools/build-identity/TestBuildIdentity.py --cmake <cmake executable>
Only the two tiny header fixtures are compiled/run; no Horde/game target is used.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def run(args: list[str], cwd: Path, env: dict[str, str]) -> str:
    result = subprocess.run(args, cwd=cwd, env=env, text=True, capture_output=True, check=False)
    if result.returncode:
        raise AssertionError(f"Command failed ({result.returncode}): {args[0]}\n{result.stdout}\n{result.stderr}")
    return result.stdout.strip()


def fields(header: Path) -> dict[str, str]:
    return dict(re.findall(r'^#define HORDE_RT_PROVENANCE_(\w+) "([^"]*)"$',
                           header.read_text(encoding="utf-8"), re.MULTILINE))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", default=shutil.which("cmake"))
    parser.add_argument("--temporary-root", type=Path,
                        help="Existing writable directory for disposable state (use an explicitly allowed sandbox root)")
    args = parser.parse_args()
    if not args.cmake:
        parser.error("--cmake is required when CMake is not on PATH")
    cmake = str(Path(args.cmake).resolve())
    git = shutil.which("git")
    if not git:
        parser.error("Git is required for disposable repository tests")
    source = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="horde-build-id-", dir=args.temporary_root) as temporary:
        temporary_root = Path(temporary).resolve()
        repo = temporary_root / "source"
        build = temporary_root / "build"
        repo.mkdir()
        env = dict(os.environ, GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=str(temporary_root / "empty-git-config"),
                   GIT_TERMINAL_PROMPT="0", GIT_AUTHOR_DATE="2026-01-01T00:00:00+00:00",
                   GIT_COMMITTER_DATE="2026-01-01T00:00:00+00:00")
        (temporary_root / "empty-git-config").write_text("", encoding="utf-8")
        for relative in ("cmake/HordeRtBuildIdentity.cmake", "tools/build-identity/Refresh.cmake",
                         "src/reporting/BuildIdentity.h"):
            destination = repo / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source / relative, destination)
        shutil.copyfile(source / "tools/build-identity/BuildIdentityTests.cpp", repo / "fixture.cpp")
        (repo / "data").mkdir()
        (repo / "data/input.h").write_text("// first input\n", encoding="utf-8")
        (repo / "data/space name.h").write_text("// spaced input\n", encoding="utf-8")
        (repo / "data/[literal].h").write_text("// literal pathspec input\n", encoding="utf-8")
        (repo / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.22)
project(BuildIdentityFixture LANGUAGES CXX)
include(cmake/HordeRtBuildIdentity.cmake)
add_executable(identity_fixture fixture.cpp)
target_compile_features(identity_fixture PRIVATE cxx_std_20)
target_include_directories(identity_fixture PRIVATE src)
target_compile_definitions(identity_fixture PRIVATE HORDE_RT_TEST_EXPECT_STAMP=1)
horde_rt_attach_build_identity(TARGET identity_fixture REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}"
    INPUTS CMakeLists.txt fixture.cpp data/input.h "data/space name.h" "data/[literal].h"
           src/reporting/BuildIdentity.h cmake/HordeRtBuildIdentity.cmake tools/build-identity/Refresh.cmake
    DEFINITIONS strategy=shipping abi=fixture)
add_executable(identity_fallback fixture.cpp)
target_compile_features(identity_fallback PRIVATE cxx_std_20)
target_include_directories(identity_fallback PRIVATE src)
target_compile_definitions(identity_fallback PRIVATE HORDE_RT_TEST_EXPECT_STAMP=0)
''', encoding="utf-8")

        def git_run(*arguments: str) -> str:
            return run([git, "-c", "core.autocrlf=false", *arguments], repo, env)

        git_run("init", "--quiet")
        git_run("config", "user.name", "Disposable Test")
        git_run("config", "user.email", "disposable@example.invalid")
        git_run("add", "--all")
        git_run("commit", "--quiet", "-m", "fixture")
        original_head = git_run("rev-parse", "HEAD")
        index_before = (repo / ".git/index").read_bytes()
        run([cmake, "-S", str(repo), "-B", str(build), "-DCMAKE_BUILD_TYPE=Debug"], temporary_root, env)

        def build_refresh() -> tuple[Path, dict[str, str]]:
            run([cmake, "--build", str(build), "--config", "Debug", "--target", "identity_fixture_build_identity"],
                temporary_root, env)
            header = build / "horde-build-identity/identity_fixture/Debug/reporting/BuildIdentityStamp.generated.h"
            assert header.is_file(), "per-target/per-configuration generated stamp missing"
            return header, fields(header)

        header, clean = build_refresh()
        spec = header.parents[1] / "inputs.cmake"

        def build_and_run_fixtures() -> dict[str, str]:
            run([cmake, "--build", str(build), "--config", "Debug", "--target", "identity_fixture", "identity_fallback"],
                temporary_root, env)
            labels: dict[str, str] = {}
            for target in ("identity_fixture", "identity_fallback"):
                candidates = (build / "Debug" / f"{target}.exe", build / f"{target}.exe", build / target)
                executable = next((path for path in candidates if path.is_file()), None)
                assert executable is not None, f"compiled fixture missing: {target}"
                labels[target] = run([str(executable)], temporary_root, env)
                assert len(labels[target]) <= 118
            return labels

        assert clean["COMMIT"] == original_head and clean["INPUT_STATE"] == "clean"
        assert re.fullmatch(r"[0-9a-f]{64}", clean["INPUT_SHA256"])
        assert len(clean["LABEL"]) <= 118 and ";c:D;s:C" in clean["LABEL"]
        assert (repo / ".git/index").read_bytes() == index_before, "refresh must not write source index"
        assert build_and_run_fixtures()["identity_fixture"] == clean["LABEL"]
        original_header = header.read_bytes()
        original_mtime = header.stat().st_mtime_ns
        build_refresh()
        assert header.stat().st_mtime_ns == original_mtime, "unchanged identity must not rewrite header"

        parked = repo / "reports/parked-owner.bin"
        parked.parent.mkdir()
        parked.write_bytes(b"private owner fixture must never enter identity")
        build_refresh()
        assert header.read_bytes() == original_header and header.stat().st_mtime_ns == original_mtime
        parked.write_bytes(b"changed unrelated owner fixture")
        build_refresh()
        assert header.read_bytes() == original_header, "unrelated untracked changes must be excluded"

        (repo / "data/input.h").write_text("// changed after configure\n", encoding="utf-8")
        _, dirty = build_refresh()
        assert dirty["INPUT_STATE"] == "dirty" and dirty["INPUT_SHA256"] != clean["INPUT_SHA256"]
        assert dirty["COMMIT"] == original_head, "dirty stamp must retain the real source commit"
        git_run("add", "data/input.h")
        _, staged = build_refresh()
        assert staged == dirty, "staged and unstaged identical input bytes have identical provenance"
        git_run("restore", "--source=HEAD", "--staged", "--worktree", "data/input.h")
        _, restored = build_refresh()
        assert restored == clean
        git_run("commit", "--allow-empty", "--quiet", "-m", "new commit without reconfigure")
        _, new_head = build_refresh()
        assert new_head["COMMIT"] != original_head and new_head["COMMIT"] == git_run("rev-parse", "HEAD")
        assert new_head["INPUT_SHA256"] == clean["INPUT_SHA256"]

        def probe_spec(name: str, suffix: str = "", replace: tuple[str, str] | None = None) -> dict[str, str]:
            text = spec.read_text(encoding="utf-8") + "\n" + suffix
            if replace:
                assert replace[0] in text
                text = text.replace(*replace)
            probe = temporary_root / f"{name}.cmake"
            output = temporary_root / f"{name}.h"
            probe.write_text(text, encoding="utf-8")
            run([cmake, f"-DID_SPEC={probe}", f"-DID_OUTPUT={output}", "-P",
                 str(repo / "tools/build-identity/Refresh.cmake")], temporary_root, env)
            return fields(output)

        (repo / "data/selected-new.h").write_text("// explicitly selected new source\n", encoding="utf-8")
        selected = probe_spec("selected", "list(APPEND ID_INPUTS data/selected-new.h)\n")
        assert selected["INPUT_STATE"] == "dirty" and selected["INPUT_SHA256"] != clean["INPUT_SHA256"]
        missing = probe_spec("missing", "list(APPEND ID_INPUTS data/not-present.h)\n")
        assert missing["INPUT_STATE"] == "unavailable" and missing["INPUT_SHA256"] == "unavailable"
        no_git = probe_spec("no-git", 'set(ID_GIT "")\n')
        assert no_git["COMMIT"] == "unavailable" and no_git["INPUT_STATE"] == "unavailable"
        assert no_git["INPUT_SHA256"] == clean["INPUT_SHA256"], "content scope remains measurable without Git"
        release = probe_spec("release", 'set(ID_CONFIGURATION Release)\n')
        assert release["INPUT_SHA256"] != clean["INPUT_SHA256"] and ";c:R;" in release["LABEL"]
        options = probe_spec("options", "list(APPEND ID_DEFINITIONS strategy=diagnostic)\n")
        assert options["INPUT_SHA256"] != clean["INPUT_SHA256"]
        compiler = probe_spec("compiler", "set(ID_COMPILER other-1.0)\n")
        assert compiler["INPUT_SHA256"] != clean["INPUT_SHA256"]
        moved = temporary_root / "relocated"
        shutil.copytree(repo, moved)
        relocated = probe_spec("relocated", replace=(repo.as_posix(), moved.as_posix()))
        assert relocated == new_head, "source location must not enter provenance"
        serialized = header.read_text(encoding="utf-8")
        assert str(temporary_root) not in serialized and temporary_root.as_posix() not in serialized
        assert "private owner" not in serialized and "disposable@example" not in serialized
        assert build_and_run_fixtures()["identity_fixture"] == new_head["LABEL"], \
            "native consumer must recompile the changed stamp without explicit reconfigure"
        print("PASS: per-build HEAD/input freshness, scoped dirty/staged/new/missing inputs, no-Git fallback,")
        print("      parked input exclusion, unchanged header/index, relocation, config/options/toolchain sensitivity,")
        print("      compiled generated/fallback APIs and exact bounded privacy grammar (disposable repositories only).")


if __name__ == "__main__":
    main()
