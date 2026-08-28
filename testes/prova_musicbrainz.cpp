// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO MUSICBRAINZ — testes/prova_musicbrainz.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. Os corpos são recortes VERBATIM das
// respostas vivas do ws/2 colhidas em 2026-08-27 (gravação 8f3471b5, a de
// 1987), aparados aos campos que os leitores lêem e ao embrulho que os engana.
// O unico caso que toca o mundo toca RELOGIO: é o do acelerador.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>

#include "nucleo/musicbrainz.hpp"

namespace nu = mysong::nucleo;

namespace {

// O url-lookup que ACHOU o link (200): a relação tras a gravação apontada.
constexpr char kCorpoDoLink[] =
    R"({"relations":[{"source-credit":"","recording":{"length":212946,)"
    R"("video":false,"title":"Never Gonna Give You Up","disambiguation":"",)"
    R"("id":"8f3471b5-7e6a-48da-86a9-c1c07a0f47ae"},"type":"free streaming",)"
    R"("direction":"backward","target-type":"recording","ended":false}],)"
    R"("resource":"https://open.spotify.com/track/1Ojc3QD0dfJ5HG8uzLsfTg",)"
    R"("id":"838f2859-5383-47c3-9e8f-b00286a41180"})";

// O url-lookup que NÃO achou (404): resposta prevista, e não erro.
constexpr char k404[] =
    R"({"help":"For usage, please see: https://musicbrainz.org/development)"
    R"(/mmd","error":"Not Found"})";

}  // namespace

TEST_CASE("do url-lookup sahe o MBID da gravação, e do 404 sahe vazio") {
  CHECK(nu::le_gravacao_da_url(kCorpoDoLink) ==
        "8f3471b5-7e6a-48da-86a9-c1c07a0f47ae");
  CHECK(nu::le_gravacao_da_url(k404).empty());
  // Corpo TRUNCADO a meio da relação: vazio, e não gravação meio lida.
  const std::string meio = std::string(kCorpoDoLink).substr(0, 90);
  CHECK(nu::le_gravacao_da_url(meio).empty());
}

namespace {

// A ficha da gravação de 1987, recortada da resposta viva: as RELEASES á frente
// do credito, como o MB de facto serializou; e tres releases que fazem a eleição
// trabalhar: a compilação mais antiga crua («¡Boom! 3», 1987, credito Various
// Artists), o single de 1987, e o album de estudo («Whenever You Need Somebody»,
// 1987-10-01, faixa 1), que é a resposta canonica.
constexpr char kCorpoDaFicha[] =
    R"({"length":212946,"title":"Never Gonna Give You Up","video":false,)"
    R"("id":"8f3471b5-7e6a-48da-86a9-c1c07a0f47ae","releases":[)"
    R"j({"title":"¡Boom! 3 (El disco de los exitos)","date":"1987",)j"
    R"("status":"Official","artist-credit":[{"name":"Various Artists"}],)"
    R"("release-group":{"primary-type":"Album","releases":[],)"
    R"("secondary-types":["Compilation"],"first-release-date":"1987"},)"
    R"("media":[{"format":"12\" Vinyl",)"
    R"("tracks":[{"number":"A3","position":3,"length":212000}]}]},)"
    R"({"title":"Never Gonna Give You Up","date":"1987","status":"Official",)"
    R"("release-group":{"primary-type":"Single","secondary-types":[],)"
    R"("first-release-date":"1987-07-27"},"media":[{"tracks":[)"
    R"({"number":"3","position":3}]}]},)"
    R"({"title":"Whenever You Need Somebody","date":"1987-10-01",)"
    R"("status":"Official","country":"XE","release-group":{)"
    R"("primary-type":"Album","secondary-types":[]},"media":[{"format":"CD",)"
    R"("track-count":10,"tracks":[{"number":"1","position":1,"length":215733}]}]}],)"
    R"("isrcs":["GB5KW2103369","GB5KW2202504","GBARL0401372","GBARL0600786",)"
    R"("GBARL0600789","GBARL8700052","GBARL9300135","USAT21601138"],)"
    R"("artist-credit":[{"name":"Rick Astley","joinphrase":""}]})";

}  // namespace

TEST_CASE("a ficha sahe com os ISRCs na ordem e a release canonica") {
  const nu::FichaMB ficha = nu::le_ficha_da_gravacao(kCorpoDaFicha);
  REQUIRE(ficha.isrcs.size() == 8);
  CHECK(ficha.isrcs.front() == "GB5KW2103369");
  CHECK(ficha.isrcs[5] == "GBARL8700052");
  CHECK(ficha.titulo == "Never Gonna Give You Up");
  // Rick Astley, e NÃO Various Artists: as releases vêm á frente do credito
  // n'este corpo, como no vivo, e a primeira d'ellas é compilação de varios.
  CHECK(ficha.artista == "Rick Astley");
  CHECK(ficha.duracao_ms == 212946);
  // A compilação e o single são os mais antigos crus; a canonica é o album.
  CHECK(ficha.album == "Whenever You Need Somebody");
  CHECK(ficha.ano == 1987);
  CHECK(ficha.numero == 1);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
