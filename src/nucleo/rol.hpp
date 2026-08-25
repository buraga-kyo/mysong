// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ROL — src/nucleo/rol.hpp
// ══════════════════════════════════════════════════════════════════════════
// As LISTAS do operador: o que elle juntou á mão, na ordem em que quiz. Vivem
// n'um banco PROPRIO, e não no da bibliotheca, e a razão não é gosto.
//
// A bibliotheca é RECONSTRUIDA a cada varredura: o Escriba lavra n'um temporario
// e renomeia por cima do antigo. Taboa de lista no mesmo arquivo seria apagada em
// toda varredura, e o operador perderia a lista por ter mandado varrer o disco.
// Donde arquivo á parte, `rol.sqlite3` ao lado do índice.
//
// A atomicidade tambem é differente, e por bom motivo. A bibliotheca escreve-se
// de uma vez e por isso o temporario e o rename servem-lhe. A lista muta-se linha
// a linha, e reconstruir o arquivo a cada linha perderia a lista da mão alheia
// que estivesse a mutar outra; aqui a atomicidade é a da TRANSACÇÃO, com
// journal_mode=DELETE e synchronous=FULL, que é a mesma garantia por operação.
//
// DOMÍNIO ......... um caminho de banco, que ENTRA POR PARÂMETRO, e os nomes e
//                   caminhos que o operador disse.
// CONTRA-DOMÍNIO .. as listas, e as faixas de cada uma na ordem gravada.
// INVARIANTE ...... a ordem é EXPLICITA e CONTIGUA: os itens de uma lista têm
//                   ordem 0, 1, 2, ... sem buraco, e retirar um fecha o buraco.
//                   Sem isso, mover para cima teria de adivinhar quem é o vizinho.
// Q.E.D. .......... entrando o caminho por parâmetro, a bateria corre inteira em
//                   directorio temporario, e prova alguma pode tocar as listas de
//                   quem nos usa.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3;

namespace mysong::nucleo {

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
