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
