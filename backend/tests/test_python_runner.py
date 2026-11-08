"""Python-to-C++ integration tests for the file based protocol."""

import json
import subprocess
import unittest
from unittest.mock import patch

from frontend.services.cpp_runner import EngineError, build_request, run_algorithm, run_request


class PythonRunnerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from pathlib import Path

        cls.network = json.loads(
            (Path(__file__).resolve().parents[2] / "data/sample_networks/campus_network.json")
            .read_text(encoding="utf-8")
        )
        cls.network = {"devices": cls.network["devices"], "edges": cls.network["edges"]}

    def test_request_builder_creates_versioned_payload(self):
        request = build_request("bfs", self.network, {"source_id": 1})
        self.assertEqual(request["schema_version"], 1)
        self.assertEqual(request["algorithm"], "bfs")
        self.assertEqual(request["network"], self.network)

    def test_all_backend_operations_from_python(self):
        cases = (
            ("bfs", {"source_id": 1}, "traversal_order"),
            ("dfs", {"source_id": 1}, "traversal_order"),
            ("dijkstra", {"source_id": 1, "target_id": 3}, "path"),
            ("risk", {}, "ranked_devices"),
        )
        for algorithm, parameters, result_key in cases:
            with self.subTest(algorithm=algorithm):
                response = run_algorithm(algorithm, self.network, parameters)
                self.assertTrue(response["success"])
                self.assertEqual(response["algorithm"], algorithm)
                self.assertIn(result_key, response["result"])

    def test_engine_failure_is_exposed_as_typed_error(self):
        with self.assertRaises(EngineError) as caught:
            run_algorithm("unknown", self.network)
        self.assertEqual(caught.exception.code, "INVALID_REQUEST")

    def test_timeout_is_reported(self):
        with patch("frontend.services.cpp_runner.subprocess.run", side_effect=subprocess.TimeoutExpired("engine", 0.01)):
            with self.assertRaises(EngineError) as caught:
                run_algorithm("risk", self.network, timeout_seconds=0.01)
        self.assertEqual(caught.exception.code, "ENGINE_TIMEOUT")

    def test_malformed_response_is_reported(self):
        def fake_run(command, **_kwargs):
            with open(command[2], "w", encoding="utf-8") as response_file:
                response_file.write("not json")
            return subprocess.CompletedProcess(command, 0, "", "")

        with patch("frontend.services.cpp_runner.subprocess.run", side_effect=fake_run):
            with self.assertRaises(EngineError) as caught:
                run_algorithm("risk", self.network)
        self.assertEqual(caught.exception.code, "MALFORMED_RESPONSE")


if __name__ == "__main__":
    unittest.main()
