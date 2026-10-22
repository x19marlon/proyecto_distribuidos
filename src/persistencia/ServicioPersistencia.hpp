#pragma once

#include <map>
#include <memory>
#include <string>

#include "comun/Servicio.hpp"
#include "persistencia/Almacenamiento.hpp"
#include "persistencia/BaseDatos.hpp"

// Capa de Servicios de Almacenamiento: "Servicio de Persistencia" del diagrama.
//
// Socket REP: atiende una petición a la vez, lo que hace atómica cada operación.
// Las reservas son idempotentes por "claveIdempotencia" y se guardan en la
// bitácora antes de responder.
class ServicioPersistencia : public Servicio {
public:
    ServicioPersistencia(std::string endpoint, std::unique_ptr<Almacenamiento> almacenamiento);

protected:
    void inicializar() override;
    void iterar() override;

private:
    using Consulta = Mensaje (BaseDatos::*)(const Mensaje&) const;
    using Modificacion = Mensaje (BaseDatos::*)(const Mensaje&);

    Mensaje atender(Mensaje peticion);
    Mensaje aplicar(const Mensaje& operacion, Modificacion modificacion);

    std::string endpoint_;
    std::unique_ptr<Almacenamiento> almacenamiento_;
    BaseDatos base_;
    std::unique_ptr<zmq::socket_t> socket_;
    const std::map<std::string, Consulta> consultas_;
    const std::map<std::string, Modificacion> modificaciones_;
};
