// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROJECTOR — src/nucleo/video.hpp
// ══════════════════════════════════════════════════════════════════════════
// O VÍDEO em janella propria do systema, e NÃO dentro do terminal. A propria
// documentação do mpv diz que a sahida grafica d'elle não sincroniza com o resto
// do terminal; a decisão tomada foi janella á parte, com qualidade cheia.
//
// E é uma SEGUNDA instancia de mpv, em processo proprio, e não a libmpv que o
// motor já embute. Duas razões, e as duas pesadas:
//
// 1. o motor abre-se com «video=no» ANTES de mpv_initialize, e essa opção não se
//    volta atraz sem reabrir o punho: reabri-lo derrubaria o som que toca;
// 2. a tarefa pede janella com CLASSE propria, para o RADICAL-OS a governar por
//    regra depois, e pede que `pgrep` prove que processo algum fica para traz.
//    Processo proprio é o unico modo de as duas cousas serem verdade.
//
// O governo é pelo soquete de commandos do mpv, em JSON por linha. Tecla alguma se
// manda á janella: a TUI manda ordem pelo soquete, e a janella obedece.
//
// DOMÍNIO ......... o caminho de uma faixa, e as ordens do operador.
// CONTRA-DOMÍNIO .. uma janella do systema a tocar, e um processo que morre
//                   quando se manda, sem deixar orfão.
// INVARIANTE ...... o AUDIO não dobra. Quem cala o motor é quem abre a fita, e a
//                   fita nunca se abre com o motor a tocar.
// Q.E.D. .......... sendo puras a lista de argumentos, a redacção das ordens e o
//                   juizo de que ha video, a bateria afere o que se HA DE correr e
//                   o que se HA DE mandar sem abrir janella alguma.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <sys/types.h>

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
