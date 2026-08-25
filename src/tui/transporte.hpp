// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO TRANSPORTE — src/tui/transporte.hpp
// ══════════════════════════════════════════════════════════════════════════
// A barra de baixo do mockup: os botões, a barra de progresso, o tempo e o
// volume. Esta peça COMPÕE, e não conduz: não conhece Tocador, não conhece
// libmpv, não lê relogio. O que ella sabe do mundo chega n'um RETRACTO, que é
// uma estructura de valores que a bateria arma á mão sem motor e sem som.
//
// DOMÍNIO ......... um Retracto (estado, posição, duração, volume, titulo,
//                   indice e tamanho da fila) e uma largura em collunhas.
// CONTRA-DOMÍNIO .. cadeias e `ftxui::Element`, sempre os mesmos para o mesmo
//                   retracto: a composição é funcção, e não processo.
// INVARIANTE ...... a peça não guarda estado. Não ha campo que se lembre do
//                   quadro anterior, donde não ha estado em duplicata que possa
//                   divergir do nucleo, que é o defeito que o aceite proscreve.
// Q.E.D. .......... conhecendo a composição sómente o Retracto, a prova afere o
//                   que a tela mostra sem erguer tela, sem abrir motor e sem
//                   tocar som; e a montagem, que é o que não se prova, fica
//                   confinada a janella.cpp e é fina de proposito.
// ══════════════════════════════════════════════════════════════════════════
#ifndef MYSONG_TUI_TRANSPORTE_HPP
#define MYSONG_TUI_TRANSPORTE_HPP

#include <cstddef>
#include <string>

#include <ftxui/dom/elements.hpp>

#include "nucleo/motor.hpp"

namespace mysong::tui {

// O RETRACTO: o que a tela sabe do nucleo num instante. Cópia de valores, e
// nunca ponteiro para o tocador: assim a composição não pode, nem por descuido,
// perguntar duas vezes e pintar duas respostas differentes no mesmo quadro.
struct Retracto {
  nucleo::Estado estado = nucleo::Estado::Parado;
  double posicao = 0.0;
  double duracao = 0.0;
  int volume = 100;
  std::string titulo;
  std::size_t indice = 0;
  std::size_t tamanho = 0;
};

}  // namespace mysong::tui

#endif  // MYSONG_TUI_TRANSPORTE_HPP

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
