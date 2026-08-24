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

Espectro::Espectro(float taxa, int canaes) {
  // A janela de HANN, computada uma vez. Sem janela alguma (que é a janela
  // rectangular) um seno puro espalha a sua raia por toda a tela, e o aceite de
  // que «o seno de 440 acende a banda de 440 e não as outras» morreria por isso
  // e por nada mais.
  hann_.resize(JANELA_DA_FFT);
  for (std::size_t i = 0; i < JANELA_DA_FFT; ++i) {
    const float parte = static_cast<float>(i) / static_cast<float>(JANELA_DA_FFT - 1);
    hann_[i] = 0.5f - 0.5f * std::cos(2.0f * PI * parte);
  }

  entrada_ = fftwf_alloc_real(JANELA_DA_FFT);
  sahida_ = fftwf_alloc_real(2 * (JANELA_DA_FFT / 2 + 1));
  // O plano nasce UMA VEZ, na construcção, e nunca dentro do callback de
  // processo: planejar custa, e custar dentro da linha de tempo real do
  // PipeWire seria pagar em falha de audio o que aqui se paga uma só vez.
  plano_ = fftwf_plan_dft_r2c_1d(static_cast<int>(JANELA_DA_FFT), entrada_,
                                 reinterpret_cast<fftwf_complex*>(sahida_),
                                 FFTW_MEASURE);
  bandas_.assign(QUANTAS_BANDAS, 0.0f);
  assenta_formato(taxa, canaes);
}

Espectro::~Espectro() {
  if (plano_ != nullptr) fftwf_destroy_plan(plano_);
  if (entrada_ != nullptr) fftwf_free(entrada_);
  if (sahida_ != nullptr) fftwf_free(sahida_);
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
