// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA VIVA — src/tui/letra_viva.hpp
// ══════════════════════════════════════════════════════════════════════════
// O RIO da letra (issue #109). A letra deixa de se alternar com o espectro e
// passa a morar POR CIMA d'elle: cada linha assoma na base do painel ainda sem
// fórma, sobe até a LINHA DE LEITURA chegando lá no instante em que se canta, e
// d'ahi segue subindo, perdendo luz, até morrer na linha zero.
//
// DOMÍNIO ......... as linhas do `.lrc` com os instantes d'ellas, a posição da
//                   faixa em segundos, e a largura e a altura do rectangulo do
//                   espectro em CÉLULLAS.
// CONTRA-DOMÍNIO .. um QUADRO DA LETRA: de cada linha á vista, a linha da tela
//                   em que assenta, a collunha, o texto com a fracção de
//                   glyphos já resolvidos, e a tinta.
// INVARIANTE ...... funcção PURA, e sem relogio proprio. A mesma posição dá o
//                   MESMO quadro, e é assim que a bateria o interroga sem
//                   terminal, sem som e sem espera.
// Q.E.D. .......... letra alguma se inventa: faixa sem `.lrc` dá quadro vazio,
//                   e o painel mostra o espectro como se esta obra não houvesse.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "nucleo/letra.hpp"
#include "tui/espectro.hpp"
#include "tui/sala.hpp"
#include "tui/tokens.hpp"

namespace mysong::tui {

// linha_de_leitura — o TERÇO do alto onde a linha chega no instante d'ella. Um
// terço, e não o meio: o que vem tem de caber por baixo, que é o vão da subida,
// e o que já passou por cima, que é o vão do apagar.
std::size_t linha_de_leitura(std::size_t altura);


// O NASCIMENTO. Quatro segundos antes do instante d'ella a linha assoma na
// base, ou o INTERVALLO desde a anterior quando elle é menor: letra apressada,
// de duas linhas no mesmo segundo, não ha de ter duas a subir sobrepostas.
inline constexpr double NASCIMENTO_MAXIMO = 4.0;

// E um PISO, que não é gosto: é a guarda da divisão. Carimbo repetido dá
// intervallo zero, e zero no denominador da subida daria fracção infinita. Um
// quarto de segundo faz a linha saltar da base á leitura, que é o que uma letra
// de carimbos eguaes de facto pede.
inline constexpr double NASCIMENTO_MINIMO = 0.25;

// nascimento_da_linha — quantos segundos ANTES do instante d'ella a linha
// `qual` assoma na base. A primeira conta o intervallo desde o zero da faixa,
// que é o instante anterior que ella tem: regra UNA para todas, e não um caso á
// parte que a bateria teria de provar duas vezes.
double nascimento_da_linha(const std::vector<nucleo::LinhaDaLetra>& linhas,
                           std::size_t qual);

// Uma LINHA do rio, já resolvida para a tela. `qual` é o indice d'ella na letra,
// e não no quadro: a irmã que ha de cristalizar a corrente em XIROD precisa de
// saber QUE verso é, e contar de novo pelo tempo daria duas verdades.
struct LinhaViva {
  std::size_t qual = 0;
  std::size_t linha_da_tela = 0;  // zero é o TOPO, como no Quadro do espectro
  std::size_t collunha = 0;       // onde o texto assenta, já centrado
  std::string texto;              // cortado á largura, e com o embaralho do instante
  std::string_view tinta = tokens::text_faint;
  double resolvida = 0.0;         // a fracção de glyphos já resolvidos, em [0,1]
  bool corrente = false;          // está na linha de leitura, e é a que se canta
};

// O QUADRO DA LETRA: as linhas Á VISTA n'uma posição, e mais nada. Linha que
// ainda não nasceu e linha que já morreu no alto não entram, de sorte que quem
// compõe não tem de as filtrar outra vez.
struct QuadroDaLetra {
  std::size_t largura = 0;
  std::size_t altura = 0;
  std::vector<LinhaViva> linhas;
  // O INDICE em `linhas` da que está na linha de leitura, ou menos um. Indice, e
  // não ponteiro, para que o quadro se copie sem cuidado.
  int corrente = -1;

  bool vazio() const noexcept { return linhas.empty(); }
};

// glifos_da_linha — a cadeia partida em pontos de codigo, um por CÉLULLA. Conta
// por CODEPOINT, e não por largura de columna: o glypho largo (CJK, emoji) sahe
// contado por um, e é o mesmo debito que a pauta d'esta Casa já carrega.
std::vector<std::string> glifos_da_linha(std::string_view texto);

// embaralha — os glyphos que ainda se não resolveram, tirados das PROPRIAS
// letras da linha. Os resolvidos são os do MEIO, e a fórma abre-se do centro
// para as pontas conforme a linha sobe. O branco CONSERVA-SE branco, que é o que
// deixa a fórma das palavras a ler-se antes das letras.
//
// `quadro` é a conta do embaralho (ver QUADROS_DO_EMBARALHO), e não o relogio:
// a mesma posição dá a mesma fita, sempre, e é isso que a bateria affirma.
std::string embaralha(const std::vector<std::string>& glifos, double resolvida,
                      std::size_t qual, long long quadro);

// Os QUADROS DO EMBARALHO por segundo. Oito, que é o bastante para o olho ler
// fervura e não pisca-pisca. Sahe da POSIÇÃO, e nunca de contador proprio: fosse
// contador, a mesma posição daria fitas differentes e a pureza cahia.
inline constexpr int QUADROS_DO_EMBARALHO = 8;

// quadro_da_letra — o rio n'uma posição. A linha `i` nasce na base
// `nascimento_da_linha` segundos antes do instante d'ella e sobe LINEARMENTE até
// a linha de leitura, onde chega no instante exacto; ahi fica em text_bright até
// que a seguinte chegue, e d'ahi sobe UMA linha por segundo, em text_muted e
// depois em text_faint, até sumir na linha zero.
//
// Linha de texto VAZIO não pinta nada, e é de proposito: é assim que o LRCLIB
// marca o silencio entre estrophes, e o carimbo d'ella continua a valer para
// expulsar a anterior da linha de leitura na hora certa.
QuadroDaLetra quadro_da_letra(const std::vector<nucleo::LinhaDaLetra>& linhas,
                              double posicao, std::size_t largura,
                              std::size_t altura);

// linha_corrente_do_rio — a linha que se canta, ou nada. Funcção NOMEADA, e não
// campo a que se chegue por conta: a issue irmã da letra em XIROD pende d'ella.
const LinhaViva* linha_corrente_do_rio(const QuadroDaLetra& quadro);

// caixa_da_corrente — o rectangulo que a linha corrente occupa, em coordenadas
// do RECTANGULO DO ESPECTRO e não da tela: quem a põe na tela somma-lhe o canto
// do painel, que é o unico que sabe onde o painel começa. Vazio quando não ha
// linha corrente, e por ahi se sabe que chapa alguma se ha de pôr.
Rectangulo caixa_da_corrente(const QuadroDaLetra& quadro);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
