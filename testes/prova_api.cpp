// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DA API, BANDA PURA — testes/prova_api.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o jsonzinho e o protocolo em MACHINA SURDA: sem socket, sem barramento,
// sem placa de som e sem arquivo em disco. É onde treze verbos e todo o
// enquadramento se provam de graça; o que de facto precisa de socket mora em
// prova_api_viva.cpp, e são cinco casos, não trinta.
//
// DOMÍNIO ......... cadeias, e sómente cadeias: nomes de faixa que trazem o que
//                   o systema de arquivos permitte, e linhas de mensagem que
//                   podem ser qualquer cousa, inclusive lixo.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... toda resposta d'esta obra é UMA linha. É o que se assere por
//                   find('\n') == npos, e não por inspecção de olho: o
//                   enquadramento de uma-mensagem-por-linha cae inteiro se um
//                   nome de faixa levar mudança de linha crua ao emissor.
// Q.E.D. .......... o dublê aqui NÃO é mais simples que o mundo nas tres cousas
//                   em que o mundo morde: nome de arquivo com aspas e UTF-8, fim
//                   natural da faixa mudando posição E estado na mesma batida, e
//                   ordem que o nucleo recusa. Dublê mais simples que o mundo é
//                   onde o defeito se aloja, e por isso se enumeram as
//                   differenças em vez de se as presumir ausentes.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>

#include "api/jsonzinho.hpp"

namespace {
using mysong::api::analysa;
using mysong::api::escapa;
using mysong::api::Mensagem;
using mysong::api::texto;
using mysong::api::Typo;
}  // namespace

TEST_CASE("o escape nao deixa passar byte que parta o enquadramento") {
  CHECK(escapa("as\"pas") == "as\\\"pas");
  CHECK(escapa("contra\\barra") == "contra\\\\barra");
  CHECK(escapa("linha\nnova") == "linha\\nnova");
  CHECK(escapa("volta\rcarro") == "volta\\rcarro");
  CHECK(escapa("tabu\tlado") == "tabu\\tlado");
  CHECK(escapa(std::string("controle\x01""cru")) == "controle\\u0001cru");
  // O UTF-8 passa INTACTO: o JSON o admitte cru, e transcreve-lo seria inflar a
  // mensagem sem ganho algum de correcção.
  CHECK(escapa("Coração") == "Coração");
  CHECK(escapa("音楽") == "音楽");
}

// O caso que de facto guarda a invariante d'esta Casa. Nome de musica com
// mudanca de linha crua é o que parte o enquadramento de uma-mensagem-por-linha,
// e é o que o mundo tem e o dublê ingenuo nao teria.
TEST_CASE("faixa hostil emittida continua a ser UMA linha") {
  const std::string hostil = "01 - As \"Melhores\"\\ Canções\nde 音楽\x02.flac";
  const std::string emittido = texto(hostil);
  CHECK(emittido.find('\n') == std::string::npos);
  CHECK(emittido.front() == '"');
  CHECK(emittido.back() == '"');
  // E o que sahiu volta a entrar: o escape e o parser são um do outro.
  const Mensagem volta = analysa("{\"faixa\":" + emittido + "}");
  REQUIRE(volta.valida);
  REQUIRE(volta.acha("faixa") != nullptr);
  CHECK(volta.acha("faixa")->typo == Typo::Texto);
  CHECK(volta.acha("faixa")->texto == hostil);
}
// Rejeitar NAO é falhar: «json_malformado» é resposta prevista do contracto, e
// parser permissivo é que seria o risco, porque acceitaria por adivinhação o que
// o contracto não promette.
TEST_CASE("o parser recusa o que sae do subconjunto, e diz por que") {
  const char* torpes[] = {
      "{\"verbo\":",              // truncado no valor
      "naoejson",                 // nem objecto
      "[]",                       // vector, e não objecto
      "{\"verbo\":\"estado\"",    // sem o fecho
      "{\"verbo\":\"esta",        // cadeia sem aspa de fecho
      "{verbo:\"estado\"}",       // chave sem aspas
      "{\"verbo\":\"estado\",}",  // virgula pendurada
      "{\"verbo\":\"\\q\"}",      // escape que o subconjunto nao conhece
      "{\"a\":1,\"a\":2}",        // chave repetida
      "{\"n\":1.2.3}",            // numero mal formado
      "{\"verbo\":\"estado\"} sobra",
  };
  for (const char* torpe : torpes) {
    const Mensagem lida = analysa(torpe);
    CHECK_FALSE(lida.valida);
    CHECK_FALSE(lida.razao.empty());  // razão calada seria o silêncio proibido
  }
}

TEST_CASE("o parser acceita o subconjunto inteiro, e sómente elle") {
  CHECK(analysa("{}").valida);
  CHECK(analysa("  { \"verbo\" : \"estado\" }  ").valida);
  const Mensagem rica = analysa(
      "{\"t\":\"vae\",\"n\":-2.5,\"b\":true,\"z\":null,\"u\":\"\\u00e7\\ud83c\\udfb5\"}");
  REQUIRE(rica.valida);
  CHECK(rica.acha("t")->typo == Typo::Texto);
  CHECK(rica.acha("n")->typo == Typo::Numero);
  CHECK(rica.acha("n")->numero == doctest::Approx(-2.5));
  CHECK(rica.acha("b")->booleano);
  CHECK(rica.acha("z")->typo == Typo::Nulo);
  CHECK(rica.acha("u")->texto == "ç\xF0\x9F\x8E\xB5");  // par de substitutos
  CHECK(rica.acha("naoexiste") == nullptr);
}
// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
