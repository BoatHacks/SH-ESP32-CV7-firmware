# PlatformIO pre-script: defines CV7_FW_VERSION from git, e.g. "1.2.0",
# "1.2.0-3-gabc1234" or "abc1234-dirty". "unknown" outside a git checkout.
import subprocess

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)


def git_version(project_dir):
    try:
        out = subprocess.run(
            ["git", "describe", "--tags", "--always", "--dirty"],
            cwd=project_dir, capture_output=True, text=True, timeout=10,
        )
    except (OSError, subprocess.SubprocessError):
        return "unknown"
    v = out.stdout.strip() if out.returncode == 0 else ""
    if v.startswith("v"):
        v = v[1:]
    return v or "unknown"


version = git_version(env["PROJECT_DIR"])  # noqa: F821
print("CV7_FW_VERSION = " + version)
env.Append(CPPDEFINES=[("CV7_FW_VERSION", env.StringifyMacro(version))])  # noqa: F821
