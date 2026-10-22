#!/usr/bin/env bash
# Detiene los procesos que levantó scripts/iniciar_sistema.sh.
set -uo pipefail
cd "$(dirname "$0")/.."

[[ -f logs/pids ]] || { echo "No hay procesos registrados en logs/pids"; exit 0; }
while read -r pid; do
  kill "$pid" 2>/dev/null
done <logs/pids
rm -f logs/pids
echo "Sistema detenido"
