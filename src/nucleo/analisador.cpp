// ══════════════════════════════════════════════════════════════════════════
//   A CARNE DO ANALISADOR — src/nucleo/analisador.cpp
// ══════════════════════════════════════════════════════════════════════════
// O TRACTADO vive no cabeçalho. Aqui mora o PipeWire, e sómente aqui: é esta a
// unica unidade de traducção da Casa que inclue pipewire.h.
//
// Duas linhas de execução se cruzam n'este arquivo, e é bom sabê-lo antes de o
// ler: o callback de processo corre na linha de TEMPO REAL do PipeWire, e
// bandas() com pulsa() correm na linha de quem chama. A fechadura guarda o
// espectro com o retracto, e nada mais: quem a tomasse para mais tempo pagaria
// em falha de audio.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/analisador.hpp"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <set>

#include "nucleo/espectro.hpp"

namespace mysong::nucleo {
namespace {
using Relogio = std::chrono::steady_clock;

// Quantos millesimos se passaram entre dous instantes.
double millesimos_entre(Relogio::time_point antes, Relogio::time_point depois) {
  return std::chrono::duration<double, std::milli>(depois - antes).count();
}

}  // namespace

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
