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

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
