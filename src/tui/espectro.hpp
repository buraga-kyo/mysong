// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO DESENHO DO ESPECTRO — src/tui/espectro.hpp
// ══════════════════════════════════════════════════════════════════════════
// Converte as QUANTAS_BANDAS magnitudes que o analisador colhe na FITA de
// barras verticaes da §7.4.9. Aqui não se colhe som: a colheita é do
// src/nucleo/espectro.*, e este manuscripto sómente DESENHA o que ella entrega.
//
// A SPEC QUE MANDA, e que se transcreve porque NÃO É VERSIONADA — mora em
// ~/.config/awesome/DESIGN_SYSTEM.md, §7.4.9, linhas 968 a 970, e diz isto e
// nada mais, verbatim:
//   «Barras verticais (EQ) ou rows de canais (Master, Capture, Front…).»
//   «Barra cheia gradiente vertical v700 → v400; mudo em text_faint.»
// Tres linhas, e d'ellas sahe todo o resto por decisão registrada no ledger.
// Mudando-se a fonte lá, esta cópia não sabe: é o preço de arquivo não
// versionado, e escreve-se aqui para que a divergencia se leia no codigo.
// ADVERTENCIA DE ORIENTAÇÃO, que se leia antes de tudo: os oito blocos U+2581 a
// U+2588 crescem de BAIXO para cima, e o Quadro (como o FTXUI) lê-se de CIMA
// para baixo. Os dous sentidos são OPPOSTOS, e a barra desenhada de cabeça para
// baixo passa em toda prova de CONTAGEM, porque o numero de célullas não muda.
// Por isso a base mora na linha `altura - 1`, e a prova o afirma por INDICE.
//
// DOMÍNIO ......... as magnitudes em [0,1] (mas tolera-se o lixo: negativa,
//                   acima do teto, NaN e infinito), a largura e a altura do
//                   painel em CÉLULLAS, e o estado de mudo.
// CONTRA-DOMÍNIO .. um QUADRO: glifo e tinta por célulla, inspeccionavel sem
//                   terminal e sem janella. D'elle se derivam, sem regra de
//                   desenho propria, o ftxui::Element e a sequencia SGR crua.
// INVARIANTE ...... tres, e o terceiro é o que a §7.4.9 exige.
//                   (i) o Quadro tem EXACTAMENTE altura linhas de largura
//                   célullas: não estoura e não deixa buraco, em largura
//                   alguma. Banda alguma se perde: painel estreito FUNDE por
//                   máximo, e não amostra.
//                   (ii) funcção pura. Estado algum se guarda entre chamadas,
//                   d'onde redimensionar e voltar dá o MESMO Quadro.
//                   (iii) o gradiente é ancorado ao PAINEL, e não á barra: a
//                   tinta da célulla sahe da posição d'ella na collunha, e
//                   JAMAIS da magnitude da banda. Ancorado na barra, uma barra
//                   de uma célulla sahiria em v400, o topo brilhante, no
//                   instante em que a banda está quasi morta, e a côr passaria
//                   a MENTIR sobre o nivel. É tambem o que o bar_meter.lua faz,
//                   que fixa o gradiente á extensão nominal do trilho «pois
//                   independe do valor corrente».
// Q.E.D. .......... côr alguma se escreve por hexadecimal: sahem todas de
//                   tui::tokens, e o gradiente interpola por tokens::mistura,
//                   que a bateria da issue #2 já prova. Terminal sem truecolor
//                   NÃO se sonda e NÃO se degrada, pela mesma decisão que a
//                   fita arrowline tomou quanto á Nerd Font: requisito mínimo
//                   declarado, e não caso a contornar aqui.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "tui/tokens.hpp"

namespace mysong::tui {

// O LIMIAR DO QUENTE. Noventa por cento, e é a régua do bar_meter.lua do
// RADICAL-OS, que trata quente do pct_hot em diante. Maior ou IGUAL: o limiar
// pertence ao quente, e a prova afere os dous lados d'elle.
inline constexpr float LIMIAR_QUENTE = 0.90f;

// Quantos degraus cabem n'uma célulla. Oito, que são os blocos U+2581 a U+2588,
// e não é numero de gosto: é quanto o terminal sabe subdividir uma célulla na
// vertical. D'onde a resolução de uma columna de N célullas é 8N degraus, e é
// contra este 8 que a issue #5 calibrou o seu LIMIAR_DE_ZERO de tres
// centesimos, «um quarto do menor passo que a barra mostra».
inline constexpr int DEGRAUS_POR_CELULA = 8;

// A célulla vazia. Espaço, e não bloco de zero oitavos: bloco de zero oitavos
// não existe na taboada Unicode, e U+2580 é o meio-bloco SUPERIOR, que
// desenharia justamente o contrario do que se quer.
inline constexpr std::string_view kCelulaVazia = " ";

// Uma CÉLULLA do quadro: o glifo que mostra, e a tinta com que se escreve. A
// tinta é TRÍADE, e não token, porque o gradiente interpola ENTRE dous tokens e
// os degraus do meio não têm nome na paleta. Quem não pinta traz `pinta` falso,
// e d'esse se emitte a ordem de repouso em vez de tríade, tal como a fita
// arrowline faz com a côr transparente.
struct Celula {
  std::string glifo = std::string(kCelulaVazia);
  tokens::Triade tinta;
  bool pinta = false;
};

// mesma_tinta — a Triade não tem egualdade propria, e não se lha acrescenta em
// tokens.hpp, que é lavra alheia e já mergeada. Compara-se aqui.
inline bool mesma_tinta(tokens::Triade a, tokens::Triade b) {
  return a.r == b.r && a.g == b.g && a.b == b.b;
}
