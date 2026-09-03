// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO MENU DE CONTEXTO — testes/prova_menu_contexto.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada de dentro do menu (issue #96), sem terminal e sem tela: o estado
// arma-se á mão e afere-se o pedido que cada tecla devolve. Ella apanha o item
// trocado de logar, defeito que apagaria a faixa errada.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include "tui/menu_contexto.hpp"

namespace tui = mysong::tui;
namespace nucleo = mysong::nucleo;

namespace {

// As tres listas de mentira. Os ids vão SALTEADOS de proposito: menu que
// devolvesse o indice passaria por ids seguidos e juntaria á lista errada.
std::vector<nucleo::Rol> tres_listas() {
  return {{7, "funk", 3}, {12, "estudo", 0}, {40, "domingo", 9}};
}

tui::MenuDeContexto menu_de_pe(std::vector<nucleo::Rol> listas) {
  tui::MenuDeContexto menu;
  tui::abre_o_menu(menu, 4, "Montagem Lunar Celestia", std::move(listas));
  return menu;
}

ftxui::Event clique(ftxui::Mouse::Button botao, ftxui::Mouse::Motion gesto) {
  ftxui::Mouse rato;
  rato.button = botao;
  rato.motion = gesto;
  return ftxui::Event::Mouse("", rato);
}

}  // namespace

TEST_CASE("o menu abre na faixa alvo, no primeiro item e sem submenu") {
  const tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  CHECK(menu.aberto);
  CHECK(menu.faixa == 4);
  CHECK(menu.titulo == "Montagem Lunar Celestia");
  CHECK(menu.item == 0);
  CHECK_FALSE(menu.submenu);
}
