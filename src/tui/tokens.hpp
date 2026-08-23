// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS CÔRES — src/tui/tokens.hpp
// ══════════════════════════════════════════════════════════════════════════
// Porta a "SUNCORE HUD, edição violeta" do RADICAL-OS a esta Casa. A fonte
// versionada é ~/.config/awesome/src/theme/palette.lua, e este manuscripto é
// CÓPIA e não ligação: quem mudar lá, muda aqui, e a prova o cobra.
// Vale o POSTULADO DO POENTE CONTIDO: amarello, laranja, magenta e cyan são
// acento CONTIDO (séries de dados, estados, urgências), nunca superfície nem
// acento primário. O acento cardeal é v500, invariante desta obra e daquella.
//
// DOMÍNIO ......... o vacuo. As côres são constantes decretadas, não medidas.
// CONTRA-DOMÍNIO .. cadeias "#RRGGBB" de sete bytes, e delas a tríade de
//                   octetos que a sequência SGR de truecolor demanda.
// INVARIANTE ...... todo token é constexpr e OPACO: aqui não mora côr de oito
//                   dígitos, porque o terminal não tem canal de opacidade; a
//                   opacidade resolve-se por mistura sobre fundo conhecido.
// Q.E.D. .......... havendo uma só taboada, nenhum modulo desta Casa escreve
//                   hexadecimal cru, e a deriva da paleta deixa de ser
//                   invisivel: torna-se uma linha a corrigir n'um só logar.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string_view>

namespace mysong::tui::tokens {

// ── Os fundos (os negros-violáceos): substrato nocturno sobre que se ergue a
// HUD. ADVERTÊNCIA DE NOME: o token `void` da fonte chama-se aqui `vacuo`,
// que `void` é palavra reservada da linguagem e por nome não se admitte.
constexpr std::string_view vacuo = "#000000", abyss = "#070310", base = "#0c0617";
constexpr std::string_view inset = "#0a0514", panel = "#130a24", panel_hi = "#1b1030";
constexpr std::string_view raised = "#241640";

// ── A rampa violeta: escala monotónica de luminosidade, do claro ao profundo.
// O degrau v500 é o acento cardeal; d'elle não se desvia sem decreto.
constexpr std::string_view v50 = "#f4effe", v100 = "#e7dafd", v200 = "#cbb2fb";
constexpr std::string_view v300 = "#b491f9", v400 = "#9d6ff6", v500 = "#8b5cf6";
constexpr std::string_view v600 = "#7c3aed", v700 = "#6d28d9", v800 = "#5b21b6";
constexpr std::string_view v900 = "#4c1d95", v950 = "#2e1065", v975 = "#2b0c45";

}  // namespace mysong::tui::tokens
