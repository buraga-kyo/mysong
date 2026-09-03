// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA ONDA — src/nucleo/onda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o ffmpeg no fim, apartado, para que o
// olho veja a fronteira de um relance.
//
// DOMÍNIO ......... o caminho de uma faixa, ou as amostras já decodificadas.
// CONTRA-DOMÍNIO .. a envolvente em [0,1], ou a ausencia com a razão dita.
// INVARIANTE ...... funcção alguma d'aqui lança, e faixa sem onda não é falha:
//                   é a barra chata, que nunca ha de pender do ffmpeg.
// Q.E.D. .......... sendo o cache texto com versão, a bateria escreve-o á mão
//                   e afere a recusa sem correr programa algum.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/onda.hpp"

#include "nucleo/capa.hpp"  // somma_dos_octetos, raiz_do_cache: o mesmo cache

#include <algorithm>
#include <cmath>
#include <fstream>

namespace mysong::nucleo {

std::vector<std::string> linha_de_commando_da_onda(
    const std::filesystem::path& faixa) {
  // Cada bandeira cobre um caso, e não um receio. O `-v error` cala o banner
  // que o ffmpeg escreve sempre; o `-nostdin` impede que elle dispute com o
  // FTXUI o teclado do operador, que corre no mesmo terminal; e o `-vn` deita
  // fóra a CAPA embutida, que sem elle o ffmpeg trataria por corrente de
  // video e a onda sahiria da imagem em vez de sahir do som.
  return {"ffmpeg", "-v",  "error", "-nostdin", "-i",    faixa.string(),
          "-vn",    "-ac", "1",     "-ar",      "8000",  "-f",
          "s16le",  "pipe:1"};
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
