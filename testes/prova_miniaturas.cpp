#include <doctest/doctest.h>

#include "tui/miniaturas.hpp"

namespace tui = mysong::tui;

namespace {
std::vector<tui::Linha> faixas() {
  return {{"A", "/a.mp3", 0, 0, {}}, {"B", "/b.mp3", 0, 0, {}}, {"C", "/c.mp3", 0, 0, {}}, {"D", "/d.mp3", 0, 0, {}}};
}
std::filesystem::path imagem(const std::string& faixa) { return faixa + ".jpg"; }
}

TEST_CASE("miniaturas carregam somente faixas inteiras na area visivel") {
  std::vector<std::string> consultadas;
  const auto plano = tui::miniaturas_da_lista(faixas(), 1, {4, 9, 40, 8}, true,
      [&](const std::string& faixa) { consultadas.push_back(faixa); return imagem(faixa); });
  REQUIRE(plano.size() == 2);
  CHECK(consultadas == std::vector<std::string>{"/b.mp3", "/c.mp3"});
  CHECK(plano[0].indice == 1);
  CHECK(plano[0].caixa.x == 4);
  CHECK(plano[0].caixa.y == 9);
  CHECK(plano[0].caixa.largura == 8);
  CHECK(plano[0].caixa.altura == 3);
  CHECK(plano[1].caixa.y == 12);
  CHECK(plano[1].caixa.y + plano[1].caixa.altura <= 17);
}

TEST_CASE("miniaturas nao abrem arquivos sem suporte ou espaco") {
  int consultas = 0;
  const auto carrega = [&](const std::string&) {
    ++consultas; return std::filesystem::path("/imagem.jpg");
  };
  CHECK(tui::miniaturas_da_lista(faixas(), 0, {0, 0, 40, 6}, false, carrega).empty());
  CHECK(tui::miniaturas_da_lista(faixas(), 4, {0, 0, 40, 6}, true, carrega).empty());
  CHECK(tui::miniaturas_da_lista(faixas(), 0, {0, 0, 8, 6}, true, carrega).empty());
  CHECK(tui::miniaturas_da_lista(faixas(), 0, {0, 0, 40, 2}, true, carrega).empty());
  CHECK(consultas == 0);
}

TEST_CASE("miniatura ausente conserva identidade dos demais lugares") {
  const auto plano = tui::miniaturas_da_lista(faixas(), 0, {0, 0, 40, 9}, true,
      [](const std::string& faixa) { return faixa == "/b.mp3" ? std::filesystem::path{} : imagem(faixa); });
  REQUIRE(plano.size() == 2);
  CHECK(plano[0].identidade == "miniatura_lista_0");
  CHECK(plano[1].identidade == "miniatura_lista_2");
  CHECK(plano[1].indice == 2);
  CHECK(plano[1].caixa.y == 6);
}

TEST_CASE("espectro animado nao reenvia imagens da lista") {
  tui::ReconciliadorDeMiniaturas reconciliador;
  const auto plano = tui::miniaturas_da_lista(faixas(), 0, {0, 0, 40, 9}, true, imagem);
  CHECK(reconciliador.atualiza(plano).size() == 3);
  for (int quadro = 0; quadro < 200; ++quadro) CHECK(reconciliador.atualiza(plano).empty());
}

TEST_CASE("rolagem atualiza lugares e remove imagens que nao cabem mais") {
  tui::ReconciliadorDeMiniaturas reconciliador;
  reconciliador.atualiza(tui::miniaturas_da_lista(faixas(), 0, {0, 0, 40, 9}, true, imagem));
  const auto alteracoes = reconciliador.atualiza(
      tui::miniaturas_da_lista(faixas(), 2, {0, 5, 40, 6}, true, imagem));
  REQUIRE(alteracoes.size() == 3);
  CHECK(alteracoes[0].remover);
  CHECK(alteracoes[0].miniatura.identidade == "miniatura_lista_2");
  CHECK_FALSE(alteracoes[1].remover);
  CHECK(alteracoes[1].miniatura.imagem == "/c.mp3.jpg");
  CHECK(alteracoes[1].miniatura.caixa.y == 5);
  CHECK(alteracoes[2].miniatura.caixa.y == 8);
}

TEST_CASE("menus e troca de aba removem sobreposicoes e reabertura restaura") {
  tui::ReconciliadorDeMiniaturas reconciliador;
  const auto plano = tui::miniaturas_da_lista(faixas(), 0, {0, 0, 40, 6}, true, imagem);
  reconciliador.atualiza(plano);
  const auto ocultas = reconciliador.atualiza({});
  REQUIRE(ocultas.size() == 2);
  CHECK(ocultas[0].remover);
  CHECK(ocultas[1].remover);
  CHECK(reconciliador.atualiza({}).empty());
  CHECK(reconciliador.atualiza(plano).size() == 2);
}
