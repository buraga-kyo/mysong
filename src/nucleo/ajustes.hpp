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

#include <string_view>

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

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
