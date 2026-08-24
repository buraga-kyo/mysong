// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA BIBLIOTHECA — src/nucleo/biblioteca.hpp
// ══════════════════════════════════════════════════════════════════════════
// Guarda o ÍNDICE do acervo, e sómente elle: não abre arquivo de audio, não
// conhece taglib, não sabe o que é um directorio de artista. Quem varre o disco
// é a Varredura, que vive ao lado; esta peça sabe de SQLite, e mais nada.
//
// DOMÍNIO ......... um caminho de banco, que ENTRA POR PARÂMETRO, e faixas já
//                   apuradas por quem as apurou.
// CONTRA-DOMÍNIO .. as consultas por artista, por album e por faixa; e, do
//                   outro lado, a escripta de um índice novo.
// INVARIANTE ...... a escripta NUNCA toca o banco em uso. O Escriba lavra
//                   n'um temporario, com journal_mode=DELETE e
//                   synchronous=FULL, e sómente ao cabo renomeia atomicamente
//                   sobre o antigo: donde uma interrupção conserva a imagem
//                   anterior INTEIRA, e jamais uma imagem meia. É o precedente
//                   que o operador já usa em agenda_index.py, e a razão d'elle
//                   é que índice corrompido a meio de escripta é pior que
//                   índice ausente. A leitura abre em SÓMENTE-LEITURA, e banco
//                   ausente é resposta vazia e não erro. Nada sahe por
//                   excepção pela borda, e toda cadeia vai ao banco por
//                   amarração, nunca por concatenação de SQL.
// Q.E.D. .......... entrando o caminho do banco por parâmetro, a bateria inteira
//                   corre em directorio temporario, e nenhuma corrida de prova
//                   pode tocar o índice do operador. Não é commodidade: é o que
//                   impede a prova de corromper o acervo de quem nos usa.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// O punho do SQLite, declarado ADIANTE e no escopo global, que é onde o
// sqlite3.h o declara. Assim este cabeçalho não arrasta o sqlite3.h consigo, e
// quem inclue a bibliotheca não herda uma dependencia que não pediu.
struct sqlite3;

namespace mysong::nucleo {

// A MÁSCARA do que se DEDUZIU do caminho, porque a issue manda registrar a
// dedução e não sómente praticá-la. Zero quer dizer que a etiqueta disse tudo;
// e a consulta que interessa ao operador — que faixas entraram sem etiqueta? —
// é «WHERE deduzido != 0», mais curta que a disjunção de quatro columnas.
enum Deduzido : unsigned {
  kDeduziuNada = 0u,
  kDeduziuArtista = 1u,
  kDeduziuAlbum = 2u,
  kDeduziuTitulo = 4u,
  kDeduziuNumero = 8u,
};

// Uma FAIXA do índice. O `caminho` é a identidade, e é canónico; a `raiz` diz
// de que raiz de acervo ella veio, que é o que permitte mais de uma. O
// `modificado` e o `tamanho` são o que a varredura compara para saber se ha de
// reler a etiqueta, e por isso vivem na linha e não fóra d'ella.
struct Faixa {
  std::string caminho;
  std::string raiz;
  std::string artista;
  std::string album;
  std::string titulo;
  int numero = 0;    // zero é «sem numero», e não faixa zero
  int anno = 0;      // zero é «sem anno»
  int duracao = 0;   // em segundos; zero é «não medida»
  std::int64_t modificado = 0;
  std::int64_t tamanho = 0;
  unsigned deduzido = kDeduziuNada;
};

// A VERSÃO do esquema. Sobe quando o esquema muda de forma, e serve a UMA
// decisão: banco de versão MAIOR que esta não se sobrescreve, porque sómente um
// mysong mais novo o pode ter lavrado, e rebaixá-lo por trás do operador
// perderia o que a versão nova enche. Versão MENOR é caso normal: nada se
// reaproveita d'ella, reconstroe-se tudo, e o banco novo sahe nesta versão.
inline constexpr int kVersaoDoEsquema = 1;

// O DESFECHO de uma escripta. Toda falha tem nome, porque «falhou» não diz a
// quem chama se ha de tentar outra vez, avisar o operador, ou calar-se.
enum class Desfecho {
  Concluido,        // renomeado no logar; o índice novo está em pé
  Abandonado,       // desistiu-se, e o temporario desfez-se
  EsquemaMaisNovo,  // o banco em disco é de um mysong mais novo; NADA se tocou
  ErroDeEscripta,   // o SQLite recusou; o banco anterior ficou como estava
  NaoComecou,       // ainda não se lavrou nada
};

// Saneia uma cadeia para UTF-8 valido, trocando cada byte invalido pelo
// caracter de substituição U+FFFD. Etiqueta suja não custa a faixa: perder
// musica por causa de um byte seria pior que mostrar um losango no nome.
std::string saneia_utf8(std::string_view crua);

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
