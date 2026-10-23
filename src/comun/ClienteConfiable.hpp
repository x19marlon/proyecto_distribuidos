#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "comun/Mensaje.hpp"
#include "comun/Zmq.hpp"

// Cliente REQ con el patrón Lazy Pirate de ZeroMQ: espera la respuesta hasta un
// timeout y, si no llega, cierra el socket, abre otro y reenvía la misma petición.
// Como la petición se reenvía igual (mismo idSolicitud), el servidor debe ser idempotente.
class ClienteConfiable {
public:
    ClienteConfiable(zmq::context_t& contexto, std::string endpoint,
                     std::chrono::milliseconds timeout, int intentos);

    // Devuelve la respuesta, o nada si se agotaron los intentos.
    std::optional<Mensaje> solicitar(const Mensaje& peticion);

    const std::string& endpoint() const { return endpoint_; }

private:
    void abrirSocket();

    zmq::context_t& contexto_;
    std::string endpoint_;
    std::chrono::milliseconds timeout_;
    int intentos_;
    std::unique_ptr<zmq::socket_t> socket_;
};
