// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO ESTALEIRO — testes/prova_estaleiro.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. A OBRA entra por parametro, e no logar
// d'ella põe-se aqui uma que se deixa SEGURAR: os casos param a obra a meio,
// olham o estaleiro, e sómente então a soltam. Donde o limite se afere por
// construcção, e não por relogio: prova do genero «esperei um segundo e não
// passou de dous» é prova que a machina carregada perde.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

#include "nucleo/estaleiro.hpp"

namespace nu = mysong::nucleo;

namespace {
}  // namespace

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
