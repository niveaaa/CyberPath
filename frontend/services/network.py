"""Sample loading, validation, and graph display helpers."""

from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Any, Mapping

import plotly.graph_objects as go


PROJECT_ROOT = Path(__file__).resolve().parents[2]
SAMPLE_DIR = PROJECT_ROOT / "data" / "sample_networks"


def load_sample_network(filename: str) -> dict[str, list[dict[str, Any]]]:
    """Load a bundled network and keep only the protocol network fields."""
    path = (SAMPLE_DIR / filename).resolve()
    if path.parent != SAMPLE_DIR.resolve() or not path.is_file():
        raise ValueError("Unknown sample network.")
    sample = json.loads(path.read_text(encoding="utf-8"))
    network = {"devices": sample["devices"], "edges": sample["edges"]}
    validate_network(network)
    return network


def validate_network(network: Mapping[str, Any]) -> None:
    """Check frontend-editable network invariants before saving or running."""
    if not isinstance(network, Mapping):
        raise ValueError("Network must be an object.")
    devices = network.get("devices")
    edges = network.get("edges")
    if not isinstance(devices, list) or not isinstance(edges, list):
        raise ValueError("Network must contain device and connection lists.")
    ids: set[int] = set()
    names: set[str] = set()
    addresses: set[str] = set()
    for device in devices:
        try:
            raw_identifier = device["id"]
            identifier = int(raw_identifier)
            if isinstance(raw_identifier, bool) or float(raw_identifier) != identifier:
                raise ValueError("ID must be a whole number")
            if not all(isinstance(device[field], str) for field in ("name", "ip_address", "type")):
                raise ValueError("name, ip_address, and type must be text")
            name = device["name"].strip()
            address = device["ip_address"].strip()
            kind = device["type"].strip()
        except (KeyError, TypeError, ValueError) as error:
            raise ValueError("Every device needs an integer ID, name, IP address, and type.") from error
        if identifier in ids or not name or name.casefold() in names or not address or address in addresses:
            raise ValueError("Device IDs, names, and IP addresses must be unique and non-empty.")
        if not kind:
            raise ValueError(f"Device {identifier} needs a type.")
        ids.add(identifier)
        names.add(name.casefold())
        addresses.add(address)
        for field in ("vulnerability_score", "exposure_score"):
            try:
                score = float(device[field])
            except (KeyError, TypeError, ValueError) as error:
                raise ValueError(f"Device {identifier}: {field} must be a number from 0 to 100.") from error
            if not math.isfinite(score) or not 0 <= score <= 100:
                raise ValueError(f"Device {identifier}: {field} must be from 0 to 100.")
        if "is_active" in device and not isinstance(device["is_active"], bool):
            raise ValueError(f"Device {identifier}: is_active must be true or false.")

    connections: set[tuple[int, int]] = set()
    for edge in edges:
        try:
            raw_source, raw_target = edge["source_id"], edge["destination_id"]
            source, target = int(raw_source), int(raw_target)
            if (isinstance(raw_source, bool) or float(raw_source) != source or
                    isinstance(raw_target, bool) or float(raw_target) != target):
                raise ValueError("Connection endpoints must be whole numbers")
            cost = float(edge["cost"])
        except (KeyError, TypeError, ValueError) as error:
            raise ValueError("Every connection needs source_id, destination_id, and a numeric cost.") from error
        pair = tuple(sorted((source, target)))
        if source == target or source not in ids or target not in ids:
            raise ValueError("Connections must join two different existing devices.")
        if pair in connections:
            raise ValueError("Duplicate connections are not allowed.")
        if not math.isfinite(cost) or cost < 0:
            raise ValueError("Connection costs must be finite and non-negative.")
        if "is_active" in edge and not isinstance(edge["is_active"], bool):
            raise ValueError("Connection is_active must be true or false.")
        connections.add(pair)


def graph_figure(
    network: Mapping[str, Any],
    *,
    risks: Mapping[int, str] | None = None,
    highlighted_ids: set[int] | None = None,
    highlighted_path: list[int] | None = None,
):
    """Draw a deterministic graph view; algorithm results remain C++ owned."""
    devices = network["devices"]
    edges = network["edges"]
    risks = risks or {}
    highlighted_ids = highlighted_ids or set()
    path_edges = {
        tuple(sorted((int(source), int(target))))
        for source, target in zip(highlighted_path or [], (highlighted_path or [])[1:])
    }
    ordered = sorted(devices, key=lambda item: int(item["id"]))
    positions: dict[int, tuple[float, float]] = {}
    count = len(ordered)
    for index, device in enumerate(ordered):
        angle = 2 * math.pi * index / max(count, 1) - math.pi / 2
        positions[int(device["id"])] = (math.cos(angle), math.sin(angle))

    figure = go.Figure()
    for edge in edges:
        source, target = int(edge["source_id"]), int(edge["destination_id"])
        x1, y1 = positions[source]
        x2, y2 = positions[target]
        active = edge.get("is_active", True)
        on_path = tuple(sorted((source, target))) in path_edges
        figure.add_trace(go.Scatter(
            x=[x1, x2], y=[y1, y2], mode="lines",
            line={"color": "#7c3aed" if on_path else ("#718096" if active else "#cbd5e0"),
                  "width": 6 if on_path else (3 if active else 1),
                  "dash": "solid" if active else "dash"},
            hovertemplate=f"{source} ↔ {target}<br>Cost: {float(edge['cost']):g}<extra></extra>",
            showlegend=False,
        ))
        figure.add_annotation(x=(x1 + x2) / 2, y=(y1 + y2) / 2, text=str(edge["cost"]), showarrow=False,
                              font={"size": 13, "color": "#111827"}, bgcolor="#ffffff", bordercolor="#64748b", borderwidth=1)

    colors = {"High": "#dc2626", "Moderate": "#f59e0b", "Low": "#16a34a"}
    for category, color in colors.items():
        category_devices = [d for d in ordered if risks.get(int(d["id"])) == category]
        if not category_devices:
            continue
        figure.add_trace(go.Scatter(
            x=[positions[int(d["id"])][0] for d in category_devices],
            y=[positions[int(d["id"])][1] for d in category_devices],
            mode="markers+text", text=[d["name"] for d in category_devices],
            textposition="top center", textfont={"color": "#111827", "size": 14}, name=f"{category} risk",
            marker={"size": [24 if int(d["id"]) in highlighted_ids else 18 for d in category_devices],
                    "color": color, "line": {"color": "#1e293b", "width": 1}},
            customdata=[[d["id"], d["ip_address"], d.get("type", ""), "Active" if d.get("is_active", True) else "Inactive"] for d in category_devices],
            hovertemplate="%{text}<br>ID %{customdata[0]}<br>%{customdata[1]}<br>%{customdata[2]}<br>%{customdata[3]}<extra></extra>",
        ))
    ranked_ids = {int(device["id"]) for device in ordered if risks.get(int(device["id"])) in colors}
    unranked = [device for device in ordered if int(device["id"]) not in ranked_ids]
    if unranked:
        figure.add_trace(go.Scatter(
            x=[positions[int(d["id"])][0] for d in unranked], y=[positions[int(d["id"])][1] for d in unranked],
            mode="markers+text", text=[d["name"] for d in unranked],
            textposition="top center", textfont={"color": "#111827", "size": 14}, name="Device",
            marker={"size": [24 if int(d["id"]) in highlighted_ids else 18 for d in unranked], "color": "#60a5fa", "line": {"color": "#1e293b", "width": 1}},
            customdata=[[d["id"], d["ip_address"], d.get("type", ""), "Active" if d.get("is_active", True) else "Inactive"] for d in unranked],
            hovertemplate="%{text}<br>ID %{customdata[0]}<br>%{customdata[1]}<br>%{customdata[2]}<br>%{customdata[3]}<extra></extra>",
        ))
    selected_devices = [device for device in ordered if int(device["id"]) in highlighted_ids]
    if selected_devices:
        figure.add_trace(go.Scatter(
            x=[positions[int(device["id"])][0] for device in selected_devices],
            y=[positions[int(device["id"])][1] for device in selected_devices],
            mode="markers", name="Visited / path",
            marker={"symbol": "circle-open", "size": 31,
                    "line": {"color": "#7c3aed", "width": 4}},
            hoverinfo="skip",
        ))
    figure.update_layout(height=500, margin={"l": 10, "r": 10, "t": 20, "b": 10},
                         xaxis={"visible": False, "range": [-1.35, 1.35]}, yaxis={"visible": False, "range": [-1.35, 1.35], "scaleanchor": "x"},
                         legend={"orientation": "h", "y": -0.08, "font": {"color": "#111827", "size": 13}},
                         plot_bgcolor="#f1f5f9", paper_bgcolor="#f1f5f9", font={"color": "#111827"})
    return figure
