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

#include "tui/tokens.hpp"

namespace mysong::tui {

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

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
