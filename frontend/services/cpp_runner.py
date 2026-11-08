"""File based Python interface to the CyberPath C++ engine."""

from __future__ import annotations

import json
import os
import subprocess
import tempfile
from pathlib import Path
from typing import Any, Mapping


PROJECT_ROOT = Path(__file__).resolve().parents[2]


class EngineError(RuntimeError):
    """A configuration, process, or protocol error from the C++ engine."""

    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code
        self.message = message


def build_request(
    algorithm: str,
    network: Mapping[str, Any],
    parameters: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    """Create the version 1 protocol request for a network algorithm."""
    if not isinstance(network, Mapping):
        raise ValueError("network must be a mapping")
    return {
        "schema_version": 1,
        "algorithm": algorithm,
        "parameters": dict(parameters or {}),
        "network": dict(network),
    }


def _resolve_executable(executable_path: str | os.PathLike[str] | None) -> Path:
    configured = executable_path or os.environ.get("CYBERPATH_ENGINE_PATH")
    if configured:
        executable = Path(configured).expanduser()
        if not executable.is_absolute():
            executable = PROJECT_ROOT / executable
    else:
        candidates = (
            PROJECT_ROOT / "build" / "cyberpath_engine.exe",
            PROJECT_ROOT / "build" / "cyberpath_engine",
        )
        executable = next((candidate for candidate in candidates if candidate.is_file()), candidates[0])
    if not executable.is_file():
        raise EngineError("ENGINE_NOT_FOUND", f"C++ engine executable not found: {executable}")
    return executable.resolve()


def run_request(
    request: Mapping[str, Any],
    *,
    executable_path: str | os.PathLike[str] | None = None,
    timeout_seconds: float = 10.0,
) -> dict[str, Any]:
    """Write a request file, invoke C++, and return its validated response."""
    if timeout_seconds <= 0:
        raise ValueError("timeout_seconds must be positive")
    executable = _resolve_executable(executable_path)
    try:
        request_text = json.dumps(request, allow_nan=False, ensure_ascii=False, indent=2)
    except (TypeError, ValueError) as error:
        raise ValueError(f"Request cannot be encoded as JSON: {error}") from error

    runtime_dir = PROJECT_ROOT / "runtime"
    runtime_dir.mkdir(parents=True, exist_ok=True)
    try:
        with tempfile.TemporaryDirectory(prefix="engine-", dir=runtime_dir) as work_dir:
            request_file = Path(work_dir) / "request.json"
            response_file = Path(work_dir) / "response.json"
            request_file.write_text(request_text, encoding="utf-8")
            try:
                process = subprocess.run(
                    [str(executable), str(request_file), str(response_file)],
                    cwd=PROJECT_ROOT,
                    capture_output=True,
                    text=True,
                    timeout=timeout_seconds,
                    check=False,
                )
            except subprocess.TimeoutExpired as error:
                raise EngineError("ENGINE_TIMEOUT", f"C++ engine exceeded {timeout_seconds:g}s timeout") from error
            if not response_file.is_file():
                detail = process.stderr.strip() or "Engine did not create a response file."
                raise EngineError("MISSING_RESPONSE", detail)
            try:
                response = json.loads(response_file.read_text(encoding="utf-8"))
            except (OSError, UnicodeError, json.JSONDecodeError) as error:
                raise EngineError("MALFORMED_RESPONSE", f"Could not parse engine response: {error}") from error
            if not isinstance(response, dict):
                raise EngineError("MALFORMED_RESPONSE", "Engine response must be a JSON object.")
            if process.returncode != 0 and response.get("success") is not False:
                detail = process.stderr.strip() or f"Engine exited with status {process.returncode}."
                raise EngineError("ENGINE_EXIT", detail)
            if response.get("success") is False:
                error_info = response.get("error")
                if not isinstance(error_info, dict):
                    raise EngineError("MALFORMED_RESPONSE", "Failure response is missing its error object.")
                raise EngineError(str(error_info.get("code", "ENGINE_ERROR")), str(error_info.get("message", "Engine request failed.")))
            required = {"schema_version", "success", "algorithm", "result", "metrics", "steps"}
            if not required.issubset(response) or response.get("schema_version") != 1 or response.get("success") is not True:
                raise EngineError("MALFORMED_RESPONSE", "Engine success response is missing required protocol fields.")
            if process.returncode != 0:
                detail = process.stderr.strip() or f"Engine exited with status {process.returncode}."
                raise EngineError("ENGINE_EXIT", detail)
            return response
    except OSError as error:
        raise EngineError("ENGINE_IO_ERROR", f"Could not run the C++ engine: {error}") from error


def run_algorithm(
    algorithm: str,
    network: Mapping[str, Any],
    parameters: Mapping[str, Any] | None = None,
    *,
    executable_path: str | os.PathLike[str] | None = None,
    timeout_seconds: float = 10.0,
) -> dict[str, Any]:
    """Build and execute one version 1 algorithm request."""
    return run_request(
        build_request(algorithm, network, parameters),
        executable_path=executable_path,
        timeout_seconds=timeout_seconds,
    )
