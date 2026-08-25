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

}  // namespace

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
