// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO TRANSPORTE — src/tui/transporte.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação da composição. Vale aqui a mesma regra do cabeçalho: cousa
// alguma d'este arquivo sabe o que é um Tocador. Se um dia souber, a prova do
// criterio C4 accusa, que ella busca os nomes proibidos por grep.
//
// DOMÍNIO ......... o Retracto e a largura.
// CONTRA-DOMÍNIO .. cadeias e elementos, deterministicos.
// INVARIANTE ...... funcção alguma d'aqui lança, e nenhuma divide sem antes
//                   provar o divisor: tempo de faixa vem do mpv, e o mpv
//                   entrega zero e não-numero antes de a faixa carregar.
// Q.E.D. .......... sendo tudo funcção de valores, a bateria afere o quadro
//                   contra alvo escripto á mão.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/transporte.hpp"

#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

#include "tui/arrowline.hpp"
#include "tui/tokens.hpp"

namespace mysong::tui {

// Os dous glifos da barra. Bloco cheio e bloco leve, e não espaço para o vazio:
// espaço com fundo pintado depende de o terminal honrar o fundo até á borda da
// célulla, e ha emulador que o não faz; glifo desenhado sahe sempre.
inline constexpr std::string_view kBarraCheia = "\u2588";
inline constexpr std::string_view kBarraVazia = "\u2591";


std::string mm_ss(double segundos) {
  // Tempo que não é tempo mostra-se como tal, e não como `00:00`: zero é uma
  // affirmação (a faixa está no principio), e o traço é a confissão de que a
  // Casa ainda não sabe. Confundir os dous faria a tela mentir no arranque.
  if (!std::isfinite(segundos) || segundos < 0.0) return "--:--";
  const long inteiro = static_cast<long>(segundos);
  const long minutos = inteiro / 60;
  const long resto = inteiro % 60;
  char molde[32];
  std::snprintf(molde, sizeof molde, "%02ld:%02ld", minutos, resto);
  return std::string(molde);
}

std::size_t enchimento(double posicao, double duracao, std::size_t largura) {
  if (largura == 0) return 0;
  // Duração que não presta dá barra vazia, e a guarda vem ANTES da divisão: o
  // mpv entrega duração zero enquanto a faixa carrega, e dividir alli daria
  // infinito, que o `round` converteria em numero qualquer.
  if (!std::isfinite(duracao) || duracao <= 0.0) return 0;
  if (!std::isfinite(posicao) || posicao <= 0.0) return 0;
  double razao = posicao / duracao;
  if (razao > 1.0) razao = 1.0;  // buscou-se para o fim, ou o mpv passou d'elle
  const double collunhas = std::round(razao * static_cast<double>(largura));
  const std::size_t cheias = static_cast<std::size_t>(collunhas);
  return cheias > largura ? largura : cheias;
}

namespace {

// Os pedaços da fita em elementos. A regra do DESIGN_SYSTEM manda que a côr da
// seta seja a côr do segmento que ella SEGUE, e a fita já a resolveu: aqui
// sómente se pinta o que ella diz.
ftxui::Element fita_em_elemento(const std::vector<Pedaco>& pedacos) {
  std::vector<ftxui::Element> partes;
  partes.reserve(pedacos.size());
  for (const Pedaco& pedaco : pedacos) {
    const tokens::Triade frente = tokens::rgb(pedaco.tinta);
    const tokens::Triade tras = tokens::rgb(pedaco.fundo);
    partes.push_back(ftxui::text(pedaco.texto) |
                     ftxui::color(ftxui::Color::RGB(frente.r, frente.g, frente.b)) |
                     ftxui::bgcolor(ftxui::Color::RGB(tras.r, tras.g, tras.b)));
  }
  return ftxui::hbox(std::move(partes));
}

}  // namespace

std::string_view nome_do_estado(nucleo::Estado estado) {
  switch (estado) {
    case nucleo::Estado::Tocando: return "Tocando";
    case nucleo::Estado::Pausado: return "Pausado";
    case nucleo::Estado::Parado: return "Parado";
  }
  return "Parado";  // o compilador não o sabe, mas o enum é fechado
}

// A fita dos botões e do estado. Sentido DEXTRA sómente: misturar os dous
// lavraria o losango que a regra proscreve, e a Fita já o torna inexprimivel.
// Os fundos descem pela rampa, do acento cardeal ao fundo do painel, que é a
// leitura da esquerda para a direita.
ftxui::Element fita_dos_botoes(const Retracto& retracto) {
  const bool tocando = retracto.estado == nucleo::Estado::Tocando;
  Fita fita(Sentido::Dextra);
  fita.junta({" " + std::string(tocando ? "\u23f8" : "\u25b6") + " ",
              tokens::v500, tokens::base});
  fita.junta({" \u23ee \u23ed ", tokens::v700, tokens::text_bright});
  fita.junta({" " + std::string(nome_do_estado(retracto.estado)) + " ",
              tokens::v900, tokens::text_bright});
  return fita_em_elemento(fita.compor());
}

std::string linha_da_barra(const Retracto& retracto, std::size_t largura) {
  const std::size_t cheias = enchimento(retracto.posicao, retracto.duracao, largura);
  std::string linha;
  linha.reserve(largura * 3);
  for (std::size_t c = 0; c < largura; ++c)
    linha += (c < cheias) ? kBarraCheia : kBarraVazia;
  return linha;
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
