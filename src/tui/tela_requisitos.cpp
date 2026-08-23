// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TELA DOS REQUISITOS — src/tui/tela_requisitos.cpp
// ══════════════════════════════════════════════════════════════════════════
// A obra da tela. Nada se sonda aqui: recebe-se o relatorio já colhido e
// mostra-se, ou em texto puro, ou em quadro pintado.
//
// DOMÍNIO ......... um relatorio da sonda.
// CONTRA-DOMÍNIO .. cadeia de texto sem escape algum, ou elemento do FTXUI.
// INVARIANTE ...... o texto puro é PURO: nenhuma sequencia de escape, nenhuma
//                   tela alternativa, nada que suje o tubo de quem o rediriga.
// Q.E.D. .......... o texto e o quadro sahem do mesmo relatorio e da mesma
//                   frase de limite; donde não podem contar historias
//                   differentes ao mesmo operador.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/tela_requisitos.hpp"

#include <string>

namespace mysong::tui {

namespace {

// O rotulo da gravidade, guarnecido á mesma largura, para que a columna do
// estado se alinhe sem se calcular largura nenhuma.
std::string_view rotulo_da_gravidade(nucleo::Gravidade gravidade) {
  return gravidade == nucleo::Gravidade::Impedimento ? "impedimento"
                                                     : "aviso      ";
}

}  // namespace

}  // namespace mysong::tui

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
