#pragma once

#include <string>

#include <zmq.hpp>
#include <zmq_addon.hpp>

// Copia el contenido de una trama ZeroMQ a un std::string.
inline std::string aTexto(const zmq::message_t& trama) {
    return std::string(static_cast<const char*>(trama.data()), trama.size());
}
