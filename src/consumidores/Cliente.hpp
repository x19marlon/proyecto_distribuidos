#pragma once

#include <string>

#include "comun/ClienteConfiable.hpp"
#include "comun/Mensaje.hpp"

// Cliente de la Capa de Consumidores: envía las solicitudes al Gestor de
// Transacciones (REQ con Lazy Pirate).
//
// Cada reserva lleva un idSolicitud nuevo; los reintentos reenvían el mismo, así
// que nunca se reserva dos veces.
class Cliente {
public:
    Cliente(zmq::context_t& contexto, const std::string& endpointGestor, std::string idEstudiante);

    const std::string& idEstudiante() const { return idEstudiante_; }

    Mensaje consultarEventosMes(const std::string& mes);
    Mensaje reservar(const std::string& idFecha, int cupos);

private:
    Mensaje enviar(Mensaje peticion);

    ClienteConfiable gestor_;
    std::string idEstudiante_;
};
