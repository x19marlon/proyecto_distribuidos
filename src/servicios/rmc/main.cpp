// Proceso de la Capa de Servicios de Negocio: Procesamiento de reservas, modificaciones y consultas.
#include "comun/Utilidades.hpp"
#include "servicios/rmc/ProcesadorRMC.hpp"

int main() {
    ProcesadorRMC procesador(
        util::variableEntorno("GESTOR_BACKEND", "tcp://localhost:5556"),
        util::variableEntorno("PERSISTENCIA_ENDPOINT", "tcp://localhost:5560"));
    return procesador.ejecutar();
}
