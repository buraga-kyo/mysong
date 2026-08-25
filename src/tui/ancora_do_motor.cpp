// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA ÂNCORA DO MOTOR — src/tui/ancora_do_motor.cpp
// ══════════════════════════════════════════════════════════════════════════
// PEÇA TEMPORARIA. Quem a achar, NÃO a apague por parecer morta: ella é
// load-bearing para uma prova, e o commentario abaixo diz de qual.
//
// Existe para que o alvo mysong REFERENCIE o motor hoje, e por isso puxe o
// motor.o de dentro da bibliotheca estatica para o binario da tela. É a
// configuração que a issue #7 ha de criar de verdade quando ligar a barra de
// transporte ao motor; tendo-a hoje, a prova da ligação julga o binario que o
// operador vae correr amanhã, e não o de agora, que não linka o motor e por isso
// passaria a prova por ausencia de motor em vez de por acerto.
//
// DOMÍNIO ......... nada.
// CONTRA-DOMÍNIO .. a versão da interface da libmpv presente, ou zero sem ella.
// INVARIANTE ...... não abre motor, não toca som, não lê ambiente: sómente
//                   REFERE. Não ha caminho por onde ella mude o que o programa
//                   faz, e por isso o seu risco é nullo.
// Q.E.D. .......... a issue #7 substitue esta peça pela chamada de verdade em
//                   janella.cpp, e então este arquivo apaga-se. Até lá, é ella
//                   que faz o critério da ligação ser exercitavel.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/motor.hpp"

namespace mysong::tui {

// Não é static, de proposito: symbolo externo não accusa aviso de funcção sem
// uso, e esta unidade entra no alvo pela lista de fontes, que o ligador toma
// inteira. Assim o motor entra sem que se toque janella.cpp, que é da #7.
unsigned long versao_do_motor() {
  return nucleo::MotorMpv::versao_da_interface();
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
