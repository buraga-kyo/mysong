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

// saneia_nome — o nome que se ha de pôr no systema de arquivos. Tira a barra, o
// NUL e o ponto inicial; apara os espaços das pontas; e corta o comprimento, que
// o ext4 não aceita nome de mais de duzentos e cincoenta e cinco octetos. Nome
// que se reduza a nada devolve «sem titulo», que é resposta e não erro: arquivo
// sem nome não se pode gravar.
std::string saneia_nome(std::string_view crua);

// O DESFECHO de uma aquisição. Toda falha tem nome, porque «falhou» não diz ao
// operador se ha de tentar outra vez, corrigir a URL ou installar o yt-dlp.
enum class Colheita {
  Colhido,          // o arquivo está no logar, com as etiquetas
  SemFerramenta,    // o yt-dlp não está no caminho
  UrlRecusada,      // o yt-dlp não conseguiu ler a URL
  JaExiste,         // ha arquivo no destino; NADA se tocou
  FalhouAoBaixar,   // o yt-dlp sahiu com erro
  FalhouAEtiqueta,  // baixou-se, mas a etiqueta não se pôde escrever
};

// A ETIQUETA que a sonda da URL colheu. Campo vazio quer dizer que a rede não o
// soube dizer, e não que elle seja vazio.
struct EtiquetaRemota {
  std::string titulo;
  std::string canal;
  std::string artista;
  std::string album;
  int numero = 0;
  int duracao = 0;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
