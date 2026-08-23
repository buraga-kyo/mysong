// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TELA DOS REQUISITOS — src/tui/tela_requisitos.hpp
// ══════════════════════════════════════════════════════════════════════════
// Mostra o que a sonda colheu, de tres maneiras que partilham UMA verdade: o
// quadro pintado, que é a unica cousa que apparece havendo impedimento; o
// relatorio em texto puro, do modo de diagnostico; e a linha curta do aviso,
// que precede o tocador sem o interromper.
//
// DA FRASE DO LIMITE, que aqui mora e d'aqui se serve ás tres: o fontconfig
// prova que a fonte está no SYSTEMA, e não que o emulador a elegeu; essa
// segunda consulta não existe. Escreve-se isto ao operador em portuguez claro,
// em vez de lhe promettermos garantia que não temos. Vivendo a frase numa
// constante só, tela e texto não podem divergir sobre o que a sonda sabe.
//
// DOMÍNIO ......... um relatorio da sonda, já colhido. Nada se sonda aqui.
// CONTRA-DOMÍNIO .. um elemento do FTXUI, ou uma cadeia de texto puro.
// INVARIANTE ...... côr alguma se escreve por literal: sahem todas de
//                   tui::tokens. O impedimento veste crit, o aviso veste warn,
//                   e o remedio veste text_muted, sempre.
// Q.E.D. .......... pintando-se o elemento fóra de terminal, o que a tela diz
//                   afere-se por prova, e não pelo olho de quem a abriu.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string>
#include <string_view>

#include <ftxui/dom/elements.hpp>

#include "nucleo/sonda.hpp"

namespace mysong::tui {

// A frase do LIMITE d'esta sonda, e a unica fonte da verdade d'ella. Vae ao
// quadro pintado e ao texto do diagnostico, palavra por palavra.
inline constexpr std::string_view kLimiteDaSonda =
    "Nota do limite: o fontconfig informa que a fonte está installada no "
    "systema, e NÃO que o seu emulador de terminal esteja configurado para "
    "usá-la; essa segunda consulta não existe para programa algum que corra "
    "dentro do terminal. Vendo aqui \"presente\" e ainda assim quadrículo "
    "vazio na tela, o logar a olhar é a configuração de fonte do emulador.";

// elemento_dos_requisitos — o QUADRO. Nomeia cada falta, veste-a da côr da sua
// gravidade, escreve o remedio abaixo, e remata pela frase do limite. Havendo
// impedimento, é a unica cousa que o operador ha de ver.
ftxui::Element elemento_dos_requisitos(const nucleo::Relatorio& relatorio);

// texto_do_relatorio — o diagnostico em texto puro, do modo --sonda. Diz TODOS
// os requisitos, presentes inclusos, que é metade do valor de um diagnostico;
// e remata pela mesma frase do limite. Nenhuma sequencia de escape sahe d'aqui.
std::string texto_do_relatorio(const nucleo::Relatorio& relatorio);

// texto_dos_avisos — as linhas curtas que precedem o tocador quando ha sómente
// aviso. Não interrompe cousa alguma: escreve-se e o tocador sobe. Não havendo
// aviso, devolve cadeia vazia, e nada se escreve.
std::string texto_dos_avisos(const nucleo::Relatorio& relatorio);

}  // namespace mysong::tui

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
