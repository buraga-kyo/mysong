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
#include <mutex>
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
  // A ORDEM PROPRIA d'esta faixa na vista plana do acervo, explicita e
  // contigua. Negativa quer dizer «ainda sem logar»: o Escriba dá-lhe o FIM da
  // fila. É por isso que faixa nova entra no fim sem quem a grava o dizer.
  std::int64_t ordem = -1;
};

// A VERSÃO do esquema. Sobe quando o esquema muda de forma, e serve a UMA
// decisão: banco de versão MAIOR que esta não se sobrescreve, porque sómente um
// mysong mais novo o pode ter lavrado, e rebaixá-lo por trás do operador
// perderia o que a versão nova enche. Versão MENOR é caso normal: nada se
// reaproveita d'ella, reconstroe-se tudo, e o banco novo sahe nesta versão.
inline constexpr int kVersaoDoEsquema = 2;

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

// A BIBLIOTHECA: o lado que LÊ. Abre o banco em sómente-leitura, e banco
// ausente é resposta vazia e não erro — o acervo que ainda não se varreu é
// caso legitimo, e não avaria. Nenhuma consulta lança pela borda.
//
// TRANCADA POR DENTRO, á maneira do Tocador da issue #50: o socket de commando
// lê-a do fio do relogio, e o reabre() troca o punho do fio da tela. Punho lido
// por um fio emquanto outro o fecha é uso de banco já fechado, e o ponteiro em
// si é corrida que o compilador tem licença de moer.
class Biblioteca {
 public:
  explicit Biblioteca(std::filesystem::path banco);
  ~Biblioteca();

  Biblioteca(const Biblioteca&) = delete;
  Biblioteca& operator=(const Biblioteca&) = delete;

  bool aberta() const noexcept;

  // Fecha e reabre o mesmo caminho. É NECESSARIO, e não commodidade: o Escriba
  // substitue o índice por RENAME, donde o punho aberto continua a apontar para
  // o inode antigo e a leitura devolveria para sempre o acervo de antes da
  // varredura. Quem varre chama isto quando a varredura conclue.
  void reabre();

  // A versão do esquema que está em disco. Zero quando não ha banco, ou quando
  // o que ha não tem taboa de versão que se possa ler.
  int versao() const noexcept;

  std::size_t total() const;

  // As tres consultas do aceite. Ordem de cadeia nos nomes; ordem de numero, e
  // depois de titulo, nas faixas de um album.
  std::vector<std::string> artistas() const;
  std::vector<std::string> albuns(std::string_view artista) const;
  std::vector<Faixa> faixas_do_album(std::string_view artista,
                                     std::string_view album) const;

  // Por titulo, e por pedaço de titulo. Ordem de artista, album e numero.
  std::vector<Faixa> busca_faixa(std::string_view termo) const;

  // As duas MUTAÇÕES de UMA linha. Não vão pelo Escriba, e de proposito: elle
  // reconstroe o índice inteiro, que é o que a varredura pede e o que renomear
  // ou apagar UMA faixa não justifica. Abrem punho PROPRIO de escripta, que o
  // d'esta classe é de sómente-leitura e ha de continuar a ser. Falso quando o
  // caminho não está no índice, ou quando o banco não se deixa escrever.
  bool muda_o_titulo(std::string_view caminho, std::string_view titulo);
  bool esquece(std::string_view caminho);

  // O que a varredura pergunta para saber se ha de reler a etiqueta. Falso
  // quando o caminho não está no índice.
  bool acha_por_caminho(std::string_view caminho, Faixa& sahida) const;

 private:
  // A tranca NÃO é reentrante: porta publica alguma d'esta classe chama outra,
  // e a segunda tomada seria abraço de si mesma. O destructor não a toma, que o
  // objecto sobrevive a todos os fios que o lêem: a janella junta-os antes de a
  // pilha se desfazer.
  mutable std::mutex tranca_;
  std::filesystem::path banco_;
  sqlite3* punho_ = nullptr;
};

// O ESCRIBA: o lado que ESCREVE, e que nunca escreve no logar. Lavra n'um
// temporario ao lado do banco, e conclui() renomeia atomicamente por cima do
// antigo. Destruir o Escriba sem concluir DESFAZ o temporario: o caminho do
// abandono não depende de quem chama se lembrar d'elle.
class Escriba {
 public:
  // `limite_de_paginas` é a INJECÇÃO com que a prova simula disco cheio: zero é
  // sem limite, e n > 0 permitte n paginas addicionaes depois do esquema, ao
  // cabo das quaes o SQLite devolve SQLITE_FULL — o mesmo codigo, pelo mesmo
  // caminho, que devolve com o systema de arquivos cheio de verdade.
  Escriba(std::filesystem::path banco, long limite_de_paginas = 0);
  ~Escriba();

  Escriba(const Escriba&) = delete;
  Escriba& operator=(const Escriba&) = delete;

  bool aberto() const noexcept;
  const std::filesystem::path& temporario() const noexcept;

  // Falso quando o SQLite recusou. Quem chama ha de parar, porque insistir
  // sobre um temporario cheio sómente accumula a mesma recusa.
  bool grava(const Faixa& faixa);

  // Renomeia por cima do antigo. Depois d'isto o Escriba está fechado.
  bool conclui();

  // Desiste, e não deixa resto. Chamar duas vezes é innocuo.
  void abandona() noexcept;

 private:
  std::filesystem::path banco_;
  std::filesystem::path temporario_;
  sqlite3* punho_ = nullptr;
  // Quantas faixas entraram SEM logar. Serve para as distinguir umas das outras
  // no fim da fila, que sem isso ficariam todas na mesma ordem provisoria.
  std::int64_t ao_cabo_ = 0;
};

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
