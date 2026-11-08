"""CyberPath defensive network analysis dashboard."""

from __future__ import annotations

import json
import os
from pathlib import Path

import pandas as pd
import plotly.express as px
import streamlit as st

from frontend.services.cpp_runner import EngineError, run_algorithm
from frontend.services.benchmark import run_benchmarks
from frontend.services.network import PROJECT_ROOT, graph_figure, load_sample_network, validate_network


st.set_page_config(page_title="CyberPath", page_icon="ðŸ›¡ï¸", layout="wide")


def _initialize_state() -> None:
    if "network" not in st.session_state:
        sample = load_sample_network("campus_network.json")
        st.session_state.network = sample


def _risk_data(network):
    response = run_algorithm("risk", network)
    ranked = response["result"]["ranked_devices"]
    categories = {int(item["id"]): item["category"] for item in ranked}
    return ranked, categories


def _analysis(algorithm, network, source=None, target=None):
    parameters = {}
    if algorithm in ("bfs", "dfs", "dijkstra"):
        parameters["source_id"] = int(source)
    if algorithm == "dijkstra":
        parameters["target_id"] = int(target)
    return run_algorithm(algorithm, network, parameters)


def _show_engine_error(error: EngineError | ValueError) -> None:
    st.error(f"{getattr(error, 'code', 'INVALID_INPUT')}: {error}")


def _draw_network(network, risks=None, highlighted=None, highlighted_path=None, chart_key="network-graph"):
    if not network["devices"]:
        st.info("Add devices to see the network graph.")
        return
    st.plotly_chart(
        graph_figure(network, risks=risks, highlighted_ids=highlighted, highlighted_path=highlighted_path),
        width="stretch",
        key=chart_key,
        config={"displayModeBar": False},
    )
    st.caption("Node colors show backend risk categories when available. â˜… marks a critical asset; dashed edges are inactive.")


def _network_editor():
    st.subheader("Network editor")
    st.caption("Edit devices and weighted connections. Save validates the complete network.")
    device_frame = pd.DataFrame(st.session_state.network["devices"], columns=[
        "id", "name", "ip_address", "type", "vulnerability_score", "exposure_score",
        "is_critical", "is_active",
    ])
    edge_frame = pd.DataFrame(st.session_state.network["edges"], columns=[
        "source_id", "destination_id", "cost", "is_active",
    ])
    left, right = st.columns(2)
    with left:
        edited_devices = st.data_editor(
            device_frame, num_rows="dynamic", hide_index=True, key="device_editor",
            column_config={
                "id": st.column_config.NumberColumn("ID", step=1),
                "vulnerability_score": st.column_config.NumberColumn("Vulnerability", min_value=0, max_value=100),
                "exposure_score": st.column_config.NumberColumn("Exposure", min_value=0, max_value=100),
            },
        )
    with right:
        edited_edges = st.data_editor(
            edge_frame, num_rows="dynamic", hide_index=True, key="edge_editor",
            column_config={"source_id": st.column_config.NumberColumn(step=1),
                           "destination_id": st.column_config.NumberColumn(step=1),
                           "cost": st.column_config.NumberColumn(min_value=0)},
        )
    if st.button("Save network", type="primary"):
        try:
            devices = edited_devices.dropna(subset=["id", "name", "ip_address", "type"]).to_dict("records")
            edges = edited_edges.dropna(subset=["source_id", "destination_id", "cost"]).to_dict("records")
            candidate = {"devices": devices, "edges": edges}
            validate_network(candidate)
            st.session_state.network = candidate
            st.success("Network saved.")
            st.rerun()
        except (ValueError, TypeError) as error:
            _show_engine_error(error)

    st.markdown("#### Add a sample network")
    sample_names = sorted(path.name for path in (PROJECT_ROOT / "data" / "sample_networks").glob("*.json"))
    if sample_names:
        selected = st.selectbox("Bundled sample", sample_names)
        if st.button("Load sample and replace current network"):
            try:
                network = load_sample_network(selected)
                st.session_state.network = network
                st.success(f"Loaded {selected}.")
                st.rerun()
            except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
                _show_engine_error(ValueError(str(error)))

    with st.expander("Create an empty network"):
        st.write("This clears the current graph.")
        if st.button("Create empty network", type="secondary"):
            empty = {"devices": [], "edges": []}
            st.session_state.network = empty
            st.rerun()


def _dashboard(network):
    st.title("CyberPath")
    st.caption("Explore a fictional network using defensive graph and risk analysis.")
    devices, edges = network["devices"], network["edges"]
    active_devices = sum(device.get("is_active", True) for device in devices)
    active_edges = sum(edge.get("is_active", True) for edge in edges)
    metrics = st.columns(4)
    metrics[0].metric("Devices", len(devices))
    metrics[1].metric("Active devices", active_devices)
    metrics[2].metric("Connections", len(edges))
    metrics[3].metric("Active connections", active_edges)
    if devices:
        try:
            ranked, categories = _risk_data(network)
            top = ranked[0]
            st.info(f"Highest calculated risk: **{top['name']}** Â· {top['risk_score']:.1f}/100 Â· {top['category']}")
            _draw_network(network, categories)
        except EngineError as error:
            _show_engine_error(error)
            _draw_network(network)
    else:
        _draw_network(network)
    st.markdown("Use the sidebar to edit a network, run algorithms, inspect risk, or compare algorithm performance.")


def _algorithm_lab(network):
    st.title("Algorithm Lab")
    if not network["devices"]:
        st.info("Add or load devices in Network Editor first.")
        return
    device_by_id = {int(device["id"]): device["name"] for device in network["devices"]}
    identifiers = list(device_by_id)
    algorithm = st.selectbox("Algorithm", ["bfs", "dfs", "dijkstra"], format_func=lambda value: value.upper())
    source = st.selectbox("Source device", identifiers, format_func=lambda value: f"{device_by_id[value]} (ID {value})")
    target = None
    if algorithm == "dijkstra":
        target = st.selectbox("Target device", identifiers, index=min(1, len(identifiers) - 1), format_func=lambda value: f"{device_by_id[value]} (ID {value})")
    if st.button("Run analysis", type="primary"):
        try:
            response = _analysis(algorithm, network, source, target)
            result = response["result"]
            st.success(f"{algorithm.upper()} completed")
            if algorithm == "dijkstra":
                st.metric("Path cost", "Unreachable" if not result["reachable"] else f"{result['total_cost']:g}")
                if result["reachable"]:
                    st.write("Path: " + " â†’ ".join(device_by_id[item] for item in result["path"]))
                    highlighted = set(result["path"])
                    highlighted_path = result["path"]
            else:
                st.write("Traversal order: " + " â†’ ".join(device_by_id[item] for item in result["traversal_order"]))
                if algorithm == "bfs":
                    st.write("Hop distances", result["hop_distances"])
                highlighted = set(result["reachable_devices"])
                highlighted_path = None
            st.json(response["metrics"])
            _draw_network(network, highlighted=highlighted, highlighted_path=highlighted_path)
        except (EngineError, KeyError) as error:
            _show_engine_error(error if isinstance(error, EngineError) else ValueError(str(error)))
    else:
        _draw_network(network)


def _risk_page(network):
    st.title("Risk Analysis")
    st.caption("Risk score is the average of vulnerability and exposure, each scored from 0 to 100.")
    if not network["devices"]:
        st.info("Add or load devices in Network Editor first.")
        return
    try:
        ranked, categories = _risk_data(network)
        components = {int(item["id"]): item for item in network["devices"]}
        table = []
        for item in ranked:
            device = components[int(item["id"])]
            table.append({"Rank": len(table) + 1, "Device": item["name"], "ID": item["id"],
                          "Vulnerability": device["vulnerability_score"], "Exposure": device["exposure_score"],
                          "Risk score": item["risk_score"], "Category": item["category"]})
        frame = pd.DataFrame(table)
        st.dataframe(frame, hide_index=True, width="stretch")
        left, right = st.columns(2)
        with left:
            chart = px.bar(frame, x="Device", y="Risk score", color="Category", color_discrete_map={"Low": "#16a34a", "Moderate": "#f59e0b", "High": "#dc2626"}, range_y=[0, 100])
            st.plotly_chart(chart, width="stretch")
        with right:
            counts = frame["Category"].value_counts().rename_axis("Category").reset_index(name="Devices")
            st.plotly_chart(px.pie(counts, names="Category", values="Devices", color="Category", color_discrete_map={"Low": "#16a34a", "Moderate": "#f59e0b", "High": "#dc2626"}), width="stretch")
        _draw_network(network, categories)
    except EngineError as error:
        _show_engine_error(error)


def _benchmark_page():
    st.title("Benchmark Lab")
    st.caption("Repeatable synthetic graphs compare C++ algorithm operation counts and timing.")
    st.info("C++ algorithm time excludes JSON parsing and graph construction. End-to-end time includes Python serialization, file exchange, process startup, and engine work.")
    with st.form("benchmark_controls"):
        topology = st.selectbox("Topology", ["sparse", "dense"], help="Sparse graphs include a connected chain and seeded chords; dense graphs include every undirected pair.")
        sizes = st.multiselect("Device counts", [10, 25, 50, 75, 100, 150], default=[10, 25, 50])
        repeats = st.number_input("Trials per algorithm and size", min_value=1, max_value=10, value=3, step=1)
        seed = st.number_input("Dataset seed", min_value=0, max_value=2_000_000_000, value=42, step=1)
        submitted = st.form_submit_button("Run benchmarks", type="primary", disabled=not sizes)
    if submitted:
        try:
            with st.spinner("Running C++ benchmarks…"):
                st.session_state.benchmark_rows = run_benchmarks(
                    sizes, topology=topology, repeats=int(repeats), seed=int(seed)
                )
            st.session_state.benchmark_description = f"{topology}, seed {seed}, {repeats} trial(s)"
        except (ValueError, EngineError) as error:
            _show_engine_error(error if isinstance(error, EngineError) else ValueError(str(error)))
            return
    rows = st.session_state.get("benchmark_rows")
    if not rows:
        st.info("Choose graph sizes and run a benchmark to see measured results.")
        return
    st.caption(f"Latest run: {st.session_state.get('benchmark_description', '')}. Repeat with the same settings to reproduce the dataset and operation counts; elapsed times vary with system load.")
    frame = pd.DataFrame(rows)
    st.dataframe(frame, hide_index=True, width="stretch")
    time_tab, operations_tab = st.tabs(["Elapsed time", "Operation counts"])
    with time_tab:
        algorithm_time = px.line(frame, x="devices", y="median_algorithm_time_us", color="algorithm", markers=True, title="Median C++ algorithm time (microseconds)")
        st.plotly_chart(algorithm_time, width="stretch", key="benchmark-algorithm-time")
        total_time = px.line(frame, x="devices", y="median_end_to_end_ms", color="algorithm", markers=True, title="Median Python-to-C++ time (milliseconds)")
        st.plotly_chart(total_time, width="stretch", key="benchmark-end-to-end-time")
    with operations_tab:
        operations = px.line(frame, x="devices", y="edges_examined", color="algorithm", markers=True, title="Median adjacency entries examined")
        st.plotly_chart(operations, width="stretch", key="benchmark-operations")
        st.caption("Operation counts are reported by the C++ implementation. Dijkstra also reports heap pushes and pops in the table.")
    st.download_button("Download results as CSV", frame.to_csv(index=False), file_name="cyberpath_benchmarks.csv", mime="text/csv")


def main() -> None:
    _initialize_state()
    st.sidebar.title("CyberPath")
    st.sidebar.caption("Defensive network analysis")
    pages = ["Dashboard", "Network Editor", "Algorithm Lab", "Risk Analysis", "Benchmark Lab"]
    page = st.sidebar.radio("Navigate", pages)
    executable = os.environ.get("CYBERPATH_ENGINE_PATH", "build/cyberpath_engine.exe (default)")
    st.sidebar.caption(f"C++ engine: {executable}")
    network = st.session_state.network
    if page == "Dashboard":
        _dashboard(network)
    elif page == "Network Editor":
        st.title("Network Editor")
        _network_editor()
        _draw_network(network)
    elif page == "Algorithm Lab":
        _algorithm_lab(network)
    elif page == "Risk Analysis":
        _risk_page(network)
    elif page == "Benchmark Lab":
        _benchmark_page()


main()
