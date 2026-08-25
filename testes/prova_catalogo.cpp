// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO CATALOGO — testes/prova_catalogo.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. O corpo vae escripto Á MÃO, recortado da
// pagina de embutir de VERDADE, e conserva o embrulho de tres niveis e o objecto
// aninhado dentro de cada faixa: são as duas cousas que um leitor plano erraria.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>
#include "nucleo/aquisicao.hpp"
#include "nucleo/catalogo.hpp"

namespace nu = mysong::nucleo;

namespace {

// Recortado de open.spotify.com/embed/playlist/..., com os campos que interessam e o
// embrulho conservado. A do MEIO não tem titulo: ella não ha de sahir, e a terceira
// ha de continuar a ser a TRES. Está no meio de proposito: posta no fim, contar a
// ordem pelas que sobreviveram daria o mesmo numero e a prova nada provaria.
constexpr char kPagina[] = R"(<html><body>
<script id="__NEXT_DATA__" type="application/json">
{"props":{"pageProps":{"state":{"data":{"entity":{
 "type":"playlist","name":"Today’s Top Hits","uri":"spotify:playlist:37i9",
 "trackList":[
  {"uri":"spotify:track:7bxa","uid":"6d2f","title":"Loser",
   "subtitle":"Tame Impala","isExplicit":true,
   "contentRatings":{"labels":["EXPLICIT"]},"duration":223069,
   "audioPreview":{"format":"MP3_96","url":"https://p.scdn.co/x.mp3"}},
  {"uri":"spotify:track:zzz","title":"","subtitle":"Sem nome","duration":1000},
  {"uri":"spotify:track:20jb","title":"hate that i made you love me",
   "subtitle":"Ariana Grande","duration":197949,
   "audioPreview":{"format":"MP3_96","url":"https://p.scdn.co/y.mp3"}}
 ]}}}}},"page":"/playlist/[id]"}
</script></body></html>)";

nu::Achado faz(const std::string& titulo, int duracao) {
  nu::Achado achado;
  achado.titulo = titulo;
  achado.canal = "Um Canal";
  achado.duracao = duracao;
  achado.url = "https://y/" + titulo;
  return achado;
}

}  // namespace

TEST_CASE("o identificador sahe das tres fórmas que o Spotify dá") {
  CHECK(nu::id_da_playlist(
            "https://open.spotify.com/playlist/37i9dQZF1DXcBWIGoYBM5M") ==
        "37i9dQZF1DXcBWIGoYBM5M");
  // Com o `?si=` que o Spotify pendura em toda ligação que se copia: o
  // identificador acaba no `?`, e não o leva consigo.
  CHECK(nu::id_da_playlist(
            "https://open.spotify.com/playlist/37i9dQZF1DXcBWIGoYBM5M?si=abc") ==
        "37i9dQZF1DXcBWIGoYBM5M");
  CHECK(nu::id_da_playlist("spotify:playlist:37i9dQZF1DXcBWIGoYBM5M") ==
        "37i9dQZF1DXcBWIGoYBM5M");
  CHECK(nu::id_da_playlist(
            "https://open.spotify.com/embed/playlist/37i9dQZF1DXcBWIGoYBM5M") ==
        "37i9dQZF1DXcBWIGoYBM5M");

  // O que NÃO é playlist não dá identificador: album e faixa são outra cousa, e
  // baixar um album como se fosse lista daria lista de uma faixa só.
  CHECK(nu::id_da_playlist("https://open.spotify.com/album/1234").empty());
  CHECK(nu::id_da_playlist("https://open.spotify.com/track/1234").empty());
  CHECK(nu::id_da_playlist("nao sou url").empty());
  CHECK(nu::id_da_playlist("").empty());
  // `playlist` sem SEPARADOR depois não vale: é palavra dentro de outra, e sem esta
  // exigencia `playlistabc` daria o identificador «bc», que é lixo que parece bom.
  CHECK(nu::id_da_playlist("https://exemplo/playlistabc").empty());
  CHECK(nu::id_da_playlist("https://exemplo/playlists/abc").empty());
}

TEST_CASE("a pagina de embutir é a que se pede, e sómente com identificador") {
  CHECK(nu::url_do_embed("abc") ==
        "https://open.spotify.com/embed/playlist/abc");
  CHECK(nu::url_do_embed("").empty());
}

TEST_CASE("a leitura tira as faixas de dentro do embrulho, na ordem da lista") {
  const nu::Catalogo lida = nu::le_catalogo(kPagina);
  CHECK(lida.nome == "Today’s Top Hits");
  // TRES no corpo, e DUAS sahem: a terceira não tem titulo, e sem titulo não se
  // pode buscar cousa alguma.
  REQUIRE(lida.faixas.size() == 2);
  CHECK(lida.faixas[0].titulo == "Loser");
  CHECK(lida.faixas[0].artista == "Tame Impala");
  CHECK(lida.faixas[0].duracao_ms == 223069);
  CHECK(lida.faixas[0].numero == 1);
  CHECK(lida.faixas[1].titulo == "hate that i made you love me");
  // TRES, e não dous: a ordem é a da LISTA, e a do meio, recusada, não empurra
  // numero algum para traz. É por este numero que a etiqueta grava a faixa.
  CHECK(lida.faixas[1].numero == 3);
}

TEST_CASE("o objecto ANINHADO não engana a leitura dos campos da faixa") {
  const nu::Catalogo lida = nu::le_catalogo(kPagina);
  REQUIRE(lida.faixas.size() == 2);
  // A primeira faixa tras `audioPreview` com `url` e `format` DENTRO, e
  // `contentRatings` com um arranjo dentro. Um leitor que buscasse a chave em
  // qualquer fundo apanharia esses; o d'esta Casa lê fundo UM, e por isso o
  // `format` do preview não vira campo da faixa e a duração é a de fóra.
  CHECK(lida.faixas[0].duracao_ms == 223069);
  CHECK(lida.faixas[0].artista == "Tame Impala");
  // E a ORDEM conta as tres, e não as duas que sobreviveram.
  CHECK(lida.faixas[1].numero == 3);
}

TEST_CASE("chave de FUNDO DOUS não vira campo da faixa") {
  // Objecto construido de proposito, e NÃO recortado: o Spotify de hoje não põe
  // `title` dentro de `audioPreview`. Mas a promessa d'esta leitura é o fundo UM, e
  // promessa que a bateria não afere é promessa que se perde na primeira mudança da
  // pagina d'elles.
  //
  // O aninhado vem ANTES do de fóra, e é isso que faz o caso valer: vindo depois,
  // uma leitura que buscasse em qualquer fundo acharia primeiro o de fóra e daria a
  // resposta certa por accidente.
  constexpr char kAninhado[] =
      R"({"trackList":[{"audioPreview":{"title":"De dentro","duration":9999},)"
      R"("title":"De fóra","subtitle":"Alguem","duration":1000}]})";
  const nu::Catalogo lida = nu::le_catalogo(kAninhado);
  REQUIRE(lida.faixas.size() == 1);
  CHECK(lida.faixas[0].titulo == "De fóra");
  CHECK(lida.faixas[0].duracao_ms == 1000);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
