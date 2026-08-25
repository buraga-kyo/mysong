// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA LETRA — testes/prova_letra.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum toca a rede. Os corpos vão escriptos á mão, com a fórma que o LRCLIB
// de facto devolve, e a prova á mão contra o serviço vivo está no PR.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <limits>
#include <string>
#include <vector>

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

TEST_CASE("o caminho do lrc é o do audio com outra extensão") {
  CHECK(nu::caminho_do_lrc("/acervo/A/B/01 - Tear.mp3").string() ==
        "/acervo/A/B/01 - Tear.lrc");
  CHECK(nu::caminho_do_lrc("/acervo/A/B/01 - Tear.flac").string() ==
        "/acervo/A/B/01 - Tear.lrc");
  // Nome com ponto no meio: sómente a ULTIMA extensão se troca.
  CHECK(nu::caminho_do_lrc("/a/Vol. 2 - Tear.opus").string() ==
        "/a/Vol. 2 - Tear.lrc");
  // Sem extensão alguma, acrescenta-se.
  CHECK(nu::caminho_do_lrc("/a/Tear").string() == "/a/Tear.lrc");
}

// A ANALYSE do lrc, sobre texto escripto á mão com as fórmas de verdade.
TEST_CASE("o lrc analysa-se, com carimbo duplo e cabeçalho saltado") {
  const std::string cru =
      "[ar:Radiohead]\n"
      "[ti:Creep]\n"
      "[00:19.19] When you were here before\n"
      "[00:28.94]You're just like an angel\n"   // sem espaço depois do carimbo
      "[00:47.27]\n"                            // carimbo SEM texto: o silencio
      "[01:00.00][02:30.50] So very special\n"  // carimbo DUPLO: repete-se
      "linha sem carimbo, que se salta\n";
  const std::vector<nu::LinhaDaLetra> linhas = nu::analysa_lrc(cru);
  REQUIRE(linhas.size() == 5u);
  CHECK(linhas[0].tempo == doctest::Approx(19.19));
  CHECK(linhas[0].texto == "When you were here before");
  CHECK(linhas[1].tempo == doctest::Approx(28.94));
  CHECK(linhas[1].texto == "You're just like an angel");
  // O carimbo sem texto CONSERVA-SE, com texto vazio: é elle que tira o verso
  // anterior da tela na hora certa.
  CHECK(linhas[2].tempo == doctest::Approx(47.27));
  CHECK(linhas[2].texto.empty());
  // O carimbo duplo deu DUAS linhas, com o mesmo texto e tempos differentes.
  CHECK(linhas[3].tempo == doctest::Approx(60.0));
  CHECK(linhas[3].texto == "So very special");
  CHECK(linhas[4].tempo == doctest::Approx(150.5));
  CHECK(linhas[4].texto == "So very special");
}

TEST_CASE("o lrc aceita os dous separadores dos centesimos") {
  const std::vector<nu::LinhaDaLetra> ponto = nu::analysa_lrc("[01:02.50] a\n");
  REQUIRE(ponto.size() == 1u);
  CHECK(ponto[0].tempo == doctest::Approx(62.5));
  // Dous pontos: ha gerador que o usa, e recusar faria a letra inteira sahir vazia.
  const std::vector<nu::LinhaDaLetra> dous = nu::analysa_lrc("[01:02:50] a\n");
  REQUIRE(dous.size() == 1u);
  CHECK(dous[0].tempo == doctest::Approx(62.5));
  // Sem centesimos.
  const std::vector<nu::LinhaDaLetra> curto = nu::analysa_lrc("[01:02] a\n");
  REQUIRE(curto.size() == 1u);
  CHECK(curto[0].tempo == doctest::Approx(62.0));
  // Lixo que se salta sem estourar.
  CHECK(nu::analysa_lrc("").empty());
  CHECK(nu::analysa_lrc("[").empty());
  CHECK(nu::analysa_lrc("[00:").empty());
  CHECK(nu::analysa_lrc("[abc] a").empty());
  CHECK(nu::analysa_lrc("[00:11.00").empty());
}

// A LINHA CORRENTE, com os alvos escriptos. Os limites são o que importa: o
// instante EXACTO de um carimbo pertence a esse carimbo, e não ao anterior.
TEST_CASE("a linha corrente acha-se, e antes da primeira dá menos um") {
  const std::vector<nu::LinhaDaLetra> linhas = {
      {10.0, "um"}, {20.0, "dous"}, {30.0, ""}, {40.0, "quatro"}};
  CHECK(nu::linha_corrente(linhas, 0.0) == -1);
  CHECK(nu::linha_corrente(linhas, 9.999) == -1);
  // O instante EXACTO pertence ao carimbo, e não ao anterior.
  CHECK(nu::linha_corrente(linhas, 10.0) == 0);
  CHECK(nu::linha_corrente(linhas, 19.999) == 0);
  CHECK(nu::linha_corrente(linhas, 20.0) == 1);
  CHECK(nu::linha_corrente(linhas, 29.999) == 1);
  // O carimbo de texto vazio é linha, e vale: é elle que limpa a tela.
  CHECK(nu::linha_corrente(linhas, 30.0) == 2);
  CHECK(nu::linha_corrente(linhas, 39.999) == 2);
  CHECK(nu::linha_corrente(linhas, 40.0) == 3);
  // Depois da ultima, fica na ultima: a musica acabou e o verso final fica.
  CHECK(nu::linha_corrente(linhas, 4000.0) == 3);
  // Degenerescencias: lista vazia, e posição que não é numero.
  CHECK(nu::linha_corrente({}, 10.0) == -1);
  CHECK(nu::linha_corrente(linhas,
                           std::numeric_limits<double>::quiet_NaN()) == -1);
  // Lista de UMA linha, que é onde a busca binaria degenera.
  const std::vector<nu::LinhaDaLetra> uma = {{5.0, "so"}};
  CHECK(nu::linha_corrente(uma, 4.999) == -1);
  CHECK(nu::linha_corrente(uma, 5.0) == 0);
  CHECK(nu::linha_corrente(uma, 500.0) == 0);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
