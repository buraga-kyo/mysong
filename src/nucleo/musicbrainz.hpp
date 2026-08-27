// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MUSICBRAINZ — src/nucleo/musicbrainz.hpp
// ══════════════════════════════════════════════════════════════════════════
// A GRAVAÇÃO por traz da faixa (issue #57). O catalogo da issue #13 casava o
// audio por titulo e duração, crivo que acceita cover e versão ao vivo; o que
// separa a gravação certa de toda imitação é o ISRC, e quem o dá de graça é o
// MusicBrainz: dados CC0, API ws/2 sem chave, sem conta e sem credencial
// alguma. Foi essa a ordem d'esta obra, e é essa a fronteira.
//
// O CAMINHO: o id do track (que a pagina de embutir já tras) acha a gravação
// pela relação de URL; não havendo o link, a busca por artista, titulo e
// duração; e a ficha da gravação dá ISRC, album, anno, numero e duração exacta.
//
// DOMÍNIO ......... o id de um track do Spotify, ou artista+titulo+duração; e
//                   os corpos JSON que o ws/2 devolve.
// CONTRA-DOMÍNIO .. uma FichaMB, ou o aviso honesto de que não se achou.
// INVARIANTE ...... NUNCA mais de uma requisição por segundo, somados todos os
//                   fios, e toda requisição se nomeia pelo agente d'esta obra:
//                   são as duas regras publicadas do servidor, e quebrá-las é
//                   ser recusado (503) em nome de todos os que pedem d'este IP.
// Q.E.D. .......... sendo puras a construcção das consultas e a leitura das
//                   respostas, a bateria afere-as contra corpos colhidos da
//                   API viva em 2026-08-27, sem tocar a rede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// O AGENTE. O MusicBrainz exige `nome/versão (contato)` e recusa o anonymo.
inline constexpr char kAgenteDoMB[] =
    "mysong/0.1 (https://github.com/bragaus/mysong)";

// A FICHA de uma gravação. Campo vazio ou zero é «o MusicBrainz não disse».
struct FichaMB {
  std::vector<std::string> isrcs;  // na ordem do MB; uma gravação accumula varios
  std::string titulo;
  std::string artista;
  std::string album;   // da release canonica: Official, grupo Album puro, a mais antiga
  int ano = 0;         // o anno d'essa release
  int numero = 0;      // a posição da faixa n'ella (`position`; `number` vem «A3»)
  int duracao_ms = 0;  // a duração exacta da gravação, em milesimos
};

// escapa_url — o percent-encoding do RFC 3986, proprio porque o do curl pede
// punho vivo. Sómente letra, cifra e `-._~` passam crus.
std::string escapa_url(std::string_view crua);

// url_da_consulta_pelo_link — a relação de URL do track do Spotify: casamento
// EXACTO quando o MusicBrainz tem o link. Vazio sem identificador.
std::string url_da_consulta_pelo_link(std::string_view id_do_track);

// url_da_consulta_pela_busca — gravação por artista, titulo e duração (janella
// de doze segundos, a da tolerancia do casamento). Vazio sem titulo; sem
// artista ou sem duração, a clausula que falta fica de fóra.
std::string url_da_consulta_pela_busca(std::string_view artista,
                                       std::string_view titulo, int duracao_ms);

// url_da_ficha — a gravação inteira por MBID: ISRCs, artistas e releases com
// grupo e numeração. Vazio sem MBID.
std::string url_da_ficha(std::string_view mbid);

// le_gravacao_da_url — o MBID da gravação que o url-lookup aponta. Vazio quando
// o corpo não tras relação de gravação (404, truncado, alheio).
std::string le_gravacao_da_url(std::string_view corpo);

// le_ficha_da_gravacao — a ficha que o corpo do lookup tras: ISRCs na ordem do
// MB, titulo, artista, duração exacta, e a release canonica (album, anno,
// numero). Campo que o corpo não diga fica vazio ou zero.
FichaMB le_ficha_da_gravacao(std::string_view corpo);

// le_eleita_da_busca — o MBID eleito do corpo da busca: score ≥ 90, duração
// dentro da janella quando o catalogo a disse, e a de first-release-date mais
// antiga entre as que passam. Vazio quando nenhuma passa.
std::string le_eleita_da_busca(std::string_view corpo, int duracao_ms);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
