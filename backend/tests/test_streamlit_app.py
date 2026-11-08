"""Streamlit page smoke tests using Streamlit's AppTest runner."""

import unittest

from streamlit.testing.v1 import AppTest


class StreamlitAppTests(unittest.TestCase):
    def setUp(self):
        self.app = AppTest.from_file("frontend/app.py", default_timeout=20).run()

    def test_all_pages_render_without_exceptions(self):
        self.assertFalse(self.app.exception)
        for page in ("Network Editor", "Algorithm Lab", "Risk Analysis"):
            with self.subTest(page=page):
                self.app.sidebar.radio[0].set_value(page).run()
                self.assertFalse(self.app.exception)

    def test_algorithm_page_runs_cpp_engine(self):
        self.app.sidebar.radio[0].set_value("Algorithm Lab").run()
        self.app.selectbox[0].set_value("dijkstra").run()
        self.app.button[0].click().run()
        self.assertFalse(self.app.exception)
        self.assertTrue(any("Path:" in item.value for item in self.app.markdown))


if __name__ == "__main__":
    unittest.main()
