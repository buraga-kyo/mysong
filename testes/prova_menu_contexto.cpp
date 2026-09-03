// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO MENU DE CONTEXTO — testes/prova_menu_contexto.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada de dentro do menu (issue #96), sem terminal e sem tela: o estado
// arma-se á mão e afere-se o pedido que cada tecla devolve. Ella apanha o item
// trocado de logar, defeito que apagaria a faixa errada.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
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

TEST_CASE("as setas andam pelos cinco itens, e dão a volta") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  for (std::size_t passo = 1; passo < tui::QUANTOS_ITENS; ++passo) {
    CHECK(tui::tecla_no_menu(menu, ftxui::Event::ArrowDown).pedido ==
          tui::PedidoDoMenu::Nada);
    CHECK(menu.item == passo);
  }
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);  // do ultimo ao primeiro
  CHECK(menu.item == 0);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowUp);  // e do primeiro ao ultimo
  CHECK(menu.item == tui::QUANTOS_ITENS - 1);
  CHECK(menu.aberto);  // andar não fecha
}

TEST_CASE("a seta direita abre o submenu das listas e a esquerda fecha-o") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  tui::tecla_no_menu(menu, ftxui::Event::ArrowRight);
  CHECK_FALSE(menu.submenu);  // no TOCAR a direita não abre cousa alguma
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowRight);
  CHECK(menu.submenu);
  CHECK(menu.lista == 0);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
  CHECK(menu.lista == 1);
  CHECK(menu.item == 1);  // dentro do submenu a seta anda nas LISTAS
  tui::tecla_no_menu(menu, ftxui::Event::ArrowLeft);
  CHECK_FALSE(menu.submenu);
  CHECK(menu.aberto);  // fechar o submenu não fecha o menu
}

TEST_CASE("o Escape fecha o menu, e o botão a descer tambem") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  tui::tecla_no_menu(menu, clique(ftxui::Mouse::Left, ftxui::Mouse::Released));
  CHECK(menu.aberto);  // o soltar do proprio clique que o abriu não o fecha
  tui::tecla_no_menu(menu, clique(ftxui::Mouse::Left, ftxui::Mouse::Pressed));
  CHECK_FALSE(menu.aberto);
  menu = menu_de_pe(tres_listas());
  tui::tecla_no_menu(menu, ftxui::Event::Escape);
  CHECK_FALSE(menu.aberto);
  // E menu fechado consome tecla nenhuma: o pedido sahe Nada, e o estado fica.
  CHECK(tui::tecla_no_menu(menu, ftxui::Event::Return).pedido ==
        tui::PedidoDoMenu::Nada);
}
