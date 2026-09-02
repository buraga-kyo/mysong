// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA SALA — testes/prova_sala.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada pura da sala, e mais adiante os écrans de PAPEL d'ella, lidos
// cella a cella. Terminal algum se abre, banco algum: os retractos que a sala
// pede armam-se á mão, que é o que faz d'ella peça aferivel.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "tui/sala.hpp"

namespace tui = mysong::tui;

TEST_CASE("a somma da colleção diz-se por extenso") {
  CHECK(tui::texto_da_duracao(0).empty());
  CHECK(tui::texto_da_duracao(-5).empty());
  CHECK(tui::texto_da_duracao(45) == "45s");
  CHECK(tui::texto_da_duracao(59) == "59s");
  CHECK(tui::texto_da_duracao(60) == "1min");
  CHECK(tui::texto_da_duracao(3599) == "59min");
  CHECK(tui::texto_da_duracao(3600) == "1h00");
  CHECK(tui::texto_da_duracao(3900) == "1h05");
  CHECK(tui::texto_da_duracao(4980) == "1h23");
}

TEST_CASE("a conta da colleção muda de substantivo com a especie") {
  CHECK(tui::texto_da_conta(4, tui::Especie::Faixas, 840) == "4 FAIXAS, 14min");
  CHECK(tui::texto_da_conta(1, tui::Especie::Faixas, 0) == "1 FAIXA");
  CHECK(tui::texto_da_conta(0, tui::Especie::Faixas, 0) == "0 FAIXAS");
  CHECK(tui::texto_da_conta(12, tui::Especie::Artistas, 0) == "12 ARTISTAS");
  CHECK(tui::texto_da_conta(1, tui::Especie::Albuns, 0) == "1 ÁLBUM");
  CHECK(tui::texto_da_conta(3, tui::Especie::Listas, 0) == "3 LISTAS");
  CHECK(tui::texto_da_conta(9, tui::Especie::Achados, 0) == "9 ACHADOS");
}

TEST_CASE("o cabeçalho nomeia a colleção pela secção e pela trilha") {
  const std::vector<std::string> alto;
  const std::vector<std::string> fundo = {"Boards of Canada", "Geogaddi"};
  CHECK(tui::nome_da_colleccao(tui::Secao::Busca, alto, "") == "MINHAS MÚSICAS");
  CHECK(tui::nome_da_colleccao(tui::Secao::Artistas, alto, "") == "ARTISTAS");
  CHECK(tui::nome_da_colleccao(tui::Secao::Albuns, alto, "") == "ÁLBUNS");
  CHECK(tui::nome_da_colleccao(tui::Secao::Albuns, fundo, "") == "Geogaddi");
  CHECK(tui::nome_da_colleccao(tui::Secao::Faixas, fundo, "") == "Geogaddi");
  CHECK(tui::nome_da_colleccao(tui::Secao::Rede, alto, "") == "REDE");
  CHECK(tui::nome_da_colleccao(tui::Secao::Rois, alto, "") == "LISTAS");
  CHECK(tui::nome_da_colleccao(tui::Secao::NoRol, fundo, "") == "Geogaddi");
  CHECK(tui::nome_da_colleccao(tui::Secao::Lista, alto, "") == "SPOTIFY");
  CHECK(tui::nome_da_colleccao(tui::Secao::Lista, alto, "Verão") == "Verão");
}

TEST_CASE("cada secção conta a sua especie") {
  CHECK(tui::especie_da_secao(tui::Secao::Artistas) == tui::Especie::Artistas);
  CHECK(tui::especie_da_secao(tui::Secao::Albuns) == tui::Especie::Albuns);
  CHECK(tui::especie_da_secao(tui::Secao::Rois) == tui::Especie::Listas);
  CHECK(tui::especie_da_secao(tui::Secao::Rede) == tui::Especie::Achados);
  CHECK(tui::especie_da_secao(tui::Secao::Busca) == tui::Especie::Faixas);
  CHECK(tui::especie_da_secao(tui::Secao::NoRol) == tui::Especie::Faixas);
}

// A BARRA entra por parametro em toda chamada: as onze collunhas são as de
// hoje, e a tarefa irmã da bibliotheca ha de as alargar sem tocar n'este
// arquivo. Passa-se o numero, e não se o presume.
TEST_CASE("a geometria esconde o painel abaixo de cem collunhas de tela") {
  // 96 collunhas UTEIS são as cem da tela do operador menos as quatro da orla.
  for (std::size_t larga : {40, 59, 60, 95})
    CHECK(tui::geometria_da_sala(larga, 38, 11).painel == 0);
  CHECK(tui::geometria_da_sala(95, 38, 11).meio == 84);
  const tui::Geometria justa = tui::geometria_da_sala(96, 38, 11);
  CHECK(justa.painel == 24);
  CHECK(justa.meio == 60);
  CHECK(justa.meio + 1 + justa.painel + 11 == 96);
}

TEST_CASE("o painel toma a quarta parte da largura e o meio o que sobra") {
  const tui::Geometria d160 = tui::geometria_da_sala(156, 38, 11);
  CHECK(d160.painel == 39);
  CHECK(d160.meio == 105);
  CHECK(d160.livre == 34);
  CHECK(d160.capa == 20);
  CHECK(d160.cabecalho == 6);
  CHECK(d160.tabella == 32);
  const tui::Geometria d220 = tui::geometria_da_sala(216, 38, 11);
  CHECK(d220.painel == 54);
  CHECK(d220.meio + 1 + d220.painel + 11 == 216);
}

TEST_CASE("o espectro não desce de oito linhas em altura alguma") {
  for (std::size_t alta = 1; alta <= 60; ++alta) {
    const tui::Geometria geo = tui::geometria_da_sala(156, alta, 11);
    if (geo.painel != 0) CHECK(geo.livre - geo.capa >= 8);
    CHECK(geo.cabecalho + geo.tabella == (alta == 0 ? 1 : alta));
  }
}

TEST_CASE("terminal baixo cede a capa, depois o cabeçalho, depois o painel") {
  const tui::Geometria baixa = tui::geometria_da_sala(156, 13, 11);
  CHECK(baixa.painel == 39);
  CHECK(baixa.capa == 0);
  CHECK(baixa.cabecalho == 6);
  CHECK(baixa.tabella == 7);
  const tui::Geometria rasa = tui::geometria_da_sala(156, 5, 11);
  CHECK(rasa.painel == 0);
  CHECK(rasa.cabecalho == 0);
  CHECK(rasa.tabella == 5);
}
