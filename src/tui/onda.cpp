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
#include <string>
#include <string_view>

#include "tui/espectro.hpp"    // oitavos e glifo_do_degrau: a mesma escada
#include "tui/rato.hpp"        // caixa_por_pintar: caixa por pintar é vazia
#include "tui/tokens.hpp"
#include "tui/transporte.hpp"  // enchimento: o andado é o mesmo do trilho

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

namespace {

// A barra chata: o traço PESADO do trilho de hoje. O leve some no fundo
// violaceo a esta opacidade, e barra que se não vê não diz onde a faixa vae.
inline constexpr std::string_view kBarraChata = "\u2501";

}  // namespace

ftxui::Element elemento_da_onda(const std::vector<float>& pontos,
                                double posicao, double duracao,
                                std::size_t largura, bool com_foco,
                                ftxui::Box* caixa) {
  if (caixa != nullptr) *caixa = caixa_por_pintar();
  // `emptyElement`, e NÃO `text("")`: o `text` do FTXUI pede sempre UMA linha,
  // e a fita que pedisse meio de largura zero ganharia uma fileira do nada.
  if (largura == 0) return ftxui::emptyElement();
  const std::size_t andadas = enchimento(posicao, duracao, largura);
  const std::vector<float> valores = dobrar(pontos, largura);
  const tokens::Triade fundo = tokens::rgb(tokens::panel);
  std::vector<ftxui::Element> cellas;
  cellas.reserve(largura);
  for (std::size_t c = 0; c < largura; ++c) {
    // O foco accende o ANDADO, e não a linha inteira: accendendo tudo, a onda
    // deixaria de dizer por onde a faixa vae, que é o officio primeiro d'ella.
    const tokens::Triade tinta = tokens::rgb(
        c < andadas ? (com_foco ? tokens::glow_core : tokens::v600)
                    : tokens::line_dim);
    // Nunca menos de UM oitavo: sem esse piso o trecho calado sahiria em
    // cella vazia, e a onda pareceria cortada em vez de rasa.
    const std::string glifo =
        valores.empty() ? std::string(kBarraChata)
                        : glifo_do_degrau(std::max(1, oitavos(valores[c], 1)));
    cellas.push_back(
        ftxui::text(glifo) |
        ftxui::color(ftxui::Color::RGB(tinta.r, tinta.g, tinta.b)) |
        ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g, fundo.b)));
  }
  ftxui::Element linha = ftxui::hbox(std::move(cellas));
  return caixa == nullptr ? linha : linha | ftxui::reflect(*caixa);
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
