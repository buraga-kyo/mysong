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
  std::vector<ftxui::Color> fundos_meio;  // e o da cella do MEIO da linha
  ftxui::Screen::Cursor cursor;
};

Papel pintar(tui::Modo modo, const std::string& termo, std::size_t largura) {
  ftxui::Element quadro =
      tui::elemento_do_campo(modo, "YouTube", termo, largura);
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
    papel.fundos_meio.push_back(
        ecran.PixelAt(static_cast<int>(largura) / 2, y).background_color);
  }
  papel.cursor = ecran.cursor();
  return papel;
}
}  // namespace

TEST_CASE("o campo abre em linha propria, e ella é a primeira") {
  const Papel papel = pintar(tui::Modo::Procura, "jk", 60);
  CHECK(papel.linhas[0].find("BUSCA NA REDE (YouTube): jk") != std::string::npos);
  // A linha de baixo fica LIMPA: a sala reserva UMA linha ao campo, e peça que
  // pintasse duas empurraria a pauta para fóra da conta que a sala lhe deu.
  CHECK(papel.linhas[1].find_first_not_of(' ') == std::string::npos);
}

TEST_CASE("o campo tem fundo proprio em toda a linha") {
  const Papel papel = pintar(tui::Modo::Procura, "jk", 60);
  const Papel fechado = pintar(tui::Modo::Nada, "", 60);
  CHECK(papel.fundos[0] != fechado.fundos[0]);
  // A primeira cella é a da marca: regressão que pintasse SÓ a marca passaria
  // por ella. O meio da linha do campo leva o fundo tambem.
  CHECK(papel.fundos_meio[0] != fechado.fundos_meio[0]);
}

TEST_CASE("o caret pousa logo a seguir ao que se digitou") {
  // duas collunhas da marca, vinte e quatro do rotulo, uma do espaço e duas do
  // termo: o caret cahe na vigesima nona, e na linha do campo.
  const Papel papel = pintar(tui::Modo::Procura, "jk", 60);
  CHECK(papel.cursor.x == 29);
  CHECK(papel.cursor.y == 0);
}

TEST_CASE("o caret e barra quieta e nao a piscar") {
  // A queixa que abriu a issue irmã #78 foi «o meu cursor fica piscando». Caret
  // a piscar aqui responderia á queixa com a propria queixa.
  const Papel papel = pintar(tui::Modo::Url, "x", 60);
  CHECK(papel.cursor.shape == ftxui::Screen::Cursor::Bar);
}

TEST_CASE("fechado o campo linha alguma se pinta") {
  const Papel papel = pintar(tui::Modo::Nada, "", 40);
  CHECK(papel.linhas[0].find_first_not_of(' ') == std::string::npos);
  CHECK(papel.cursor.shape == ftxui::Screen::Cursor::Hidden);
}

TEST_CASE("a pergunta do apagar tem linha propria mas nao pede caret") {
  const Papel papel = pintar(tui::Modo::Confirma, "", 40);
  CHECK(papel.linhas[0].find("apagar") != std::string::npos);
  CHECK(papel.cursor.shape == ftxui::Screen::Cursor::Hidden);
}

TEST_CASE("terminal estreito nao empurra o caret para fora") {
  const Papel papel =
      pintar(tui::Modo::Url, std::string(40, 'z'), 20);
  CHECK(papel.cursor.x < 20);
  CHECK(papel.linhas[0].find("zz") != std::string::npos);
}

TEST_CASE("o corte do termo nao parte codepoint ao meio") {
  const Papel papel = pintar(tui::Modo::Url, "çãoçãoçãoçãoção", 14);
  CHECK(papel.linhas[0].find("ção") != std::string::npos);
  CHECK(papel.cursor.x < 14);
}

TEST_CASE("o rotulo sobrevive ao termo comprido no terminal estreito") {
  // O caso que morde: `b` com URL comprida em oitenta collunhas. O rabo da
  // cadeia INTEIRA comia o «URL: » pela esquerda, e o operador ficava com um
  // rabo de texto SEM nome de campo, que é parente do defeito que esta issue
  // veio matar. O rotulo mostra-se INTEIRO, e quem perde o começo é o termo.
  const Papel papel =
      pintar(tui::Modo::Url, std::string(90, 'w'), 80);
  CHECK(papel.linhas[0].find("URL: w") != std::string::npos);
  CHECK(papel.cursor.x < 80);
  // E não cabendo nem o rotulo, apara-se ELLE á direita: fica o começo, que é
  // o que diz o officio, e o caret pousa na ultima collunha.
  const Papel curto = pintar(tui::Modo::Procura, "abc", 8);
  CHECK(curto.linhas[0].find("BUSCA") != std::string::npos);
  CHECK(curto.cursor.x == 7);
}
