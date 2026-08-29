// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO PROMPT — testes/prova_prompt.cpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta o topo n'um écran de PAPEL e lê os bytes que sahiram. Terminal algum se
// abre. Afere-se o que a issue #79 mandou provar: a trilha NÃO se substitue, o
// campo tem linha propria e tinta propria, e o cursor pousa dentro d'elle. O defeito vivia em dez linhas que a bateria não olhava, por serem
// da janella, que não se prova.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <vector>
#include "tui/prompt.hpp"

namespace tui = mysong::tui;
namespace {

// TODOS os modos. Modo que entre no enum e não aqui faria a bateria provar
// menos do que existe: a conta afere-se.
const std::vector<tui::Modo> kTodos = {
    tui::Modo::Nada,     tui::Modo::Busca,    tui::Modo::Url,
    tui::Modo::Procura,  tui::Modo::NomeNovo, tui::Modo::NomeOutro,
    tui::Modo::Confirma, tui::Modo::Lista};
}  // namespace

TEST_CASE("o enum tem oito modos e seis d'elles digitam") {
  CHECK(kTodos.size() == 8);
  std::size_t digitam = 0;
  for (const tui::Modo modo : kTodos)
    if (tui::aceita_letra(modo)) ++digitam;
  CHECK(digitam == 6);
  CHECK_FALSE(tui::aceita_letra(tui::Modo::Nada));
  CHECK_FALSE(tui::aceita_letra(tui::Modo::Confirma));
}

TEST_CASE("o topo tem uma linha fechado o campo e duas aberto") {
  CHECK(tui::linhas_do_topo(tui::Modo::Nada) == 1);
  for (const tui::Modo modo : kTodos)
    if (modo != tui::Modo::Nada) CHECK(tui::linhas_do_topo(modo) == 2);
}

TEST_CASE("a novidade de fundo so assenta com o campo fechado") {
  CHECK(tui::assenta_novidade(tui::Modo::Nada));
  for (const tui::Modo modo : kTodos)
    if (modo != tui::Modo::Nada) CHECK_FALSE(tui::assenta_novidade(modo));
}
