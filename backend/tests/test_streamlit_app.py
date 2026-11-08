"""Streamlit page smoke tests using Streamlit's AppTest runner."""

import unittest

from streamlit.testing.v1 import AppTest


class StreamlitAppTests(unittest.TestCase):
    def setUp(self):
        self.app = AppTest.from_file("frontend/app.py", default_timeout=20).run()

    def test_all_pages_render_without_exceptions(self):
        self.assertFalse(self.app.exception)
        for page in ("Network Editor", "Algorithm Lab", "Risk Analysis", "What-If Simulator", "Benchmark Lab"):
            with self.subTest(page=page):
                self.app.sidebar.radio[0].set_value(page).run()
                self.assertFalse(self.app.exception)

    def test_algorithm_page_runs_cpp_engine(self):
        self.app.sidebar.radio[0].set_value("Algorithm Lab").run()
        self.app.selectbox[0].set_value("dijkstra").run()
        self.app.button[0].click().run()
        self.assertFalse(self.app.exception)
        self.assertTrue(any("Path:" in item.value for item in self.app.markdown))

    def test_simulator_compares_baseline_and_simulation(self):
        self.app.sidebar.radio[0].set_value("What-If Simulator").run()
        button = next(item for item in self.app.button if item.label == "Compare baseline and simulation")
        button.click().run()
        self.assertFalse(self.app.exception)
        self.assertTrue(any("Baseline" in item.value for item in self.app.markdown))
        self.assertTrue(any("Simulation" in item.value for item in self.app.markdown))

    def test_benchmark_page_runs_and_displays_measurements(self):
        self.app.sidebar.radio[0].set_value("Benchmark Lab").run()
        self.app.multiselect[0].set_value([10])
        self.app.number_input[0].set_value(1)
        self.app.run(timeout=30)
        button = next(item for item in self.app.button if item.label == "Run benchmarks")
        button.click().run(timeout=30)
        self.assertFalse(self.app.exception)
        self.assertEqual(len(self.app.session_state["benchmark_rows"]), 3)


if __name__ == "__main__":
    unittest.main()
