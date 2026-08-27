// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DOS FIOS — testes/prova_tocador_fios.cpp
// ══════════════════════════════════════════════════════════════════════════
// Bate no MESMO tocador de mais de um fio ao mesmo tempo, mil voltas cada,
// como a janella faz: o relogio n'um fio, as teclas n'outro. O oraculo de
// verdade é o sanitizador de fios, que o aceite da issue #50 corre sobre esta
// bateria; as asserções ficam para DEPOIS do join, que o doctest não assere
// de fio segundo.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <thread>

#include "nucleo/tocador.hpp"

namespace {

using mysong::nucleo::Estado;
using mysong::nucleo::Retracto;
using mysong::nucleo::Tocador;

// Mil voltas por fio, como o aceite pede.
constexpr int VOLTAS = 1000;

}  // namespace

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
