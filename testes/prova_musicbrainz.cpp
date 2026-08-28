// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO MUSICBRAINZ — testes/prova_musicbrainz.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. Os corpos são recortes VERBATIM das
// respostas vivas do ws/2 colhidas em 2026-08-27 (gravação 8f3471b5, a de
// 1987), aparados aos campos que os leitores lêem e ao embrulho que os engana.
// O unico caso que toca o mundo toca RELOGIO: é o do acelerador.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <chrono>
#include <string>
#include <thread>

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

namespace {

// O corpo da busca, no embrulho vivo do ws/2: tres homonymas de score cem que
// só a data separa, mais duas ISCAS que a eleição ha de recusar; cada uma é a
// mais antiga de todas, e venceria se a guarda respectiva morresse. E a de 1987
// tras uma release ANINHADA de data alheia, que o leitor de fundo um não lê.
constexpr char kCorpoDaBusca[] =
    R"({"created":"2026-08-27T14:00:00.000Z","count":21,"offset":0,)"
    R"("recordings":[{"id":"8f3471b5-7e6a-48da-86a9-c1c07a0f47ae","score":100,)"
    R"("title":"Never Gonna Give You Up","length":212946,)"
    R"("first-release-date":"1987-07-27","video":null,)"
    R"("releases":[{"title":"Remaster","date":"2024-01-01"}]},)"
    R"({"id":"0efeb239-fa6d-4284-ba83-94c604584809","score":100,)"
    R"("title":"Never Gonna Give You Up","length":213000,)"
    R"("first-release-date":"1998"},)"
    R"({"id":"cd29e7db-6f4b-4b53-8b19-3e0e05b4bda5","score":100,)"
    R"("title":"Never Gonna Give You Up","length":209000,)"
    R"("first-release-date":"2008"},)"
    R"({"id":"ff6c55fc-1111-4222-8333-944444444444","score":85,)"
    R"("title":"Never Gonna Give You Up","length":213000,)"
    R"("first-release-date":"1970"},)"
    R"({"id":"2e7756e5-5555-4666-8777-988888888888","score":100,)"
    R"("title":"Never Gonna Give You Up","length":240000,)"
    R"("first-release-date":"1960"}]})";

}  // namespace

TEST_CASE("a eleição da busca criva score e duração, e elege a mais antiga") {
  // A de 1987 vence: as de 1998 e 2008 passam os crivos mas são mais novas; a
  // de 1970 tem score 85, e a de 1960 está a 27 segundos do pedido. Se qualquer
  // das duas guardas morresse, a isca respectiva ganhava por mais antiga.
  CHECK(nu::le_eleita_da_busca(kCorpoDaBusca, 213000) ==
        "8f3471b5-7e6a-48da-86a9-c1c07a0f47ae");
  // Pedido a cem segundos de tudo: candidata alguma passa, e o vazio manda ao
  // caminho de hoje em vez de casar por casar.
  CHECK(nu::le_eleita_da_busca(kCorpoDaBusca, 100000).empty());
  CHECK(nu::le_eleita_da_busca("", 213000).empty());
}

TEST_CASE("os termos da colheita: tres ISRCs no tecto, e o de hoje por ultimo") {
  // A ficha de oito ISRCs dá TRES termos de ISRC, na ordem do MB, e o termo de
  // hoje (artista e titulo) por derradeiro: é o tecto de RULINGS R5, que poupa
  // cinco buscas de rede por faixa sem degradar a que o primeiro ISRC não acha.
  const nu::FichaMB cheia = nu::le_ficha_da_gravacao(kCorpoDaFicha);
  const std::vector<std::string> termos =
      nu::termos_de_busca(cheia, "Rick Astley", "Never Gonna Give You Up");
  REQUIRE(termos.size() == 4);
  CHECK(termos[0] == "GB5KW2103369");
  CHECK(termos[1] == "GB5KW2202504");
  CHECK(termos[2] == "GBARL0401372");
  CHECK(termos[3] == "Rick Astley Never Gonna Give You Up");

  // Ficha sem ISRC algum: sómente o termo de hoje, que sahe com a duvida
  // confessada por quem chama. E sem artista, o termo é o titulo sósinho.
  const nu::FichaMB vazia;
  const std::vector<std::string> sos =
      nu::termos_de_busca(vazia, "", "Never Gonna Give You Up");
  REQUIRE(sos.size() == 1);
  CHECK(sos[0] == "Never Gonna Give You Up");
}

TEST_CASE("duas passagens pelo acelerador distam um segundo, de fios distinctos") {
  // Os dous obreiros do estaleiro chegam em rajada; a promessa é UMA requisição
  // por segundo somados todos os fios. Toca RELOGIO, e não rede: o segundo que
  // este caso custa á bateria é o preço declarado da promessa (PLAN, Riscos).
  using relogio = std::chrono::steady_clock;
  relogio::time_point a, b;
  std::thread um([&a] { nu::espera_a_vez_do_mb(); a = relogio::now(); });
  std::thread dous([&b] { nu::espera_a_vez_do_mb(); b = relogio::now(); });
  um.join();
  dous.join();
  const auto entre = a < b ? b - a : a - b;
  // Novecentos e noventa, e não mil crus: o carimbo toma-se dentro da tranca e o
  // relogio lê-se fóra, e a folga de dez milesimos paga essa fresta sem deixar
  // passar cadencia quebrada, que erraria por um segundo inteiro.
  CHECK(entre >= std::chrono::milliseconds(990));
}

TEST_CASE("o agente do MB nomeia a obra: nome, versão e contato") {
  // O formato é a politica publicada do MusicBrainz: `nome/versão (contato)`.
  // Agente anonymo é recusado pelo servidor, e recusado em nome do IP inteiro.
  const std::string agente = nu::kAgenteDoMB;
  REQUIRE(agente.rfind("mysong/", 0) == 0);
  const std::size_t espaco = agente.find(' ');
  REQUIRE(espaco != std::string::npos);
  for (const char c : agente.substr(7, espaco - 7))
    CHECK((c == '.' || (c >= '0' && c <= '9')));
  CHECK(espaco > 7);                   // ha versão entre a barra e o espaço
  CHECK(agente[espaco + 1] == '(');
  CHECK(agente.back() == ')');
  CHECK(agente.size() > espaco + 3);   // e ha contato entre os parentheses
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
