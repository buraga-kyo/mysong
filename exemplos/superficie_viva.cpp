// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SUPERFÍCIE VIVA — exemplos/superficie_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// O binario com que o aceite d'esta issue se prova, á maneira do toca_tom que a
// issue #3 deixou. Ergue o motor de carne, o tocador e o socket de commando, e bate
// os dous num laço só. NÃO linka FTXUI, e é parte do aceite que não linke: a
// superfície de commando não conhece a TUI, e um binario que a carregasse deixaria
// a affirmação sem prova mecanica.
//
// Por que existe, e vale dizê-lo: o binario «mysong» de hoje NÃO toca. O seu
// erguer_tocador() pinta a marca e espera a tecla q, e Tocador algum se constroe
// ahi; ligar a tela ao nucleo é a issue #7. Donde «com o mysong tocando», que é como
// o aceite está escripto, não tem hoje onde se dar, e prova-se aqui.
//
// DOMÍNIO ......... os caminhos de faixa que vierem na linha de commando.
// CONTRA-DOMÍNIO .. som na saída de áudio, e um socket que responde.
// INVARIANTE ...... UMA linha de execução, e uma só. O tocador não é seguro a
//                   threads, e o socket bate na MESMA linha que elle: ordem alguma
//                   se intercala no meio de uma transição do nucleo, e a garantia é
//                   estructural e não vigilancia.
// Q.E.D. .......... o que falta degrada e não aborta: sem faixa alguma, o socket
//                   sobe e responde; sem socket, o som toca. Cada falta escreve a
//                   sua razão no stderr, e nenhuma d'ellas cala.
// ══════════════════════════════════════════════════════════════════════════
#include <csignal>
#include <iostream>
#include <string>

#include <ctime>

#include "api/socket.hpp"
#include "nucleo/motor.hpp"
#include "nucleo/tocador.hpp"

namespace {

// A bandeira do sinal. Volatil e de typo atomico do C, que é o que se pode tocar
// dentro de um manejador de sinal sem entrar em terreno que a norma não define.
volatile std::sig_atomic_t pedido_de_sahida = 0;

extern "C" void ao_sinal(int) { pedido_de_sahida = 1; }

}  // namespace

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
