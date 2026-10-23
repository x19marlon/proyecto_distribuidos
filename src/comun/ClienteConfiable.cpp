#include "comun/ClienteConfiable.hpp"

ClienteConfiable::ClienteConfiable(zmq::context_t& contexto, std::string endpoint,
                                   std::chrono::milliseconds timeout, int intentos)
    : contexto_(contexto), endpoint_(std::move(endpoint)), timeout_(timeout), intentos_(intentos) {
    abrirSocket();
}

void ClienteConfiable::abrirSocket() {
    socket_ = std::make_unique<zmq::socket_t>(contexto_, zmq::socket_type::req);
    socket_->set(zmq::sockopt::linger, 0);
    socket_->connect(endpoint_);
}

std::optional<Mensaje> ClienteConfiable::solicitar(const Mensaje& peticion) {
    const std::string datos = peticion.serializar();
    for (int intento = 1; intento <= intentos_; ++intento) {
        socket_->send(zmq::buffer(datos), zmq::send_flags::none);

        zmq::pollitem_t items[] = {{static_cast<void*>(*socket_), 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, timeout_);
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t respuesta;
            (void)socket_->recv(respuesta, zmq::recv_flags::none);
            return Mensaje::deserializar(aTexto(respuesta));
        }
        // Sin respuesta: un socket REQ queda bloqueado esperando, así que se descarta.
        abrirSocket();
    }
    return std::nullopt;
}
