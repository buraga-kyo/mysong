// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO EXEMPLO DA FITA DO ESPECTRO — exemplos/fita_espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta a fita de barras verticaes para se OLHAR, e para se INSPECCIONAR. Não é
// prova: é a peça que o olho do operador ha de julgar, n'um terminal com
// truecolor, e é tambem a sahida cujos bytes se conferem com o grep.
//
// Não colhe som algum: as bandas vêm do argumento, de sorte que a mesma linha de
// commando dá sempre os mesmos bytes. Fosse ella a colher do PipeWire, a sahida
// mudaria a cada corrida e prova alguma se poderia fazer d'ella.
//
// DOMÍNIO ......... largura e altura em célullas, e as bandas em [0,1]. Sem
//                   bandas, arma-se uma rampa determinística.
// CONTRA-DOMÍNIO .. `altura` linhas na sahida padrão, e o status zero.
// INVARIANTE ...... a tinta sahe IMMEDIATAMENTE antes do glifo que veste, sem
//                   repouso pelo meio, e cada linha remata em repouso.
// Q.E.D. .......... redigida a sahida a um arquivo, os bytes dizem quaes glifos
//                   sahiram, em que linha, e com que tinta. Que a fonte os
//                   resolva sem filete é materia que sómente o olho decide.
// ══════════════════════════════════════════════════════════════════════════
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "nucleo/analisador.hpp"
#include "tui/espectro.hpp"
#include "tui/tokens.hpp"

namespace es = mysong::tui;
namespace tk = mysong::tui::tokens;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr,
                 "uso: fita_espectro <largura> <altura> [--mudo] [banda...]\n"
                 "  sem bandas, arma-se uma rampa de %zu degraus.\n",
                 mysong::nucleo::QUANTAS_BANDAS);
    return 2;
  }
  const int largura = std::atoi(argv[1]);
  const int altura = std::atoi(argv[2]);
  if (largura < 0 || altura < 0) return 2;

  bool mudo = false;
  std::vector<float> bandas;
  for (int i = 3; i < argc; ++i) {
    if (std::string(argv[i]) == "--mudo") { mudo = true; continue; }
    bandas.push_back(static_cast<float>(std::atof(argv[i])));
  }
  // A rampa determinística: a banda b vale b sobre QUANTAS_BANDAS, de sorte que
  // a fita sobe da esquerda para a direita e o gradiente se lê em toda a altura.
  if (bandas.empty())
    for (std::size_t b = 0; b < mysong::nucleo::QUANTAS_BANDAS; ++b)
      bandas.push_back(static_cast<float>(b) /
                       static_cast<float>(mysong::nucleo::QUANTAS_BANDAS));

  const es::Quadro quadro =
      es::compor(bandas, static_cast<std::size_t>(largura),
                 static_cast<std::size_t>(altura), mudo);

  std::string tela;
  for (std::size_t l = 0; l < quadro.altura; ++l) {
    for (std::size_t c = 0; c < quadro.largura; ++c)
      tela += es::sequencia_da_celula(quadro.em(l, c));
    tela += tk::repouso;
    tela += '\n';
  }
  std::fputs(tela.c_str(), stdout);
  return 0;
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
