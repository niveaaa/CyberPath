"""Repeatable synthetic network generation and C++ algorithm benchmarks."""

from __future__ import annotations

import random
import statistics
import time
from typing import Any, Iterable

from frontend.services.cpp_runner import run_algorithm


ALGORITHMS = ("bfs", "dfs", "dijkstra")


def generate_network(size: int, topology: str = "sparse", seed: int = 42) -> dict[str, list[dict[str, Any]]]:
    """Create a deterministic, connected graph with unique synthetic addresses."""
    if size < 1:
        raise ValueError("Network size must be at least one device.")
    if topology not in {"sparse", "dense"}:
        raise ValueError("Topology must be 'sparse' or 'dense'.")
    devices = []
    for identifier in range(1, size + 1):
        devices.append({
            "id": identifier,
            "name": f"Device {identifier}",
            "ip_address": f"198.51.{(identifier - 1) // 254}.{(identifier - 1) % 254 + 1}",
            "type": "server" if identifier % 5 == 0 else "workstation",
            "vulnerability_score": float((identifier * 17) % 101),
            "exposure_score": float((identifier * 29) % 101),
            "connectivity_score": float((identifier * 11) % 101),
            "criticality_score": float((identifier * 7) % 101),
            "is_critical": identifier % 10 == 0,
            "is_active": True,
        })

    pairs: set[tuple[int, int]] = set()
    if topology == "dense":
        pairs = {(source, target) for source in range(1, size + 1) for target in range(source + 1, size + 1)}
    elif size > 1:
        # A chain keeps every node reachable; seeded chords add sparse variation.
        pairs.update((identifier, identifier + 1) for identifier in range(1, size))
        rng = random.Random(seed)
        desired_edges = min(size * 3, size * (size - 1) // 2)
        while len(pairs) < desired_edges:
            source, target = rng.sample(range(1, size + 1), 2)
            pairs.add(tuple(sorted((source, target))))

    edges = [
        {"source_id": source, "destination_id": target,
         "cost": float(1 + ((source * 13 + target * 7) % 20)), "is_active": True}
        for source, target in sorted(pairs)
    ]
    return {"devices": devices, "edges": edges}


def run_benchmarks(
    sizes: Iterable[int],
    *,
    topology: str = "sparse",
    repeats: int = 3,
    seed: int = 42,
    algorithms: Iterable[str] = ALGORITHMS,
) -> list[dict[str, Any]]:
    """Return medians of C++ algorithm time, end-to-end time, and operation counts."""
    if repeats < 1:
        raise ValueError("Repeat count must be at least one.")
    selected_algorithms = tuple(algorithms)
    if not selected_algorithms or any(algorithm not in ALGORITHMS for algorithm in selected_algorithms):
        raise ValueError(f"Algorithms must be selected from {', '.join(ALGORITHMS)}.")
    rows = []
    for size in sizes:
        network = generate_network(int(size), topology, seed + int(size))
        parameters = {"source_id": 1}
        if "dijkstra" in selected_algorithms:
            parameters["target_id"] = int(size)
        measured: dict[str, list[dict[str, Any]]] = {algorithm: [] for algorithm in selected_algorithms}
        for _ in range(repeats):
            for algorithm in selected_algorithms:
                started = time.perf_counter_ns()
                response = run_algorithm(algorithm, network, parameters)
                end_to_end_ms = (time.perf_counter_ns() - started) / 1_000_000
                metrics = response["metrics"]
                measured[algorithm].append({
                    "algorithm_time_us": metrics.get("algorithm_time_us", 0),
                    "end_to_end_ms": end_to_end_ms,
                    "vertices_visited": metrics.get("vertices_visited", 0),
                    "edges_examined": metrics.get("edges_examined", 0),
                    "heap_pushes": metrics.get("heap_pushes", 0),
                    "heap_pops": metrics.get("heap_pops", 0),
                })
        for algorithm, trials in measured.items():
            rows.append({
                "algorithm": algorithm.upper(),
                "topology": topology,
                "seed": seed + int(size),
                "devices": int(size),
                "edges": len(network["edges"]),
                "repeats": repeats,
                "median_algorithm_time_us": statistics.median(t["algorithm_time_us"] for t in trials),
                "median_end_to_end_ms": statistics.median(t["end_to_end_ms"] for t in trials),
                "vertices_visited": statistics.median(t["vertices_visited"] for t in trials),
                "edges_examined": statistics.median(t["edges_examined"] for t in trials),
                "heap_pushes": statistics.median(t["heap_pushes"] for t in trials),
                "heap_pops": statistics.median(t["heap_pops"] for t in trials),
            })
    return rows
