// Proceso de la capa de Servicios de Almacenamiento: Servicio de Persistencia.
#include <memory>

#include "comun/Utilidades.hpp"
#include "persistencia/AlmacenamientoArchivo.hpp"
#include "persistencia/ServicioPersistencia.hpp"

int main() {
    const std::string endpoint = util::variableEntorno("PERSISTENCIA_BIND", "tcp://*:5560");
    const std::string archivo = util::variableEntorno("ARCHIVO_DATOS", "datos/bitacora.txt");

    ServicioPersistencia servicio(endpoint, std::make_unique<AlmacenamientoArchivo>(archivo));
    return servicio.ejecutar();
}
