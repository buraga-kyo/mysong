// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA LETRA — testes/prova_letra.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum toca a rede. Os corpos vão escriptos á mão, com a fórma que o LRCLIB
// de facto devolve, e a prova á mão contra o serviço vivo está no PR.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>

#include "nucleo/letra.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("o escape da URL cobre o que parte a consulta") {
  CHECK(nu::escapa_para_url("Creep") == "Creep");
  CHECK(nu::escapa_para_url("a b") == "a%20b");
  // Os tres que mais enganam n'um titulo de musica.
  CHECK(nu::escapa_para_url("Rock & Roll") == "Rock%20%26%20Roll");
  CHECK(nu::escapa_para_url("Track #1") == "Track%20%231");
  CHECK(nu::escapa_para_url("A+B") == "A%2BB");
  // O que NÃO se escapa, pela lista fechada do RFC 3986.
  CHECK(nu::escapa_para_url("a-b.c_d~e") == "a-b.c_d~e");
  // Acento sahe em dous grupos, que é UTF-8 por octeto.
  CHECK(nu::escapa_para_url("á") == "%C3%A1");
  CHECK(nu::escapa_para_url("") == "");
}

// O RECORTE do primeiro objecto. Os casos que importam são os que trazem chave
// DENTRO de aspas: letra de musica tem-nas, e recorte que conte chaves á cega
// pararia na primeira.
TEST_CASE("o recorte do primeiro objecto respeita aspas e contra-barra") {
  CHECK(nu::primeiro_objecto("[{\"a\":1},{\"b\":2}]") == "{\"a\":1}");
  CHECK(nu::primeiro_objecto("[]").empty());
  CHECK(nu::primeiro_objecto("").empty());
  CHECK(nu::primeiro_objecto("nada de json").empty());
  // Chave DENTRO de aspas não fecha o objecto.
  CHECK(nu::primeiro_objecto("[{\"a\":\"}\"},{\"b\":2}]") ==
        "{\"a\":\"}\"}");
  // Aspa escapada não fecha a cadeia, donde a chave que a segue continua dentro.
  CHECK(nu::primeiro_objecto("[{\"a\":\"x\\\"}\"},{\"b\":2}]") ==
        "{\"a\":\"x\\\"}\"}");
  // Arranjo truncado devolve NADA, e não objecto meio.
  CHECK(nu::primeiro_objecto("[{\"a\":1").empty());
  CHECK(nu::primeiro_objecto("[{\"a\":\"sem fecho").empty());
  // Objecto sozinho, sem arranjo, tambem se recorta.
  CHECK(nu::primeiro_objecto("{\"a\":1}") == "{\"a\":1}");
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
