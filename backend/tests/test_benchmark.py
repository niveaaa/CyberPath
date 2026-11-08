"""Tests for repeatable synthetic graph benchmarks."""

import unittest

from frontend.services.benchmark import generate_network, run_benchmarks


class BenchmarkTests(unittest.TestCase):
    def test_seed_reproduces_sparse_network(self):
        first = generate_network(12, "sparse", 90)
        second = generate_network(12, "sparse", 90)
        self.assertEqual(first, second)
        self.assertEqual(len(first["edges"]), 36)

    def test_dense_edge_count_is_complete_undirected_graph(self):
        network = generate_network(8, "dense", 7)
        self.assertEqual(len(network["edges"]), 8 * 7 // 2)

    def test_benchmark_reports_backend_counts_and_two_time_scales(self):
        rows = run_benchmarks([8], topology="sparse", repeats=1, seed=5)
        repeated = run_benchmarks([8], topology="sparse", repeats=1, seed=5)
        self.assertEqual({row["algorithm"] for row in rows}, {"BFS", "DFS", "DIJKSTRA"})
        for row, repeated_row in zip(rows, repeated):
            self.assertEqual(row["devices"], 8)
            self.assertGreater(row["vertices_visited"], 0)
            self.assertLessEqual(row["vertices_visited"], 8)
            self.assertGreaterEqual(row["edges_examined"], 0)
            self.assertGreaterEqual(row["median_algorithm_time_us"], 0)
            self.assertGreater(row["median_end_to_end_ms"], 0)
            for field in ("edges", "vertices_visited", "edges_examined", "heap_pushes", "heap_pops"):
                self.assertEqual(row[field], repeated_row[field])

    def test_invalid_benchmark_configuration_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "Topology"):
            generate_network(4, "random")
        with self.assertRaisesRegex(ValueError, "Repeat count"):
            run_benchmarks([4], repeats=0)


if __name__ == "__main__":
    unittest.main()
