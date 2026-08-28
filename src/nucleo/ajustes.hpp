// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DOS AJUSTES — src/nucleo/ajustes.hpp
// ══════════════════════════════════════════════════════════════════════════
// O que o operador ajusta n'esta obra, e de ONDE cada ajuste veio. O arquivo
// d'elle entra aqui, e d'aqui sahe RESOLVIDO: quem consome não pergunta ao
// ambiente, não abre arquivo e não escolhe padrão algum.
//
// O PROGRAMA NUNCA ESCREVE O ARQUIVO: o commentario que o operador poz lá não
// morre n'uma reescrita nossa.
//
// DOMÍNIO ......... o TEXTO do arquivo, o que o ambiente diz, e o que a linha
//                   de commando trouxe. Nunca o disco directamente.
// CONTRA-DOMÍNIO .. os quatro ajustes, cada um com a sua ORIGEM, mais a lista
//                   das queixas que o --sonda mostra.
// INVARIANTE ...... valor e origem vivem no MESMO typo, donde não podem
//                   divergir; e queixa alguma tranca a porta da obra.
// Q.E.D. .......... a origem acompanha o valor, donde o --sonda diz de onde
//                   veio cada ajuste, e o arquivo depura-se sem se ler codigo.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "nucleo/estaleiro.hpp"  // OBREIROS_DA_BAIXA, e por elle o Fonte

namespace mysong::nucleo {

// A ORIGEM de um ajuste, na ordem em que vencem: o argumento da linha de
// commando ganha da variavel de ambiente, que ganha do arquivo, que ganha do
// padrão da Casa. A ordem da declaração É a da precedencia.
enum class Origem { Argumento, Ambiente, Arquivo, Padrao };

// nome_da_origem — a palavra que o --sonda escreve. Vive aqui, e não na tela,
// pela razão do nome_da_fonte: origem nova sem nome não compila.
std::string_view nome_da_origem(Origem origem);

// UM ajuste: o valor, e de onde elle veio, no mesmo typo. Em campos separados,
// alguem poria o valor e esqueceria a origem, e o --sonda mentiria.
template <class T>
struct Ajuste {
  T valor{};
  Origem origem = Origem::Padrao;
};

// O ESTADO do arquivo. Ausente e Illegivel NÃO são o mesmo caso: ausente é o
// caso normal e cala-se; presente que se não lê é queixa.
enum class EstadoDoArquivo { Ausente, Lido, Illegivel };

// O VOLUME de fabrica. Applica-se SEMPRE ao abrir, donde é este o numero que
// vale, e não o do tocador: uma verdade, e não duas a divergirem com o tempo.
inline constexpr int VOLUME_DA_CASA = 100;

// O TECTO das baixas simultaneas. Oito, e não sem tecto: zero obreiro é fila
// que nunca anda, e o tractado do estaleiro já declara que a rede é uma só;
// numero sem tecto seria fio de systema por conta de erro de dedo.
inline constexpr int BAIXAS_NO_MAXIMO = 8;

// Os tectos do ARQUIVO, que existem para que lixo não vire relatorio infinito:
// arquivo binario passado por engano, linha de um megabyte, queixa por byte.
inline constexpr std::size_t LINHA_NO_MAXIMO = 4096;
inline constexpr std::size_t ARQUIVO_NO_MAXIMO = 1024 * 1024;
inline constexpr std::size_t QUEIXAS_NO_MAXIMO = 32;

// OS AJUSTES em vigor, já resolvidos: os quatro que o operador governa, o
// caminho do arquivo que se considerou (ainda que ausente), o estado d'elle, e
// as queixas. O acervo nasce vazio porque o padrão d'elle depende do HOME, que
// é do mundo e não d'este cabeçalho.
struct Ajustes {
  Ajuste<std::filesystem::path> acervo;
  Ajuste<int> volume{VOLUME_DA_CASA, Origem::Padrao};
  Ajuste<Fonte> fonte_da_busca{Fonte::YouTube, Origem::Padrao};
  Ajuste<std::size_t> baixas_simultaneas{OBREIROS_DA_BAIXA, Origem::Padrao};

  std::filesystem::path arquivo;
  EstadoDoArquivo estado = EstadoDoArquivo::Ausente;
  std::vector<std::string> queixas;

  // queixa — accrescenta uma queixa, até o tecto. Passado o tecto, cala-se e
  // deixa UMA linha a dizer quantas ficaram de fóra.
  void queixa(std::string dito);
};

// Um PAR do arquivo: o numero da linha, a chave e o valor, já aparados. O
// numero guarda-se porque queixa que não diz a linha é queixa que o operador
// não sabe onde corrigir.
struct Par {
  std::size_t linha = 0;
  std::string chave;
  std::string valor;
};

// aparar e corta_commentario — as duas partidas de uma linha. Sahem do namespace
// anonymo por serem DECLARADAS aqui, pela razão do nomeado_na_forcagem da sonda:
// a bateria prova-as uma a uma, e não sómente por dentro do leitor.
std::string_view aparar(std::string_view texto);
std::string_view corta_commentario(std::string_view linha);

// ler_pares — o LEITOR. Recebe o TEXTO do arquivo, e nunca um caminho: é d'isto
// que vem a bateria provar o formato inteiro sem tocar em disco. Devolve os
// pares na ORDEM em que vieram, e as queixas ficam nos ajustes.
std::vector<Par> ler_pares(std::string_view texto, Ajustes* ajustes);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
