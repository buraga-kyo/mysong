// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA MARCA, src/nucleo/marca.hpp
// ══════════════════════════════════════════════════════════════════════════
// Declara a MARCA do programa: o nome pelo qual esta obra se annuncia a quem
// a invoca. É a primeira matéria do núcleo, e a única de que ele se occupa
// neste estado da casa. Não é titulo de canção, nem rótulo de janella: é o
// nome do proprio tocador, e por isso mora no núcleo e não na tela.
//
// DOMÍNIO ......... o vacuo. A marca não se calcula de nada; é constante da
//                   obra, decretada pelo auctor e não pelo ambiente.
// CONTRA-DOMÍNIO .. um std::string_view de caracteres ASCII imprimiveis,
//                   nunca vazio, nunca terminado em espaço.
// INVARIANTE ...... a vista devolvida aponta para armazenamento de duração
//                   estática; sobrevive a todo o programa e a ninguem pertence,
//                   de sorte que o chamador jamais a liberta. Duas invocações
//                   devolvem o mesmo texto, byte por byte.
// Q.E.D. .......... havendo uma só fonte da verdade do nome, tela e bateria de
//                   provas observam o MESMO texto; discordancia entre o que se
//                   mostra e o que se afirma torna-se impossivel por
//                   construcção, e não por vigilancia.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string_view>

namespace mysong::nucleo {

// A marca do programa. Constante da obra; jamais falha.
std::string_view marca() noexcept;

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//, Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
