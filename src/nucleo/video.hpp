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

// A CLASSE da janella. Fixa, e n'uma constante com nome: é por ella que o
// RADICAL-OS a governa por regra, e regra que aponte para nome que se mova é regra
// que se quebra calada.
inline constexpr char kClasseDoVideo[] = "mysong-video";

// tem_video — o juizo pela EXTENSÃO, e a lista é fechada. O limite vae declarado:
// um `.mkv` sem faixa de video existe, e este juizo diz que tem. Perguntar ao mpv
// seria certo e custaria um processo por faixa a cada quadro da tela.
bool tem_video(const std::filesystem::path& faixa);

// extensao_com_video — a mesma pergunta, feita á extensão sósinha. Existe porque a
// VARREDURA tambem precisa d'esta lista, e duas listas em dous arquivos dariam
// duas verdades: o dia em que uma ganhasse `.mkv` e a outra não, o acervo indexava
// o que a tela recusava abrir. A lista tem UM logar, e é este.
bool extensao_com_video(std::string_view extensao);

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
