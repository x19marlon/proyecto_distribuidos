#pragma once

#include <map>
#include <memory>
#include <string>

#include "comun/ClienteConfiable.hpp"
#include "comun/Mensaje.hpp"
#include "comun/Servicio.hpp"

// Procesamiento de reservas y consultas (caja "RMC" de la Figura 1).
//
// Recibe peticiones síncronas del Gestor de Transacciones, valida los datos y
// llama al Servicio de Persistencia (Lazy Pirate). Se pueden levantar varias
// instancias; el gestor reparte la carga entre las que estén libres.
class ProcesadorRMC : public Servicio {
public:
    ProcesadorRMC(std::string endpointGestor, std::string endpointPersistencia);

protected:
    void inicializar() override;
    void iterar() override;

private:
    using Operacion = Mensaje (ProcesadorRMC::*)(const Mensaje&);

    void enviarControl(const char* control);
    Mensaje procesar(const Mensaje& peticion);

    Mensaje consultarEventosMes(const Mensaje& peticion);
    Mensaje reservar(const Mensaje& peticion);

    // Envía a persistencia; si no responde, devuelve el error PERSISTENCIA_NO_DISPONIBLE.
    Mensaje llamarPersistencia(const Mensaje& peticion);

    std::string endpointGestor_;
    std::string endpointPersistencia_;
    std::unique_ptr<zmq::socket_t> gestor_;
    std::unique_ptr<ClienteConfiable> persistencia_;
    long long ultimoLatido_ = 0;
    const std::map<std::string, Operacion> operaciones_;
};
