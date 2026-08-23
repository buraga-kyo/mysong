// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA FITA ARROWLINE — src/tui/arrowline.hpp
// ══════════════════════════════════════════════════════════════════════════
// Compõe a FITA: enfiada de segmentos de borda esquerda RETA e ponta AFIADA á
// direita, encaixados um no outro por um glifo de junção.
//
// ADVERTÊNCIA DE FIDELIDADE, que se leia antes de tudo: no RADICAL-OS o
// arrowline não é glifo, é polygono traçado em cairo por shapes.powerline, e o
// angulo da diagonal é grandeza derivada, atan(tip / (h/2)) — cerca de 41,6
// graus da vertical com os tokens vivos. Em terminal de célullas isso não se
// copia: ha o glifo U+E0B0, cujo angulo a fonte já decidiu, e a célulla é
// indivisivel, de sorte que o sobrepôr de -T nem se propõe. É TRADUCÇÃO, e não
// cópia. Não se promette fidelidade geometrica; promettem-se as tres REGRAS do
// systema de desenho que a traducção conserva:
//   (a) seta de UMA direcção: borda esquerda reta, ponta afiada á direita.
//       JAMAIS losango de duas pontas, que é erro pregresso registrado.
//   (b) a côr da seta é a côr do segmento que ella SEGUE: a junção sahe com a
//       tinta do anterior e o fundo do seguinte. É d'isto que a fita parece
//       continua, em vez de rectangulos costurados.
//   (c) o enchimento fica um degrau REBAIXADO em relação á orla.
//
// DOMÍNIO ......... a enfiada de segmentos, cada um com rotulo, fundo e tinta.
// CONTRA-DOMÍNIO .. a sequencia ordenada de PEDAÇOS, inspeccionavel sem
//                   terminal e sem a janella — o que torna a regra (b)
//                   asserção verificavel, em vez de boa intenção.
// INVARIANTE ...... fita de N segmentos emitte exactamente N menos um pedaços
//                   de junção INTERNA. A junção de CAUDA, que remata a fita em
//                   ponta, conta-se á parte e nunca entra n'aquelle N-1.
// Q.E.D. .......... a guarnição do rotulo e o córte por largura ficam com quem
//                   desenha, e não com este primitivo: cortar aqui obrigaria a
//                   cortar no meio de um par tinta/fundo, que é a emenda
//                   visivel que o aceite proscreve.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "tui/tokens.hpp"

namespace mysong::tui {

// Os glifos da fita, da JetBrainsMono Nerd Font. U+E0B0 aponta á dextra;
// U+E0B2, á esquerda. NUNCA os dous no mesmo sentido de fita.
inline constexpr std::string_view kPontaDextra = "";
inline constexpr std::string_view kPontaEsquerda = "";

}  // namespace mysong::tui
