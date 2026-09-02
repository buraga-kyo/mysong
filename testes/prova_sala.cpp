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
