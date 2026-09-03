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

namespace {

// O BALDE de um ponto, e é UMA funcção para os DOUS regimes. Havendo amostras
// que cheguem, o balde tem `quantas / pontos` d'ellas e o RESTO fica no
// ultimo, que assim a cauda da faixa não se perde. Havendo MENOS amostras que
// baldes, degenera n'uma amostra por proporção: o indice não decresce, d'onde
// o balde sem amostra propria repete a do vizinho de traz.
struct Balde {
  std::size_t principio = 0, fim = 0;
};
Balde balde_do_ponto(std::size_t quantas, std::size_t p, std::size_t pontos) {
  Balde balde;
  const std::size_t por_balde = quantas / pontos;
  if (por_balde > 0) {
    balde.principio = p * por_balde;
    balde.fim = p + 1 == pontos ? quantas : balde.principio + por_balde;
    return balde;
  }
  balde.principio = (p * quantas) / pontos;
  balde.fim = balde.principio + 1;
  return balde;
}
}  // namespace

Onda onda_das_amostras(const std::int16_t* amostras, std::size_t quantas,
                       std::size_t pontos) {
  Onda onda;
  if (amostras == nullptr || quantas == 0 || pontos == 0) return onda;
  onda.pontos.assign(pontos, 0.0f);
  float maior = 0.0f;
  for (std::size_t p = 0; p < pontos; ++p) {
    const Balde balde = balde_do_ponto(quantas, p, pontos);
    // A somma em double: um balde de faixa comprida traz dezenas de milhar
    // de quadrados, e em float os ultimos não pesariam por falta de mantissa.
    double somma = 0.0;
    for (std::size_t i = balde.principio; i < balde.fim; ++i) {
      const double amostra = static_cast<double>(amostras[i]) / 32768.0;
      somma += amostra * amostra;
    }
    const std::size_t conta = balde.fim - balde.principio;
    onda.pontos[p] = static_cast<float>(
        std::sqrt(somma / static_cast<double>(conta)));
    maior = std::max(maior, onda.pontos[p]);
  }
  // Máximo zero dá zeros, e não divisão por zero: faixa calada é onda rasa.
  if (maior > 0.0f)
    for (float& ponto : onda.pontos) ponto /= maior;
  return onda;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
