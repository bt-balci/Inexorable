#!/usr/bin/env bash
# INEXORABLE: Automated Performance Benchmark Runner
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Executing Inexorable Benchmarking Suite under WSL2 KVM..."
python3 "$ROOT_DIR/scripts/benchmark.py"
