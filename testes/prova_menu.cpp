// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO MENU — testes/prova_menu.cpp
// ══════════════════════════════════════════════════════════════════════════
// A machina de foco da barra (issue #80), inteira e sem terminal: as taboadas
// de tecla e de degrau, e a machina que guarda o foco. É a bateria que a
// issue exige: Tab alterna, seta anda, Enter entra, e a tecla alheia segue.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/component/event.hpp>

#include <string>
#include <vector>

#include "nucleo/rol.hpp"
#include "tui/menu.hpp"

namespace nu = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

// listas — as listas do operador que a taboada da barra consome. Escrevem-se á
// mão para o caso aferir contra taboa que se lê, e o id vae de dez em dez para
// que confundir id com indice apareça em vez de passar por acaso.
std::vector<nu::Rol> listas(int quantas) {
  std::vector<nu::Rol> feitas;
  for (int i = 1; i <= quantas; ++i)
    feitas.push_back({i * 10, "lista " + std::to_string(i), 0});
  return feitas;
}

}  // namespace

TEST_CASE("a conta dos degraus da barra é a das listas mais cinco") {
  CHECK(tui::degraus_da_barra(0) == 5);
  CHECK(tui::degraus_da_barra(1) == 6);
  CHECK(tui::degraus_da_barra(5) == 10);
}

// O espelho é o que garante que o dedo acorda na fileira que o Enter abriria.
TEST_CASE("a taboada da barra é espelho fiel com zero, uma e cinco listas") {
  for (const int quantas : {0, 1, 5}) {
    const std::vector<nu::Rol> rois = listas(quantas);
    for (std::size_t i = 0; i < tui::degraus_da_barra(rois.size()); ++i) {
      const tui::AlvoDaBarra alvo = tui::alvo_do_degrau(i, rois);
      CHECK(tui::degrau_da_secao(alvo.secao, rois, alvo.rol) == i);
    }
    CHECK(tui::alvo_do_degrau(99, rois).secao == tui::Secao::Busca);
  }
}

using ftxui::Event;
using Gesto = tui::GestoDaBarra;

TEST_CASE("a taboada do degrau e a da secção são espelho fiel") {
  for (std::size_t i = 0; i < tui::DEGRAUS_DA_BARRA; ++i)
    CHECK(tui::degrau_da_secao(tui::secao_do_degrau(i)) == i);
  // Dentro de uma lista o degrau é o das LISTAS, como na pintura de hoje.
  CHECK(tui::degrau_da_secao(tui::Secao::NoRol) ==
        tui::degrau_da_secao(tui::Secao::Rois));
  // Degrau fóra da conta não estoura: cahe no primeiro.
  CHECK(tui::secao_do_degrau(99) == tui::Secao::Artistas);
}

TEST_CASE("a ordem da barra é as minhas musicas, as listas, e os de navegar") {
  const std::vector<nu::Rol> rois = listas(2);
  CHECK(tui::alvo_do_degrau(0, rois).secao == tui::Secao::Busca);
  CHECK(tui::alvo_do_degrau(1, rois).rol == 10);
  CHECK(tui::alvo_do_degrau(2, rois).rol == 20);
  CHECK(tui::alvo_do_degrau(3, rois).secao == tui::Secao::Artistas);
  CHECK(tui::alvo_do_degrau(4, rois).secao == tui::Secao::Albuns);
  CHECK(tui::alvo_do_degrau(5, rois).secao == tui::Secao::Rede);
  CHECK(tui::alvo_do_degrau(6, rois).secao == tui::Secao::Lista);
  CHECK(tui::rotulo_do_degrau(0, rois) == "MINHAS MÚSICAS");
  CHECK(tui::rotulo_do_degrau(2, rois) == "lista 2");
  CHECK(tui::rotulo_do_degrau(4, rois) == "ÁLBUNS");
  CHECK(tui::rotulo_do_degrau(6, rois) == "SPOTIFY");
}

TEST_CASE("as secções sem fileira propria acordam onde a barra as mostra") {
  const std::vector<nu::Rol> rois = listas(3);
  // As FAIXAS de um album acendem ÁLBUNS, que é o degrau de que se veio.
  CHECK(tui::degrau_da_secao(tui::Secao::Faixas, rois, 0) ==
        tui::degrau_da_secao(tui::Secao::Albuns, rois, 0));
  // As LISTAS abrem-se pelo `P`: degrau algum da barra as tem por alvo, e ellas
  // acordam no alto em vez de acenderem fileira que o operador não pode eleger.
  CHECK(tui::degrau_da_secao(tui::Secao::Rois, rois, 0) == 0);
  for (std::size_t i = 0; i < tui::degraus_da_barra(rois.size()); ++i)
    CHECK(tui::alvo_do_degrau(i, rois).secao != tui::Secao::Rois);
  // Dentro de uma lista o degrau é o d'ELLA; o id que sumiu cahe no alto.
  CHECK(tui::degrau_da_secao(tui::Secao::NoRol, rois, 20) == 2);
  CHECK(tui::degrau_da_secao(tui::Secao::NoRol, rois, 999) == 0);
}

TEST_CASE("o Tab e o Shift+Tab abrem o menu e mais tecla alguma o abre") {
  CHECK(tui::tecla_abre_menu(Event::Tab));
  CHECK(tui::tecla_abre_menu(Event::TabReverse));
  CHECK_FALSE(tui::tecla_abre_menu(Event::Return));
  CHECK_FALSE(tui::tecla_abre_menu(Event::Escape));
  CHECK_FALSE(tui::tecla_abre_menu(Event::Character('P')));
}

TEST_CASE("na barra aberta cada tecla dá o gesto da taboada") {
  for (const Event& fecha : {Event::Tab, Event::TabReverse, Event::Escape,
                             Event::Backspace, Event::ArrowLeft})
    CHECK(tui::gesto_da_barra(fecha) == Gesto::Fecha);
  CHECK(tui::gesto_da_barra(Event::ArrowDown) == Gesto::Desce);
  CHECK(tui::gesto_da_barra(Event::Character('j')) == Gesto::Desce);
  CHECK(tui::gesto_da_barra(Event::ArrowUp) == Gesto::Sobe);
  CHECK(tui::gesto_da_barra(Event::Character('k')) == Gesto::Sobe);
  CHECK(tui::gesto_da_barra(Event::Home) == Gesto::AoPrincipio);
  CHECK(tui::gesto_da_barra(Event::Character('g')) == Gesto::AoPrincipio);
  CHECK(tui::gesto_da_barra(Event::End) == Gesto::AoFim);
  CHECK(tui::gesto_da_barra(Event::Character('G')) == Gesto::AoFim);
  CHECK(tui::gesto_da_barra(Event::Return) == Gesto::Entra);
  CHECK(tui::gesto_da_barra(Event::ArrowRight) == Gesto::Entra);
}

// O Alheio é a regra aprovada no portão: a tecla que a barra não conhece
// fecha-a e SEGUE, e é ella que faz o atalho levar aonde a barra levaria.
TEST_CASE("a tecla alheia é alheia e é ella que faz o atalho valer") {
  for (const Event& alheia :
       {Event::Character('P'), Event::Character('s'), Event::Character('I'),
        Event::Character(' '), Event::Character('q'), Event::Character('n'),
        Event::Character('/'), Event::Character('z')})
    CHECK(tui::gesto_da_barra(alheia) == Gesto::Alheio);
}

TEST_CASE("o Tab alterna e a seta anda saturando nos extremos") {
  tui::Menu menu;
  CHECK_FALSE(menu.aberto());
  menu.abre(tui::Secao::NoRol);  // dentro de uma lista: acorda em LISTS
  CHECK(menu.aberto());
  CHECK(menu.alvo() == tui::Secao::Rois);
  menu.sobe();  // LISTS, NET, SEARCH, TRACKS, ALBUMS, ARTISTS, e satura
  for (int i = 0; i < 9; ++i) menu.sobe();
  CHECK(menu.alvo() == tui::Secao::Artistas);
  for (int i = 0; i < 9; ++i) menu.desce();  // até SPOTIFY, e satura
  CHECK(menu.alvo() == tui::Secao::Lista);
  menu.ao_principio();
  CHECK(menu.degrau() == 0);
  menu.ao_fim();
  CHECK(menu.degrau() + 1 == tui::DEGRAUS_DA_BARRA);
  menu.fecha();
  CHECK_FALSE(menu.aberto());
  menu.abre(tui::Secao::Busca);  // reabrir assenta na secção corrente
  CHECK(menu.alvo() == tui::Secao::Busca);
}

TEST_CASE("a barra satura nos extremos com zero, uma, cinco e cincoenta listas") {
  for (const int quantas : {0, 1, 5, 50}) {
    const std::vector<nu::Rol> rois = listas(quantas);
    tui::Menu menu;
    menu.abre(tui::Secao::Busca, rois, 0);
    CHECK(menu.degrau() == 0);
    for (int i = 0; i < 99; ++i) menu.sobe();
    CHECK(menu.degrau() == 0);  // menu não é carrossel
    for (int i = 0; i < 99; ++i) menu.desce();
    CHECK(menu.degrau() + 1 == tui::degraus_da_barra(rois.size()));
    menu.ao_principio();
    CHECK(menu.degrau() == 0);
    menu.ao_fim();
    CHECK(menu.degrau() + 1 == tui::degraus_da_barra(rois.size()));
  }
}

TEST_CASE("dentro de uma lista a barra acorda na fileira d'ella") {
  const std::vector<nu::Rol> rois = listas(3);
  tui::Menu menu;
  menu.abre(tui::Secao::NoRol, rois, 30);
  CHECK(menu.degrau() == 3);
  CHECK(tui::alvo_do_degrau(menu.degrau(), rois).rol == 30);
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
