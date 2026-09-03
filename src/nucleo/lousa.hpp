// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LOUSA — src/nucleo/lousa.hpp
// ══════════════════════════════════════════════════════════════════════════
// A LOUSA é uma janella de X11 posta POR CIMA do terminal, e não desenho no
// terminal: quem a segura é o `ueberzugpp`, erguido em filho proprio, a quem
// se fala por um cano em JSON, uma ordem por linha. É assim que o yazi d'esta
// machina fica nitido, e é o unico caminho que ha: o Alacritty corre dentro do
// tmux, e nem o protocolo do kitty nem o sixel atravessam o tmux.
//
// DOMÍNIO ......... um identificador, o caminho de uma imagem, e o rectangulo
//                   em CÉLULLAS a contar do canto do TERMINAL.
// CONTRA-DOMÍNIO .. linhas de JSON no cano do filho; ou nada, e ahi quem chama
//                   pinta os symbolos do chafa como sempre pintou.
// INVARIANTE ...... funcção alguma d'aqui bloqueia nem lança. Cano cheio
//                   descarta a ordem e conta-a; filho morto responde ausente.
// Q.E.D. .......... sendo a composição do JSON funcção pura, a bateria afere o
//                   protocolo inteiro sem X11 vivo e sem erguer processo algum.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// argumentos_da_lousa — o que se corre, pelo precedente do argumentos_do_chafa.
// `layer` é o modo de janella persistente; `--silent` manda o erro d'elle ao
// buraco, que esta Casa corre debaixo de uma tela do FTXUI e linha de aviso no
// meio do quadro estraga-o; `-o x11` é a sahida MEDIDA n'esta machina, a mesma
// que o yazi escolhe. A de wayland existe, e fica para quando houver Wayland.
std::vector<std::string> argumentos_da_lousa();

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
