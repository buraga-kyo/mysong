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

// O PASSO DA BUSCA, em segundos. Cinco, que é o passo que o mpv usa nas suas
// proprias setas: quem vem do mpv não reaprende o dedo.
inline constexpr double PASSO_DA_BUSCA = 5.0;

// O DEGRAU DO VOLUME, em pontos percentuaes.
inline constexpr int DEGRAU_DO_VOLUME = 5;

enum class Verbo { Nada, Pausar, Retomar, Proxima, Anterior, Buscar, Volume, Sahir };

// Uma ORDEM. `alvo` sómente presta para Buscar (segundos) e Volume (por cento),
// e nos outros verbos vale zero de proposito: ordem que não tem alvo não deve
// carregar numero que alguem possa vir a ler.
struct Ordem {
  Verbo verbo = Verbo::Nada;
  double alvo = 0.0;
};

// ordem_da_tecla — a taboada. Não toca no tocador: devolve o que se HA DE fazer.
Ordem ordem_da_tecla(const ftxui::Event& tecla, const Retracto& retracto);

}  // namespace mysong::tui

#endif  // MYSONG_TUI_COMMANDO_HPP

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
