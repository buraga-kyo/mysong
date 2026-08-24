// ══════════════════════════════════════════════════════════════════════════
//   A CARNE DO ESPECTRO — src/nucleo/espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// O TRACTADO vive no cabeçalho. Aqui mora a fftw3, e sómente aqui: é esta a
// unica unidade de traducção da Casa que inclue fftw3.h.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/espectro.hpp"

#include <fftw3.h>

#include <algorithm>
#include <cmath>

namespace mysong::nucleo {
namespace {

constexpr float PI = 3.14159265358979323846f;

// O coefficiente de um filtro de primeira ordem, tirado do TEMPO e do passo.
// Passo maior que o tempo dá coefficiente um, que é chegar de uma vez: é o que
// se quer, e não um coefficiente maior que um a estourar o valor por cima.
float coeficiente(double tempo_ms, double passo_ms) {
  if (tempo_ms <= 0.0 || passo_ms <= 0.0) return 1.0f;
  const double crua = 1.0 - std::exp(-passo_ms / tempo_ms);
  return static_cast<float>(std::min(1.0, std::max(0.0, crua)));
}

// A compressão em decibeis, aparada ao piso. Magnitude não positiva vale zero,
// e a comparação por maior-que trata o NaN de graça: NaN não é maior que zero,
// donde NaN nunca sahe d'esta função, e a barra nunca recebe o que não pinta.
float em_decibeis(float magnitude) {
  if (!(magnitude > 0.0f)) return 0.0f;
  const float db = 20.0f * std::log10(magnitude);
  if (db <= PISO_EM_DECIBEIS) return 0.0f;
  if (db >= 0.0f) return 1.0f;
  return (db - PISO_EM_DECIBEIS) / (0.0f - PISO_EM_DECIBEIS);
}

}  // namespace

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
