// Proceso de la Capa de Consumidores: cliente del estudiante.
//
// Uso: cliente EST-1
#include <iostream>

#include "comun/Utilidades.hpp"
#include "consumidores/Cliente.hpp"
#include "consumidores/ConsolaCliente.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <idEstudiante>   (por ejemplo EST-1)\n";
        return 2;
    }
    zmq::context_t contexto(1);
    Cliente cliente(contexto, util::variableEntorno("GESTOR_ENDPOINT", "tcp://localhost:5555"),
                    argv[1]);
    return ConsolaCliente(cliente).ejecutar();
}
