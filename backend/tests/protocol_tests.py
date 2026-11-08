"""Executable-level checks for the file-based C++ JSON protocol."""

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ENGINE = ROOT / "build" / "cyberpath_engine.exe"
SAMPLE = ROOT / "data" / "sample_requests" / "dijkstra.json"


class ProtocolTests(unittest.TestCase):
    def invoke(self, request_text):
        with tempfile.TemporaryDirectory() as directory:
            request_path = Path(directory) / "request.json"
            response_path = Path(directory) / "response.json"
            request_path.write_text(request_text, encoding="utf-8")
            process = subprocess.run(
                [str(ENGINE), str(request_path), str(response_path)],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertTrue(response_path.exists())
            return process, json.loads(response_path.read_text(encoding="utf-8"))

    def test_algorithms_write_valid_responses_without_stdout(self):
        base = json.loads(SAMPLE.read_text(encoding="utf-8"))
        expected = {
            "bfs": [1, 2, 3, 4],
            "dfs": [1, 2, 3, 4],
            "dijkstra": [1, 2, 3],
        }
        for algorithm in ("bfs", "dfs", "dijkstra", "risk"):
            with self.subTest(algorithm=algorithm):
                request = dict(base, algorithm=algorithm)
                request["parameters"] = {"source_id": 1}
                if algorithm == "dijkstra":
                    request["parameters"]["target_id"] = 3
                process, response = self.invoke(json.dumps(request))
                self.assertEqual(process.returncode, 0, process.stderr)
                self.assertEqual(process.stdout, "")
                self.assertTrue(response["success"])
                self.assertEqual(response["schema_version"], 1)
                self.assertIn("metrics", response)
                self.assertIn("steps", response)
                if algorithm in expected:
                    key = "traversal_order" if algorithm != "dijkstra" else "path"
                    self.assertEqual(response["result"][key], expected[algorithm])
                if algorithm == "dijkstra":
                    self.assertEqual(response["result"]["total_cost"], 3.5)

    def test_bad_json_writes_structured_error(self):
        process, response = self.invoke("{")
        self.assertEqual(process.returncode, 1)
        self.assertEqual(process.stdout, "")
        self.assertFalse(response["success"])
        self.assertEqual(response["error"]["code"], "INVALID_JSON")

    def test_missing_field_writes_structured_error(self):
        process, response = self.invoke('{"schema_version":1}')
        self.assertEqual(process.returncode, 1)
        self.assertEqual(process.stdout, "")
        self.assertFalse(response["success"])
        self.assertEqual(response["error"]["code"], "INVALID_REQUEST")


if __name__ == "__main__":
    unittest.main()
