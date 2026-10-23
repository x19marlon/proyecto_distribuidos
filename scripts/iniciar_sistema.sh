#!/usr/bin/env bash
# Levanta en esta máquina el gestor, los procesadores RMC y persistencia.
#
#   scripts/iniciar_sistema.sh [base|escalable]
#
#   base       1 procesador RMC
#   escalable  3 procesadores RMC (el gestor reparte la carga entre ellos)
#
# Los logs quedan en logs/ y los PID en logs/pids. Para detener: scripts/detener_sistema.sh
set -euo pipefail
cd "$(dirname "$0")/.."

arquitectura="${1:-base}"
case "$arquitectura" in
  base) procesadores=1 ;;
  escalable) procesadores=3 ;;
  *) echo "Uso: $0 [base|escalable]" >&2; exit 2 ;;
esac

if [[ -f logs/pids ]]; then
  echo "Ya hay un sistema en ejecución (logs/pids). Deténgalo con scripts/detener_sistema.sh" >&2
  exit 1
fi
mkdir -p logs

iniciar() {
  local nombre="$1"; shift
  "$@" >"logs/$nombre.log" 2>&1 &
  echo $! >>logs/pids
  echo "  $nombre (pid $!)"
}

echo "Iniciando arquitectura $arquitectura:"
iniciar persistencia bin/persistencia
iniciar gestor-transacciones bin/gestor-transacciones
for i in $(seq 1 "$procesadores"); do
  iniciar "procesador-rmc-$i" bin/procesador-rmc
done

sleep 1
echo "Listo. Cliente interactivo: bin/cliente EST-1"
