#include "servicios/rmc/ProcesadorRMC.hpp"

#include <regex>
#include <vector>

#include "comun/Utilidades.hpp"
#include "servicios/Protocolo.hpp"

namespace {

Mensaje faltaCampo(const std::string& campo) {
    return Mensaje::error("DATOS_INVALIDOS", "Falta el campo " + campo);
}

}  // namespace

ProcesadorRMC::ProcesadorRMC(std::string endpointGestor, std::string endpointPersistencia)
    : Servicio("procesador-rmc"),
      endpointGestor_(std::move(endpointGestor)),
      endpointPersistencia_(std::move(endpointPersistencia)),
      operaciones_{{"consultarEventosMes", &ProcesadorRMC::consultarEventosMes},
                   {"reservar", &ProcesadorRMC::reservar}} {}

void ProcesadorRMC::inicializar() {
    persistencia_ = std::make_unique<ClienteConfiable>(contexto(), endpointPersistencia_,
                                                       std::chrono::milliseconds(300), 3);

    gestor_ = std::make_unique<zmq::socket_t>(contexto(), zmq::socket_type::dealer);
    gestor_->set(zmq::sockopt::linger, 0);
    // Identidad fija: si el gestor se reinicia, reconoce al procesador por sus latidos.
    gestor_->set(zmq::sockopt::routing_id, util::generarId("rmc"));
    gestor_->connect(endpointGestor_);
    enviarControl(protocolo::kListo);
    log("conectado al gestor en " + endpointGestor_ + ", persistencia en " +
        endpointPersistencia_);
}

void ProcesadorRMC::enviarControl(const char* control) {
    gestor_->send(zmq::const_buffer(), zmq::send_flags::sndmore);
    gestor_->send(zmq::buffer(std::string(control)), zmq::send_flags::none);
    ultimoLatido_ = util::relojMs();
}

void ProcesadorRMC::iterar() {
    zmq::pollitem_t items[] = {{static_cast<void*>(*gestor_), 0, ZMQ_POLLIN, 0}};
    zmq::poll(items, 1, std::chrono::milliseconds(protocolo::kIntervaloLatidoMs / 2));

    if (items[0].revents & ZMQ_POLLIN) {
        // ["", idCliente, "", petición]
        std::vector<zmq::message_t> partes;
        (void)zmq::recv_multipart(*gestor_, std::back_inserter(partes));
        if (partes.size() == 4) {
            const Mensaje respuesta = procesar(Mensaje::deserializar(aTexto(partes[3])));
            gestor_->send(zmq::const_buffer(), zmq::send_flags::sndmore);
            gestor_->send(partes[1], zmq::send_flags::sndmore);
            gestor_->send(zmq::const_buffer(), zmq::send_flags::sndmore);
            gestor_->send(zmq::buffer(respuesta.serializar()), zmq::send_flags::none);
            ultimoLatido_ = util::relojMs();  // la respuesta también cuenta como latido
        }
    }
    if (util::relojMs() - ultimoLatido_ >= protocolo::kIntervaloLatidoMs) {
        enviarControl(protocolo::kLatido);
    }
}

Mensaje ProcesadorRMC::procesar(const Mensaje& peticion) {
    auto it = operaciones_.find(peticion.tipo());
    if (it == operaciones_.end()) {
        return Mensaje::error("OPERACION_DESCONOCIDA", "Operación desconocida: " + peticion.tipo());
    }
    if (!peticion.tiene("idEstudiante")) return faltaCampo("idEstudiante");
    return (this->*(it->second))(peticion);
}

Mensaje ProcesadorRMC::llamarPersistencia(const Mensaje& peticion) {
    std::optional<Mensaje> respuesta = persistencia_->solicitar(peticion);
    if (!respuesta) {
        log("persistencia no responde");
        return Mensaje::error("PERSISTENCIA_NO_DISPONIBLE",
                              "La capa de persistencia no respondió; intenta de nuevo");
    }
    return *respuesta;
}

Mensaje ProcesadorRMC::consultarEventosMes(const Mensaje& peticion) {
    static const std::regex formatoMes(R"(\d{4}-(0[1-9]|1[0-2]))");
    const std::string mes = peticion.texto("mes");
    if (!std::regex_match(mes, formatoMes)) {
        return Mensaje::error("DATOS_INVALIDOS", "El mes debe tener el formato AAAA-MM");
    }
    return llamarPersistencia(Mensaje("listarFechasMes").poner("mes", mes));
}

Mensaje ProcesadorRMC::reservar(const Mensaje& peticion) {
    if (!peticion.tiene("idSolicitud")) return faltaCampo("idSolicitud");
    if (!peticion.tiene("idFecha")) return faltaCampo("idFecha");
    if (peticion.entero("cupos") <= 0) {
        return Mensaje::error("DATOS_INVALIDOS", "Los cupos deben ser un número mayor a 0");
    }

    // El idSolicitud es la clave de idempotencia: un reintento no reserva dos veces.
    return llamarPersistencia(Mensaje("crearReserva")
                                  .poner("claveIdempotencia", peticion.texto("idSolicitud"))
                                  .poner("idSolicitud", peticion.texto("idSolicitud"))
                                  .poner("idEstudiante", peticion.texto("idEstudiante"))
                                  .poner("idFecha", peticion.texto("idFecha"))
                                  .poner("cupos", peticion.entero("cupos")));
}
