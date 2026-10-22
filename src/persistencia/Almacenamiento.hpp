#pragma once

#include <string>
#include <vector>

#include "comun/Mensaje.hpp"

// Almacenamiento durable de la base de datos.
//
// Se guarda la bitácora de las reservas (ya con su "instante"); al arrancar se
// vuelven a aplicar en orden y se obtiene el mismo estado. Otra
// implementación (por ejemplo PostgreSQL) puede reemplazar a la de archivo.
class Almacenamiento {
public:
    virtual ~Almacenamiento() = default;

    // Operaciones guardadas, en el orden en que se aplicaron.
    virtual std::vector<Mensaje> cargar() = 0;

    // Guarda una operación antes de responderla; debe quedar en disco al volver.
    virtual void registrar(const Mensaje& operacion) = 0;
};
