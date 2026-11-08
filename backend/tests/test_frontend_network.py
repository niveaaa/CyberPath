"""Tests for user-editable network validation and graph display."""

import unittest

from frontend.services.network import graph_figure, load_sample_network, validate_network


class FrontendNetworkTests(unittest.TestCase):
    def test_sample_is_valid_and_has_devices_and_connections(self):
        network = load_sample_network("campus_network.json")
        validate_network(network)
        self.assertEqual(len(network["devices"]), 4)
        self.assertEqual(len(network["edges"]), 3)

    def test_duplicate_device_id_is_rejected(self):
        network = load_sample_network("campus_network.json")
        network["devices"].append(dict(network["devices"][0], name="Duplicate", ip_address="192.0.2.20"))
        with self.assertRaisesRegex(ValueError, "unique"):
            validate_network(network)

    def test_connection_to_unknown_device_is_rejected(self):
        network = load_sample_network("campus_network.json")
        network["edges"].append({"source_id": 1, "destination_id": 99, "cost": 2})
        with self.assertRaisesRegex(ValueError, "existing devices"):
            validate_network(network)

    def test_non_integer_device_id_is_rejected(self):
        network = load_sample_network("campus_network.json")
        network["devices"][0]["id"] = 1.5
        with self.assertRaisesRegex(ValueError, "integer ID"):
            validate_network(network)

    def test_graph_marks_path_edges(self):
        network = load_sample_network("campus_network.json")
        figure = graph_figure(network, highlighted_path=[1, 2, 3])
        highlighted = [trace for trace in figure.data if getattr(trace.line, "color", None) == "#7c3aed"]
        self.assertEqual(len(highlighted), 2)


if __name__ == "__main__":
    unittest.main()
