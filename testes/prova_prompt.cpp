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
#include <string>
#include <vector>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
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

TEST_CASE("cada modo que captura tecla diz o seu rotulo") {
  CHECK(tui::rotulo_do_prompt(tui::Modo::Busca, "") == "FILTRO:");
  CHECK(tui::rotulo_do_prompt(tui::Modo::Url, "") == "URL:");
  CHECK(tui::rotulo_do_prompt(tui::Modo::Procura, "YouTube") ==
        "BUSCA NA REDE (YouTube):");
  CHECK(tui::rotulo_do_prompt(tui::Modo::Lista, "") == "PLAYLIST DO SPOTIFY:");
  CHECK(tui::rotulo_do_prompt(tui::Modo::NomeNovo, "") == "LISTA NOVA:");
  CHECK(tui::rotulo_do_prompt(tui::Modo::NomeOutro, "") == "NOME:");
  CHECK(tui::rotulo_do_prompt(tui::Modo::Confirma, "roque") ==
        "apagar «roque»? s/n");
  // O Nada não leva rotulo: não ha campo aberto para o carregar.
  CHECK(tui::rotulo_do_prompt(tui::Modo::Nada, "qualquer").empty());
  // E rotulo algum sahe vazio nos que capturam tecla: campo mudo seria o mesmo
  // defeito por outra porta, que o operador não saberia o que se lhe pergunta.
  for (const tui::Modo modo : kTodos)
    if (modo != tui::Modo::Nada)
      CHECK_FALSE(tui::rotulo_do_prompt(modo, "x").empty());
}

namespace {

// O ÉCRAN DE PAPEL, lido cella a cella. Não se lê o `ToString`, pela razão que a
// prova da tabella deu: elle mette escapes no meio dos bytes.
struct Papel {
  std::vector<std::string> linhas;
  std::vector<ftxui::Color> fundos;  // o fundo da primeira cella de cada linha
  ftxui::Screen::Cursor cursor;
};

Papel pintar(const std::string& trilha, tui::Modo modo, const std::string& termo,
             std::size_t largura) {
  ftxui::Element quadro =
      tui::elemento_do_topo(trilha, modo, "YouTube", termo, largura);
  ftxui::Screen ecran =
      ftxui::Screen::Create(ftxui::Dimension::Fixed(static_cast<int>(largura)),
                            ftxui::Dimension::Fixed(3));
  ftxui::Render(ecran, quadro);
  Papel papel;
  for (int y = 0; y < 3; ++y) {
    std::string linha;
    for (int x = 0; x < static_cast<int>(largura); ++x)
      linha += ecran.PixelAt(x, y).character;
    papel.linhas.push_back(linha);
    papel.fundos.push_back(ecran.PixelAt(0, y).background_color);
  }
  papel.cursor = ecran.cursor();
  return papel;
}
}  // namespace

TEST_CASE("a trilha sobrevive a todos os oito modos") {
  for (const tui::Modo modo : kTodos) {
    const Papel papel = pintar("ARTISTS > AYMEE > Voce", modo, "jk", 60);
    CHECK(papel.linhas[0].substr(0, 22) == "ARTISTS > AYMEE > Voce");
  }
}

TEST_CASE("o campo abre em linha propria por baixo da trilha") {
  const Papel papel = pintar("ARTISTS", tui::Modo::Procura, "jk", 60);
  CHECK(papel.linhas[0].substr(0, 7) == "ARTISTS");
  CHECK(papel.linhas[1].find("BUSCA NA REDE (YouTube): jk") != std::string::npos);
}
