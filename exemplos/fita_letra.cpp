// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO EXEMPLO DA FITA DA LETRA — exemplos/fita_letra.cpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta o RIO da letra por cima do espectro, para se OLHAR e para se
// INSPECCIONAR. Não é prova: é a peça que o olho do operador ha de julgar n'um
// terminal com truecolor, e é tambem a sahida cujos bytes se conferem.
//
// Não colhe som nem relogio: a posição vem do argumento, donde a mesma linha de
// commando dá sempre os mesmos bytes. Corrida em tres posições, vê-se a mesma
// linha a nascer, a subir e a chegar, sem se abrir o tocador.
//
// DOMÍNIO ......... largura e altura em célullas, a posição em segundos, e o
//                   caminho de um `.lrc`. Sem elle, arma-se uma letra propria.
// CONTRA-DOMÍNIO .. `altura` linhas de espectro com o rio por cima, na sahida
//                   padrão, mais a linha que diz qual verso se canta; status 0.
// INVARIANTE ...... a fita sahe do MESMO tapete que a janella compõe. Regra de
//                   desenho alguma se escreve aqui: o que divergir do tapete é
//                   defeito, e não decisão d'este exemplo.
// Q.E.D. .......... redigida a sahida a um arquivo, os bytes dizem que glyphos
//                   sahiram, em que linha e com que tinta e que cama.
// ══════════════════════════════════════════════════════════════════════════
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "nucleo/analisador.hpp"
#include "nucleo/letra.hpp"
#include "tui/espectro.hpp"
#include "tui/letra_viva.hpp"
#include "tui/tokens.hpp"

namespace es = mysong::tui;
namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
