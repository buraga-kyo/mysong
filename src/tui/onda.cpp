// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA ONDA NA TELA — src/tui/onda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação: a dobra primeiro, a pintura depois.
//
// DOMÍNIO ......... os pontos em [0,1] e a geometria da fita.
// CONTRA-DOMÍNIO .. a linha de cellas, sempre da largura pedida.
// INVARIANTE ...... a dobra é a MESMA repartição do espectro: onda e barras
//                   hão de fundir egual n'esta Casa.
// Q.E.D. .......... funcção alguma d'aqui lê relogio, disco nem ambiente.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/onda.hpp"

#include <algorithm>

namespace mysong::tui {

std::vector<float> dobrar(const std::vector<float>& pontos,
                          std::size_t largura) {
  std::vector<float> dobrados;
  if (pontos.empty() || largura == 0) return dobrados;
  dobrados.reserve(largura);
  const std::size_t quantos = pontos.size();
  for (std::size_t c = 0; c < largura; ++c) {
    // O intervallo SEMI-ABERTO [c*n/largura, (c+1)*n/largura), alargado ao
    // minimo de um ponto. Partem [0,n) sem sobra e sem vão, d'onde ponto
    // algum se perde; e sendo a largura maior que n, o intervallo sahiria
    // vazio por truncamento, e o alargamento fal-o esticar em vez de furar.
    std::size_t principio = (c * quantos) / largura;
    if (principio >= quantos) principio = quantos - 1;
    std::size_t fim = ((c + 1) * quantos) / largura;
    if (fim <= principio) fim = principio + 1;
    if (fim > quantos) fim = quantos;
    float pico = 0.0f;
    for (std::size_t p = principio; p < fim; ++p) {
      // Cinge-se aqui: a peça da tela ha de aguentar pontos que outrem lhe
      // dê, e valor fóra de [0,1] daria bloco fóra da escada dos oito.
      pico = std::max(pico, pontos[p] < 0.0f
                                ? 0.0f
                                : (pontos[p] > 1.0f ? 1.0f : pontos[p]));
    }
    dobrados.push_back(pico);
  }
  return dobrados;
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
