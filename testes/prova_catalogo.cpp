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

TEST_CASE("o id do track sahe do uri, e uri que não é de track dá id vazio") {
  const nu::Catalogo lida = nu::le_catalogo(kPagina);
  REQUIRE(lida.faixas.size() == 2);
  CHECK(lida.faixas[0].id_do_track == "7bxa");
  CHECK(lida.faixas[1].id_do_track == "20jb");
  // Episodio de podcast: linha legitima do catalogo, mas gravação que não se pode
  // casar. O id fica vazio, e a faixa fica; é o vazio que manda a baixa á busca.
  constexpr char kEpisodio[] =
      R"({"trackList":[{"uri":"spotify:episode:abc1","title":"Um Episodio",)"
      R"("subtitle":"Alguem","duration":1000}]})";
  const nu::Catalogo mixto = nu::le_catalogo(kEpisodio);
  REQUIRE(mixto.faixas.size() == 1);
  CHECK(mixto.faixas[0].id_do_track.empty());
  CHECK(mixto.faixas[0].titulo == "Um Episodio");
}

TEST_CASE("corpo sem lista dá catalogo vazio, e não erro") {
  CHECK(nu::le_catalogo("").faixas.empty());
  CHECK(nu::le_catalogo("<html>nada</html>").faixas.empty());
  CHECK(nu::le_catalogo("{\"trackList\":[]}").faixas.empty());
  // Arranjo TRUNCADO: nada sahe. Rede que corta a pagina a meio é caso de todo dia.
  CHECK(nu::le_catalogo("{\"trackList\":[{\"title\":\"Meia\"").faixas.empty());
  CHECK(nu::nome_da_lista("<html>sem lista</html>").empty());
}

TEST_CASE("o casamento recusa quem está fóra da tolerancia") {
  nu::Pedido pedido;
  pedido.titulo = "Loser";
  pedido.artista = "Tame Impala";
  pedido.duracao = 223;

  // Tres achados: um dentro da tolerancia, um de dez minutos, e um de um minuto.
  const std::vector<nu::Achado> achados = {
      faz("Tame Impala - Loser (Official Audio)", 225),
      faz("Loser - EXTENDED MIX 10 HOURS", 600),
      faz("Loser (snippet)", 61),
  };
  CHECK(nu::melhor_achado(achados, pedido, nu::TOLERANCIA_DO_CASAMENTO) == 0);

  // Sómente os de fóra: NENHUM casa, e a faixa sahe duvidosa. É o crivo que impede
  // baixar mistura de dez minutos por faixa de tres.
  const std::vector<nu::Achado> longe = {faz("Loser 10 HOURS", 600),
                                         faz("Loser (snippet)", 61)};
  CHECK(nu::melhor_achado(longe, pedido, nu::TOLERANCIA_DO_CASAMENTO) == -1);
  CHECK(nu::melhor_achado({}, pedido, nu::TOLERANCIA_DO_CASAMENTO) == -1);
}

TEST_CASE("entre os que passam a duração, ganha quem tras o titulo") {
  nu::Pedido pedido;
  pedido.titulo = "Loser";
  pedido.duracao = 223;
  // O PRIMEIRO está mais proximo em duração, e não tras o titulo; o segundo está um
  // segundo mais longe, e tras. Ganha o segundo: o crivo do titulo separa a faixa
  // certa da vizinha de egual comprimento, que é o que mais engana.
  const std::vector<nu::Achado> achados = {
      faz("Outra cousa qualquer", 223),
      faz("Tame Impala - LOSER (audio)", 224),
  };
  CHECK(nu::melhor_achado(achados, pedido, nu::TOLERANCIA_DO_CASAMENTO) == 1);

  // Não passando nenhum o crivo do titulo, ganha o de duração mais proxima.
  const std::vector<nu::Achado> sem_titulo = {faz("Alheia", 230),
                                              faz("Outra alheia", 224)};
  CHECK(nu::melhor_achado(sem_titulo, pedido, nu::TOLERANCIA_DO_CASAMENTO) == 1);
}

TEST_CASE("pedido SEM duração não casa com ninguem") {
  nu::Pedido pedido;
  pedido.titulo = "Loser";
  pedido.duracao = 0;  // o catalogo não disse
  const std::vector<nu::Achado> achados = {faz("Loser", 223), faz("Loser", 224)};
  // Menos um, e não zero. Sem duração não ha crivo algum, e a tarefa manda marcar
  // por duvidosa em vez de baixar cousa errada calada.
  CHECK(nu::melhor_achado(achados, pedido, nu::TOLERANCIA_DO_CASAMENTO) == -1);
  // E o caso que mostra por que a guarda presta: achado de CINCO segundos. Sem a
  // guarda, a distancia d'elle a uma duração desconhecida sahe cinco, que cabe na
  // tolerancia, e baixava-se um trecho de cinco segundos por faixa inteira.
  CHECK(nu::melhor_achado({faz("Loser", 5)}, pedido,
                          nu::TOLERANCIA_DO_CASAMENTO) == -1);
  // E achado sem duração tambem não se pode crivar: fica de fóra.
  pedido.duracao = 223;
  CHECK(nu::melhor_achado({faz("Loser", 0)}, pedido,
                          nu::TOLERANCIA_DO_CASAMENTO) == -1);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
