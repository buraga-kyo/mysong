// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MENU — src/tui/menu.hpp
// ══════════════════════════════════════════════════════════════════════════
// A machina de FOCO da barra lateral (issue #80): quem manda na tecla, a barra
// ou a lista. Não pinta, não conhece Navegador por dentro, não toca tocador:
// traduz tecla em gesto e guarda o degrau eleito. A ordem do despacho fica
// DECLARADA, para que ninguem a tome por accidente: o prompt trata primeiro
// (issue #79), a barra aberta depois, e a taboada geral do commando por fim.
// Tecla que a barra não conhece FECHA-A e segue á taboada de sempre, e é assim
// que o atalho continua a levar á mesma secção que a barra leva.
//
// DOMÍNIO ......... a tecla que o terminal entrega, e a secção corrente na
//                   hora de abrir.
// CONTRA-DOMÍNIO .. um GestoDaBarra, e o degrau eleito da barra.
// INVARIANTE ...... o degrau eleito está SEMPRE entre zero e o ultimo: as
//                   ordens saturam nos extremos, e ordem alguma o põe fóra. E
//                   o menu não pede cursor de terminal: o unico nó com foco
//                   d'esta obra segue sendo o caret do campo (issue #78).
// Q.E.D. .......... sendo a taboada e a machina funcções puras de tecla e de
//                   estado, a bateria prova o foco inteiro sem erguer terminal.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>

#include <ftxui/component/event.hpp>

#include "tui/navegador.hpp"

namespace mysong::tui {

// Os SETE degraus da barra, na ordem do mockup. A taboada degrau↔secção vive
// aqui e na pintura da tabella; sete é a conta das duas.
inline constexpr std::size_t DEGRAUS_DA_BARRA = 7;

// A taboada. Dentro de uma lista (NoRol) o degrau é o das LISTAS, como na
// pintura de hoje: é lá que se está, um degrau abaixo.
Secao secao_do_degrau(std::size_t degrau) noexcept;
std::size_t degrau_da_secao(Secao secao) noexcept;

// tecla_abre_menu — o Tab, e o Shift+Tab com elle: havendo sómente dous focos,
// avançar e voltar são o mesmo gesto, e tecla morta não se dá a quem explora.
bool tecla_abre_menu(const ftxui::Event& tecla) noexcept;

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
