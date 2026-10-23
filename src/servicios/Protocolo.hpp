#pragma once

// Constantes del protocolo entre el Gestor de Transacciones y los procesadores RMC.
namespace protocolo {

constexpr const char* kListo = "LISTO";    // el procesador arranca y queda disponible
constexpr const char* kLatido = "LATIDO";  // el procesador sigue vivo y libre
constexpr long long kIntervaloLatidoMs = 1000;
constexpr long long kVidaProcesadorMs = 3 * kIntervaloLatidoMs;  // 3 latidos perdidos = caído

}  // namespace protocolo
