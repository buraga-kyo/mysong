// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS UNIDADES — src/api/unidades.hpp
// ══════════════════════════════════════════════════════════════════════════
// A traducção entre as unidades do NUCLEO e as do MPRIS. Vive em arquivo proprio e
// PURO, sem `libdbus`, e a razão está na issue: é onde se erra, e é o que se ha de
// poder provar em machina surda, sem barramento algum.
//
// As tres divergencias, e nenhuma é de gosto:
//   posição   o nucleo fala em SEGUNDOS de ponto flutuante; o MPRIS em
//             MICROSSEGUNDOS de inteiro de sessenta e quatro bits.
//   volume    o nucleo fala em PORCENTO inteiro de zero a cem; o MPRIS em
//             `double` de zero a um.
//   estado    o nucleo tem Tocando, Pausado e Parado; o MPRIS quer as cadeias
//             `Playing`, `Paused` e `Stopped`, e essas exactas.
//
// DOMÍNIO ......... valores do nucleo, e valores que chegam pelo barramento.
// CONTRA-DOMÍNIO .. os do outro lado, aparados.
// INVARIANTE ...... funcção alguma d'aqui lança, e nenhuma devolve valor fóra do
//                   arco que a especificação do MPRIS admitte.
// Q.E.D. .......... sendo tudo funcção de escalar para escalar, a bateria afere a
//                   traducção inteira sem barramento, sem som e sem terminal.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "nucleo/motor.hpp"

namespace mysong::api {

// Um MICROSSEGUNDO por milionesimo de segundo. Escripto como constante para que o
// numero appareça UMA vez: seis zeros escriptos duas vezes é um zero a menos n'uma
// d'ellas, e é assim que estes defeitos nascem.
inline constexpr std::int64_t kMicrosPorSegundo = 1000000;

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
