#include "servicios/gestor/GestorTransacciones.hpp"

#include <cerrno>
#include <vector>

#include "comun/Mensaje.hpp"
#include "comun/Utilidades.hpp"
#include "servicios/Protocolo.hpp"

GestorTransacciones::GestorTransacciones(std::string frontend, std::string backend)
    : Servicio("gestor-transacciones"),
      endpointFrontend_(std::move(frontend)),
      endpointBackend_(std::move(backend)) {}

void GestorTransacciones::inicializar() {
    frontend_ = std::make_unique<zmq::socket_t>(contexto(), zmq::socket_type::router);
    backend_ = std::make_unique<zmq::socket_t>(contexto(), zmq::socket_type::router);
    frontend_->set(zmq::sockopt::linger, 0);
    backend_->set(zmq::sockopt::linger, 0);
    // Si un procesador se cayó, enviarle falla en vez de perder el mensaje en silencio.
    backend_->set(zmq::sockopt::router_mandatory, 1);

    frontend_->bind(endpointFrontend_);
    backend_->bind(endpointBackend_);
    log("clientes en " + endpointFrontend_ + ", procesadores RMC en " + endpointBackend_);
}

void GestorTransacciones::iterar() {
    zmq::pollitem_t items[] = {{static_cast<void*>(*backend_), 0, ZMQ_POLLIN, 0},
                               {static_cast<void*>(*frontend_), 0, ZMQ_POLLIN, 0}};
    // Solo se reciben clientes cuando hay un procesador libre; mientras tanto esperan en
    // la cola de ZeroMQ.
    const int cantidad = procesadoresLibres_.empty() ? 1 : 2;
    zmq::poll(items, cantidad, std::chrono::milliseconds(250));

    if (items[0].revents & ZMQ_POLLIN) atenderProcesador();
    if (cantidad == 2 && (items[1].revents & ZMQ_POLLIN)) atenderCliente();
}

void GestorTransacciones::atenderCliente() {
    // [idCliente, "", petición]
    std::vector<zmq::message_t> partes;
    (void)zmq::recv_multipart(*frontend_, std::back_inserter(partes));
    if (partes.size() != 3) return;

    if (!enviarAProcesador(partes[0], partes[2])) {
        responderCliente(aTexto(partes[0]),
                         Mensaje::error("SIN_PROCESADORES", "No hay procesadores RMC disponibles")
                             .serializar());
    }
}

bool GestorTransacciones::enviarAProcesador(const zmq::message_t& idCliente,
                                            const zmq::message_t& peticion) {
    while (!procesadoresLibres_.empty()) {
        const std::string idProcesador = procesadoresLibres_.front();
        procesadoresLibres_.pop_front();
        enCola_.erase(idProcesador);

        auto contacto = ultimoContacto_.find(idProcesador);
        if (contacto == ultimoContacto_.end() ||
            util::relojMs() - contacto->second > protocolo::kVidaProcesadorMs) {
            log("procesador sin latidos, se descarta");
            ultimoContacto_.erase(idProcesador);
            continue;
        }
        try {
            // [idProcesador, "", idCliente, "", petición]
            backend_->send(zmq::buffer(idProcesador), zmq::send_flags::sndmore);
            backend_->send(zmq::const_buffer(), zmq::send_flags::sndmore);
            backend_->send(zmq::buffer(idCliente.data(), idCliente.size()),
                           zmq::send_flags::sndmore);
            backend_->send(zmq::const_buffer(), zmq::send_flags::sndmore);
            backend_->send(zmq::buffer(peticion.data(), peticion.size()), zmq::send_flags::none);
            ocupados_.insert(idProcesador);
            return true;
        } catch (const zmq::error_t& e) {
            if (e.num() != EHOSTUNREACH) throw;
            log("procesador desconectado, se descarta");  // se intenta con el siguiente
            ultimoContacto_.erase(idProcesador);
        }
    }
    return false;
}

void GestorTransacciones::marcarLibre(const std::string& idProcesador) {
    ocupados_.erase(idProcesador);
    if (enCola_.insert(idProcesador).second) procesadoresLibres_.push_back(idProcesador);
}

void GestorTransacciones::atenderProcesador() {
    // [idProcesador, "", LISTO|LATIDO] o [idProcesador, "", idCliente, "", respuesta]
    std::vector<zmq::message_t> partes;
    (void)zmq::recv_multipart(*backend_, std::back_inserter(partes));
    if (partes.size() < 3) return;

    const std::string idProcesador = aTexto(partes[0]);
    const bool conocido = ultimoContacto_.count(idProcesador) > 0;
    ultimoContacto_[idProcesador] = util::relojMs();

    if (partes.size() == 3) {
        const std::string control = aTexto(partes[2]);
        // Un latido de un procesador ocupado llegó antes que su petición: se ignora.
        if (control == protocolo::kListo ||
            (control == protocolo::kLatido && ocupados_.count(idProcesador) == 0)) {
            marcarLibre(idProcesador);
            if (!conocido) {
                log("procesador RMC registrado (libres: " +
                    std::to_string(procesadoresLibres_.size()) + ")");
            }
        }
        return;
    }
    if (partes.size() != 5) return;
    marcarLibre(idProcesador);
    responderCliente(aTexto(partes[2]), aTexto(partes[4]));
}

void GestorTransacciones::responderCliente(const std::string& idCliente,
                                           const std::string& respuesta) {
    frontend_->send(zmq::buffer(idCliente), zmq::send_flags::sndmore);
    frontend_->send(zmq::const_buffer(), zmq::send_flags::sndmore);
    frontend_->send(zmq::buffer(respuesta), zmq::send_flags::none);
}
