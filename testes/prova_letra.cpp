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

// A LEITURA da resposta, sobre corpos com a fórma que o LRCLIB de facto devolve.
TEST_CASE("a resposta lê-se, e a instrumental dá letra vazia sem erro") {
  const std::string corpo =
      "[{\"id\":30020794,\"trackName\":\"Creep\",\"artistName\":\"Radiohead\","
      "\"duration\":237.0,\"instrumental\":false,"
      "\"plainLyrics\":\"When you were here before\\nYou float like a feather\","
      "\"syncedLyrics\":\"[00:11.00] When you were here before\\n"
      "[00:16.30] You float like a feather\"}]";
  const nu::Letra lida = nu::le_resposta(corpo);
  CHECK(lida.sincronizada ==
        "[00:11.00] When you were here before\n[00:16.30] You float like a feather");
  CHECK(lida.plana == "When you were here before\nYou float like a feather");

  // Instrumental: o LRCLIB devolve nulo, e ler texto de um nulo daria cadeia vazia
  // por ACASO. O typo confere-se, donde é por decisão.
  const nu::Letra muda = nu::le_resposta(
      "[{\"trackName\":\"Prelude\",\"instrumental\":true,"
      "\"plainLyrics\":null,\"syncedLyrics\":null}]");
  CHECK(muda.sincronizada.empty());
  CHECK(muda.plana.empty());

  // Corpo vazio, corpo de erro, e corpo que não é JSON: os tres dão letra vazia.
  CHECK(nu::le_resposta("").sincronizada.empty());
  CHECK(nu::le_resposta("[]").sincronizada.empty());
  const nu::Letra erro = nu::le_resposta(
      "{\"message\":\"Failed to find specified track\","
      "\"name\":\"TrackNotFound\",\"statusCode\":404}");
  CHECK(erro.sincronizada.empty());
  CHECK(erro.plana.empty());
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
