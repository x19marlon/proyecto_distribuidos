// Proceso de la Capa de Servicios de Negocio: Gestor de Transacciones.
#include "comun/Utilidades.hpp"
#include "servicios/gestor/GestorTransacciones.hpp"

int main() {
    GestorTransacciones gestor(util::variableEntorno("GESTOR_FRONTEND_BIND", "tcp://*:5555"),
                               util::variableEntorno("GESTOR_BACKEND_BIND", "tcp://*:5556"));
    return gestor.ejecutar();
}
