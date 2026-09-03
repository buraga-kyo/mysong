// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA LOUSA — testes/prova_lousa.cpp
// ══════════════════════════════════════════════════════════════════════════
// O protocolo do Überzug++, a conta do rectangulo e a decisão da alavanca,
// tudo sem X11 vivo e sem se erguer processo algum: as tres cousas são
// funcções puras, e é justamente para isto que ellas o são.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <algorithm>
#include <string>

#include "nucleo/capa.hpp"
#include "nucleo/lousa.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("a ordem de pôr traz o rectangulo e o caminho n'uma linha de JSON") {
  CHECK(nu::ordem_de_por("capa", "/tmp/a.jpg", 10, 5, 40, 12) ==
        "{\"action\":\"add\",\"identifier\":\"capa\",\"x\":10,\"y\":5,"
        "\"max_width\":40,\"max_height\":12,\"path\":\"/tmp/a.jpg\"}\n");
}

TEST_CASE("a ordem de tirar nomeia sómente a identidade") {
  CHECK(nu::ordem_de_tirar("capa") ==
        "{\"action\":\"remove\",\"identifier\":\"capa\"}\n");
}

TEST_CASE("aspa no caminho escapa-se e não parte a linha ao meio") {
  const std::string ordem =
      nu::ordem_de_por("capa", "/tmp/o \"melhor\"/a.jpg", 0, 0, 1, 1);
  CHECK(ordem.find("\\\"melhor\\\"") != std::string::npos);
  CHECK(std::count(ordem.begin(), ordem.end(), '\n') == 1);
}

TEST_CASE("a barra invertida e o de controle vão na fórma que a norma pede") {
  CHECK(nu::escapado_em_json("a\\b") == "a\\\\b");
  CHECK(nu::escapado_em_json(std::string("a\x01"
                                         "b")) == "a\\u0001b");
  CHECK(nu::escapado_em_json("Música") == "Música");  // o UTF-8 passa inteiro
}

TEST_CASE("a capa de dezaseis por nove deixa fileiras para o espectro") {
  // A célulla é de nove por vinte pixeis: 1280 por 720 em quarenta collunhas
  // pede 40*720*9 / (1280*20), que é 10,125, e arredonda para onze linhas.
  const nu::Retangulo qual =
      nu::rectangulo_da_capa({1280, 720}, 40, 21, nu::CELLULA_DA_CASA);
  CHECK(qual.collunas == 40);
  CHECK(qual.linhas == 11);
}

TEST_CASE("a capa quadrada em tecto baixo encolhe a largura") {
  // 500 por 500 em quarenta collunhas pediria dezoito linhas, e cabem.
  CHECK(nu::rectangulo_da_capa({500, 500}, 40, 21, nu::CELLULA_DA_CASA).linhas ==
        18);
  // Com tecto de dez linhas, a altura manda: 10*500*20 / (500*9) dá 22,2, que
  // arredonda para vinte e tres collunhas, e a proporção fica guardada.
  const nu::Retangulo baixo =
      nu::rectangulo_da_capa({500, 500}, 40, 10, nu::CELLULA_DA_CASA);
  CHECK(baixo.collunas == 23);
  CHECK(baixo.linhas == 10);
}

TEST_CASE("medida por ler toma o tecto inteiro") {
  const nu::Retangulo qual =
      nu::rectangulo_da_capa({0, 0}, 40, 21, nu::CELLULA_DA_CASA);
  CHECK(qual.collunas == 40);
  CHECK(qual.linhas == 21);
}

TEST_CASE("painel de largura zero não pede rectangulo algum") {
  const nu::Retangulo qual =
      nu::rectangulo_da_capa({1280, 720}, 0, 21, nu::CELLULA_DA_CASA);
  CHECK(qual.collunas == 0);
  CHECK(qual.linhas == 0);
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
