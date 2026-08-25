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

// mm_ss — segundos em `MM:SS`, e o que não é tempo em `--:--`. Não lança, e não
// arredonda para cima: o operador que vê `02:05` ouviu dous minutos e cinco
// segundos, e nunca um segundo que ainda não passou.
std::string mm_ss(double segundos);

// enchimento — quantas collunhas da barra estão cheias. `round`, e não `floor`:
// com `floor` a barra fica uma collunha atrás do som por metade do tempo, e a
// ultima collunha só acende no fim exacto. Razão maior que um cinge-se em um, e
// duração que não é positiva ou não é finita dá zero, sem divisão alguma.
std::size_t enchimento(double posicao, double duracao, std::size_t largura);

// elemento_do_transporte — a barra inteira: os botões em fita arrowline, a barra
// de progresso enchida em v500 sobre inset, o tempo em MM:SS / MM:SS, e o
// volume. Largura zero dá elemento vazio, e nunca quadro roto.
ftxui::Element elemento_do_transporte(const Retracto& retracto,
                                      std::size_t largura);

}  // namespace mysong::tui

#endif  // MYSONG_TUI_TRANSPORTE_HPP

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
