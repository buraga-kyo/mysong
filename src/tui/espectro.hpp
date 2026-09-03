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

// ── OS REGISTROS. Quatro familias, e a côr diz QUAL d'ellas sôa. Diga-se com
// honestidade o que é: a côr vem do REGISTRO, que é a faixa de hertz onde a
// familia mora, e NÃO de instrumento reconhecido. Separar instrumentos de
// verdade pede modelo de separação de fontes, que não corre em tempo real
// dentro d'um tocador de terminal, e vender o que não ha seria mentir na tela.
enum class Registro { Graves, MediosGraves, MediosAgudos, Agudos };

// As tres fronteiras, em HERTZ e não em indice de banda: mudando-se
// QUANTAS_BANDAS, a familia continua onde estava, que a physica não se mexe com
// o numero de columnas. São as da mesa de som: o bumbo e o baixo até 250, a
// caixa, a guitarra e o corpo da voz até 1 k, a voz, a presença e os teclados
// até 4 k, e os pratos, o chimbal e o ar d'ahi para cima. Fronteira FECHADA em
// cima: 250 Hz ainda é grave, e 250,1 já não.
inline constexpr float FRONTEIRA_DOS_GRAVES = 250.0f;
inline constexpr float FRONTEIRA_DOS_MEDIOS_GRAVES = 1000.0f;
inline constexpr float FRONTEIRA_DOS_MEDIOS_AGUDOS = 4000.0f;

// registro_da_banda — o registro em que cae o CENTRO da banda. Funcção pura, e é
// a UNICA regra de pertença d'esta obra: quem quiser saber a familia de uma
// banda pergunta aqui, e jamais conta indices por fóra.
Registro registro_da_banda(float centro_em_hertz);

// tinta_do_registro — a côr do registro, e devolve o TOKEN e não a tríade
// porque o gradiente compõe por tokens::mistura, que pede o hexadecimal do
// design system. Violeta v500 nos graves, cyan data5 nos medios-graves, laranja
// data3 nos medios-agudos e amarello data2 nos agudos: é o Postulado do Poente
// Contido, que reserva o amarello, o laranja e o cyan á série de dados, e o
// espectro É uma série de dados.
std::string_view tinta_do_registro(Registro registro);

// nome_do_registro — o rótulo em caixa alta, para a legenda que o olho lê. Mora
// aqui, ao pé da côr, para que nome e tinta tenham UMA verdade só: legenda que
// se escrevesse no exemplo divergiria da tela no dia em que a côr mudasse.
std::string_view nome_do_registro(Registro registro);

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

// O QUADRO: a fita já resolvida em célullas. A LINHA 0 É O TOPO, e a base mora
// em `altura - 1`: é a orientação do FTXUI, e não a dos blocos. Guarda-se em
// vector chato, e não em vector de vectores, para que a fórma seja UMA asserção
// (largura * altura) em vez de altura asserções.
struct Quadro {
  std::size_t largura = 0;
  std::size_t altura = 0;
  std::vector<Celula> celulas;

  // em — a célulla da linha e da collunha. Fóra de limite devolve célulla vazia,
  // e não estoura: assim a prova pode varrer largura + 1 sem armar guarda.
  const Celula& em(std::size_t linha, std::size_t collunha) const;
};

// oitavos — a magnitude em DEGRAUS, e é a conta que se prova exacta. Devolve
// inteiro em [0, DEGRAUS_POR_CELULA * altura]. Lixo entra e não sahe: negativa
// vale zero, acima do teto vale o teto, e NÃO FINITA vale zero. Esta ultima
// escreve-se ANTES do cingir, e de propósito: comparação com NaN é sempre
// falsa, d'onde um cingir escripto ingenuamente deixaria o NaN passar ao floor.
int oitavos(float magnitude, std::size_t altura);

// glifo_do_degrau — o bloco de k oitavos, em U+2580 + k. Zero dá a célulla
// vazia; acima de oito cinge-se a oito, que é o bloco cheio.
std::string glifo_do_degrau(int degrau);

// tinta_da_linha — o GRADIENTE, ancorado ao PAINEL. `desde_a_base` conta da
// base para cima, de sorte que zero dá v700 EXACTO e `altura - 1` dá v400
// EXACTO. Painel de uma célulla só dá v700, a base, que é d'onde a §7.4.9
// ancora a rampa. Não recebe magnitude alguma, e é n'isto que o invariante
// (iii) se torna estructural em vez de boa intenção.
tokens::Triade tinta_da_linha(std::size_t desde_a_base, std::size_t altura);

// compor — o QUADRO. Não guarda estado: as mesmas bandas na mesma largura dão o
// mesmo quadro, hoje e depois de dez redimensionamentos. Não presume que as
// bandas sejam QUANTAS_BANDAS: conta o tamanho REAL do vector, que presumir o
// vinte e quatro seria ler fóra de limite no dia em que o contracto mudasse.
Quadro compor(const std::vector<float>& bandas, std::size_t largura,
              std::size_t altura, bool mudo = false);

// sequencia_da_celula — os BYTES da célulla: a tinta imediatamente antes do
// glifo, sem repouso pelo meio, ou a ordem de repouso quando não se pinta. Mora
// aqui, e não no exemplo, para que emissão e quadro tenham UMA verdade só.
std::string sequencia_da_celula(const Celula& celula);

// elemento_do_espectro — o quadro em FTXUI, para a issue #7 o encaixar na
// janella. Derivação fina, e regra de desenho alguma se acrescenta aqui: o que
// divergir do quadro é defeito, e não decisão.
ftxui::Element elemento_do_espectro(const Quadro& quadro);

}  // namespace mysong::tui

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
