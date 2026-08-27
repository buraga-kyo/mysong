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

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
