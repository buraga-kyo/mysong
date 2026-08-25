// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COMMANDO — src/tui/commando.hpp
// ══════════════════════════════════════════════════════════════════════════
// A tradução de TECLA em ORDEM. Existe á parte da janella porque a janella não
// se prova: ella abre terminal, abre motor e abre som. Esta peça é funcção pura
// de tecla e retracto para uma ordem, e a bateria afere a taboada inteira sem
// erguer cousa alguma.
//
// DOMÍNIO ......... a tecla que o terminal entrega, e o Retracto do instante.
// CONTRA-DOMÍNIO .. uma Ordem: o verbo e, quando o verbo o pede, o alvo já
//                   APARADO nas bordas.
// INVARIANTE ...... tecla que não é do mockup dá Ordem::Nada, e Ordem::Nada não
//                   chama cousa alguma no tocador. A aparadura é feita AQUI, e
//                   não no tocador: o tocador tambem apara, e ter as duas dá
//                   duas verdades; esta é a que a tela mostra.
// Q.E.D. .......... sendo a taboada uma funcção, a prova conta as chamadas por
//                   dublê e não por terminal, e a issue seguinte que acrescente
//                   tecla acrescenta uma linha da taboada e um caso.
// ══════════════════════════════════════════════════════════════════════════
#ifndef MYSONG_TUI_COMMANDO_HPP
#define MYSONG_TUI_COMMANDO_HPP

#include <ftxui/component/event.hpp>

#include "tui/transporte.hpp"

namespace mysong::tui {

// ordem_da_tecla — a taboada. Não toca no tocador: devolve o que se HA DE fazer.
Ordem ordem_da_tecla(const ftxui::Event& tecla, const Retracto& retracto);

}  // namespace mysong::tui

#endif  // MYSONG_TUI_COMMANDO_HPP

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
