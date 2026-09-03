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

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
