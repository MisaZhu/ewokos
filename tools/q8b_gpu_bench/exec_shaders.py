#!/usr/bin/env python3
"""Compile identical built-ins with alternate workgroup shapes for the board sweep."""
import pathlib
import re
import subprocess
import sys

out = pathlib.Path(sys.argv[1])
source_dir = pathlib.Path(__file__).resolve().parents[2] / "machines/q8b/system/libs/adreno/shaders"
baseline_dir = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else source_dir
for name in ("blit", "raster"):
    source = (source_dir / f"{name}.comp").read_text()
    for x, y in ((8, 8), (32, 4), (64, 2), (64, 1), (16, 8), (64, 4)):
        # A saved source directory makes 8x8 the original shader baseline.
        variant = (baseline_dir / f"{name}.comp").read_text() if (x, y) == (8, 8) else source
        text, count = re.subn(r"layout\(local_size_x=\d+, local_size_y=\d+, local_size_z=1\) in;",
                             f"layout(local_size_x={x}, local_size_y={y}, local_size_z=1) in;", variant)
        if count != 1:
            raise ValueError(f"missing workgroup declaration: {name}")
        path = out / f"{name}-{x}x{y}.comp"
        path.write_text(text)
        subprocess.run([str(out / "compile"), str(path), str(path.with_suffix(".bin")), "raw"], check=True)
