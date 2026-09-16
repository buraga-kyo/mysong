// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO CORREIO, testes/prova_correio.cpp
// ══════════════════════════════════════════════════════════════════════════
// Sem fios e sem tela: o que se afere é a conta das duas gerações, que é o que faz
// a colheita consumir e a substituição funccionar.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "tui/correio.hpp"

namespace tui = mysong::tui;

namespace {

tui::Linha linha(const std::string& texto) { return {texto, texto, 0, 0, {}}; }

}  // namespace

TEST_CASE("correio vazio não tem o que colher") {
  tui::Correio correio;
  std::vector<tui::Linha> achados;
  std::string recado;
  CHECK_FALSE(correio.colhe(&achados, &recado));
  CHECK(correio.geracao() == 0);
}

TEST_CASE("a colheita CONSOME, e a segunda vem vazia") {
  tui::Correio correio;
  correio.poe({linha("Toccata")}, "um achado");
  CHECK(correio.geracao() == 1);

  std::vector<tui::Linha> achados;
  std::string recado;
  REQUIRE(correio.colhe(&achados, &recado));
  CHECK(achados.size() == 1);
  CHECK(achados[0].texto == "Toccata");
  CHECK(recado == "um achado");

  // A SEGUNDA é falsa. Sem o consumo, a tela poria a lista outra vez a cada quadro,
  // e o operador não poderia andar n'ella: o eleito voltava ao alto vinte vezes por
  // segundo.
  CHECK_FALSE(correio.colhe(&achados, &recado));
  // E a geração NÃO decresce ao colher: ella conta o que se pôz, não o que sobra.
  CHECK(correio.geracao() == 1);
}

TEST_CASE("recado novo substitue o velho, e a geração conta os dous") {
  tui::Correio correio;
  correio.poe({linha("velho")}, "primeira busca");
  correio.poe({linha("novo"), linha("outro")}, "segunda busca");
  CHECK(correio.geracao() == 2);

  std::vector<tui::Linha> achados;
  std::string recado;
  REQUIRE(correio.colhe(&achados, &recado));
  // O velho NÃO chega: ninguem quer ver a busca de antes cahir depois da de agora.
  REQUIRE(achados.size() == 2);
  CHECK(achados[0].texto == "novo");
  CHECK(recado == "segunda busca");
  // Duas postas e uma colheita: a colheita apanha as duas, e não sobra a primeira.
  CHECK_FALSE(correio.colhe(&achados, &recado));
}

TEST_CASE("colher sem querer o conteudo tambem consome") {
  tui::Correio correio;
  correio.poe({linha("Toccata")}, "recado");
  // Punho nullo é pedido legitimo: quem sómente quer saber se houve recado não ha de
  // ser obrigado a declarar duas variaveis para as jogar fóra.
  CHECK(correio.colhe(nullptr, nullptr));
  CHECK_FALSE(correio.colhe(nullptr, nullptr));
}

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
