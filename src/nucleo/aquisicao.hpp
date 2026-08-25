// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA AQUISIÇÃO — src/nucleo/aquisicao.hpp
// ══════════════════════════════════════════════════════════════════════════
// A UNICA camada d'esta obra que toca a rede. Chama o `yt-dlp`, guarda o audio na
// hierarchia do acervo, e escreve as etiquetas ELLA MESMA, com a taglib.
//
// Porque a etiqueta se escreve aqui, e não se deixa ao yt-dlp: medido na issue
// #34 sobre um acervo de verdade, o `yt-dlp --embed-metadata` grava `artist` com
// o nome do CANAL e `title` com o titulo do video inteiro. Quem baixa Bach de um
// canal chamado «Public Domain Classical Music» fica com esse por artista, e o
// acervo passa a estar organizado por canal de YouTube. Esta Casa não aceita
// isso: o operador diz o artista, e o que se grava é o que elle disse.
//
// DOMÍNIO ......... uma URL, e o que o operador quiser dizer sobre a faixa.
// CONTRA-DOMÍNIO .. um arquivo de audio no logar certo, com as etiquetas certas.
// INVARIANTE ...... nome algum vindo da rede chega ao systema de arquivos sem
//                   passar pelo saneamento: barra, NUL e ponto inicial saem.
//                   Baixar NUNCA sobrescreve arquivo que já exista.
// Q.E.D. .......... sendo puras a construcção do caminho, o saneamento e a lista
//                   de argumentos, a bateria afere o que se HA DE correr sem
//                   correr cousa alguma, e sem tocar a rede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// O que se sabe de uma faixa que se vae baixar. Campo vazio quer dizer «não sei»,
// e ahi vale o que a sonda da URL tiver dito.
struct Pedido {
  std::string url;
  std::string artista;
  std::string album;
  std::string titulo;
  int numero = 0;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
