#include "persistencia/ServicioPersistencia.hpp"

#include "comun/Utilidades.hpp"

ServicioPersistencia::ServicioPersistencia(std::string endpoint,
                                           std::unique_ptr<Almacenamiento> almacenamiento)
    : Servicio("persistencia"),
      endpoint_(std::move(endpoint)),
      almacenamiento_(std::move(almacenamiento)),
      consultas_{{"listarFechasMes", &BaseDatos::listarFechasMes}},
      modificaciones_{{"crearReserva", &BaseDatos::crearReserva}} {}

void ServicioPersistencia::inicializar() {
    base_.cargarDatosEjemplo();

    // Se reconstruye el estado volviendo a aplicar la bitácora.
    const std::vector<Mensaje> operaciones = almacenamiento_->cargar();
    for (const Mensaje& operacion : operaciones) {
        auto it = modificaciones_.find(operacion.tipo());
        if (it != modificaciones_.end()) aplicar(operacion, it->second);
    }
    log("bitácora cargada: " + std::to_string(operaciones.size()) + " operaciones");

    socket_ = std::make_unique<zmq::socket_t>(contexto(), zmq::socket_type::rep);
    socket_->set(zmq::sockopt::linger, 0);
    socket_->bind(endpoint_);
    log("escuchando en " + endpoint_);
}

void ServicioPersistencia::iterar() {
    zmq::pollitem_t items[] = {{static_cast<void*>(*socket_), 0, ZMQ_POLLIN, 0}};
    zmq::poll(items, 1, std::chrono::milliseconds(250));
    if (!(items[0].revents & ZMQ_POLLIN)) return;

    zmq::message_t trama;
    (void)socket_->recv(trama, zmq::recv_flags::none);
    const Mensaje respuesta = atender(Mensaje::deserializar(aTexto(trama)));
    socket_->send(zmq::buffer(respuesta.serializar()), zmq::send_flags::none);
}

Mensaje ServicioPersistencia::atender(Mensaje peticion) {
    if (auto consulta = consultas_.find(peticion.tipo()); consulta != consultas_.end()) {
        return (base_.*(consulta->second))(peticion);
    }
    auto modificacion = modificaciones_.find(peticion.tipo());
    if (modificacion == modificaciones_.end()) {
        return Mensaje::error("OPERACION_DESCONOCIDA", "Operación desconocida: " + peticion.tipo());
    }

    // Reintento de una petición ya aplicada: se devuelve la misma respuesta sin repetirla.
    if (peticion.tiene("claveIdempotencia")) {
        const std::string clave = peticion.tipo() + ":" + peticion.texto("claveIdempotencia");
        if (const Mensaje* previa = base_.respuestaPrevia(clave)) return *previa;
    }

    peticion.poner("instante", util::ahoraMs());
    const Mensaje respuesta = aplicar(peticion, modificacion->second);
    almacenamiento_->registrar(peticion);
    return respuesta;
}

Mensaje ServicioPersistencia::aplicar(const Mensaje& operacion, Modificacion modificacion) {
    const Mensaje respuesta = (base_.*modificacion)(operacion);
    if (operacion.tiene("claveIdempotencia")) {
        base_.guardarRespuesta(operacion.tipo() + ":" + operacion.texto("claveIdempotencia"),
                               respuesta);
    }
    return respuesta;
}
