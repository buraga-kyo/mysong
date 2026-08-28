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

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
