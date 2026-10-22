# Compila los 4 procesos del sistema en bin/.
#
#   make                      compila todo (requiere libzmq3-dev y cppzmq-dev)
#   make ZMQ_PREFIX=/ruta     usa una instalación de ZeroMQ en otra carpeta
#   make limpiar              borra build/ y bin/

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
ZMQ_PREFIX ?= /usr

CPPFLAGS += -Isrc -I$(ZMQ_PREFIX)/include -MMD -MP
LDFLAGS += -L$(ZMQ_PREFIX)/lib
ifneq ($(ZMQ_PREFIX),/usr)
LDFLAGS += -Wl,-rpath,$(abspath $(ZMQ_PREFIX))/lib
endif
LDLIBS += -lzmq -pthread

COMUN := $(wildcard src/comun/*.cpp)

FUENTES_cliente := $(wildcard src/consumidores/*.cpp) $(COMUN)
FUENTES_gestor-transacciones := $(wildcard src/servicios/gestor/*.cpp) $(COMUN)
FUENTES_procesador-rmc := $(wildcard src/servicios/rmc/*.cpp) $(COMUN)
FUENTES_persistencia := $(wildcard src/persistencia/*.cpp) $(COMUN)

PROGRAMAS := cliente gestor-transacciones procesador-rmc persistencia
BINARIOS := $(addprefix bin/,$(PROGRAMAS))

objetos = $(patsubst src/%.cpp,build/%.o,$(FUENTES_$(1)))

.PHONY: all limpiar
all: $(BINARIOS)

.SECONDEXPANSION:
$(BINARIOS): bin/%: $$(call objetos,%)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

limpiar:
	rm -rf build bin

-include $(shell find build -name '*.d' 2>/dev/null)
