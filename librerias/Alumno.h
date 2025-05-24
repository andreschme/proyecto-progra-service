//
// Created by Andres on 21/05/25.
//

#ifndef ALUMNO_H
#define ALUMNO_H

#include <string>
#include "json.hpp"

using namespace std;
using json = nlohmann::ordered_json;

struct Alumno {
    int id;
    string nombre;
    int edad;
    string genero;
    string grado;

    // Amigas para la serialización/deserialización con nlohmann/json
    friend void to_json(json& j, const Alumno& v) {
        j = json{
                    {"id", v.id},
                    {"nombre", v.nombre},
                    {"edad", v.edad},
                    {"genero", v.genero},
                    {"grado", v.grado}
        };
    }

    friend void from_json(const json& j, Alumno& v) {
        // El ID se asigna al crear o se lee del archivo,
        // por lo que solo lo leemos si está presente (útil para cargar desde archivo)
        if (j.contains("id")) {
            j.at("id").get_to(v.id);
        }

        // Para los campos que vienen del cliente, es bueno verificar si existen
        // antes de intentar accederlos para evitar excepciones si faltan.
        if (j.contains("nombre")) j.at("nombre").get_to(v.nombre);
        if (j.contains("edad")) j.at("edad").get_to(v.edad);
        if (j.contains("genero")) j.at("genero").get_to(v.genero);
        if (j.contains("grado")) j.at("grado").get_to(v.grado);
    }
};

#endif //ALUMNO_H
#pragma once