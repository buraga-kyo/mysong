// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS CÔRES, src/tui/tokens.hpp
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

#include <string>
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

// ── Néon e fulgor: as côres do halo luminoso, ditas na língua estrangeira
// "glow". O glow_hot, magenta das montanhas ao poente, é das urgências.
constexpr std::string_view glow_ice = "#d6c2ff", glow_soft = "#b794ff";
constexpr std::string_view glow_core = "#a855f7", glow_hot = "#ff2fa0";

// ── O texto: hierarchia de legibilidade, do brilhante ao desvanecido.
constexpr std::string_view text_bright = "#e9dcff", text_primary = "#cbb6ff";
constexpr std::string_view text_heading = "#b794ff", text_body = "#9a82c4";
constexpr std::string_view text_muted = "#6f5a96", text_faint = "#463566";
constexpr std::string_view text_disabled = "#2f2348";

// ── Linhas, bordas e biséis: a geometria das arestas e do relevo apparente.
constexpr std::string_view line_faint = "#241640", line_dim = "#3a1f63";
constexpr std::string_view line_base = "#5b21b6", line_bright = "#7c3aed";
constexpr std::string_view bevel_hi = "#9d6ff6", bevel_lo = "#160c28";
constexpr std::string_view grid = "#3a1f63";

// ── Séries de dados: aos gráphicos concede-se o espectro inteiro do poente,
// o violeta do núcleo (data1, soberano), amarello-sol, laranja, magenta, o
// CYAN dos olhos do eidolon (data5) e o vermelho do quadro solar.
constexpr std::string_view data1 = "#a855f7", data2 = "#ffd319", data3 = "#ff7a1a";
constexpr std::string_view data4 = "#ff2fa0", data5 = "#2ef2da", data6 = "#ff3d3d";

// ── Os estados (subtis): bom curso, advertência e crise.
constexpr std::string_view ok = "#2ee6a8", warn = "#ffd319";
constexpr std::string_view crit = "#ff3948", info = "#8b5cf6";

// ── Sentinella de plena transparência. No terminal ella não se pinta: vale
// por ORDEM DE REPOUSO, e quem a receba emitte o reset em vez de tríade.
constexpr std::string_view transparent = "#00000000";

// ── A família alaranjada do lançador, calibrada ao céu poente da estampa.
constexpr std::string_view launcher_ring = "#b35414", launcher_ring_hi = "#ff7a1a";
constexpr std::string_view launcher_ring_on = "#ff9e2c", launcher_glow = "#ff7a1a";

// ── Os tons APAGADOS do horizonte (o poente ao cahir da noite): data4/data3
// rebaixadas a ~55/100, para o repouso dos widgets; o horizonte vivo fica á
// eleição e ao hover. E o traço de UPLOAD (lima-neon), par do laranja data3.
constexpr std::string_view data4_deep = "#8c1a58", data3_deep = "#8c430e";
constexpr std::string_view graph_up = "#d8ff2f";

// ── Receitas nomeadas de opacidade, portadas de palette.alpha: extirpam a
// dispersão dos valores crus. No terminal ellas NÃO pintam sozinhas, não ha
// canal de opacidade; entram sempre por mistura(), sobre um fundo conhecido.
namespace alfa {
constexpr double panel = 0.92, panel_hi = 0.95, bar = 0.8, clock_bar = 0.85;
constexpr double dash = 0.9, chip_bg = 0.06, chip_border = 0.20;
constexpr double divider_tail = 0.22, bevel_hi = 0.7, bevel_lo = 0.9, tick = 0.9;
constexpr double rail = 0.5, graph_area = 0.22, graph_halo = 0.18, well = 0.96;
constexpr double well_border = 0.22, segment = 0.90, tab = 0.88;
constexpr double hover_delta = 0.08, overlay = 0.55, scrim = 0.7;
}  // namespace alfa

// A tríade de octetos que a sequência SGR de truecolor demanda.
struct Triade {
  unsigned char r = 0, g = 0, b = 0;
};

// octeto, lê um par de dígitos hexadecimaes. Tolera-se caixa alta e baixa.
constexpr unsigned octeto(char alto, char baixo) {
  const auto valor = [](char c) -> unsigned {
    return c <= '9' ? static_cast<unsigned>(c - '0')
                    : static_cast<unsigned>((c | 0x20) - 'a' + 10);
  };
  return valor(alto) * 16u + valor(baixo);
}

// rgb, decompõe "#RRGGBB" na tríade, como o fazia palette.rgb para o cairo.
// Tolera-se o prefixo '#' e o oitavo byte de opacidade: sómente a tríade se
// restitue ao chamador, que é tudo quanto o terminal sabe pintar.
constexpr Triade rgb(std::string_view hex) {
  if (!hex.empty() && hex.front() == '#') hex.remove_prefix(1);
  if (hex.size() < 6) return {};
  return {static_cast<unsigned char>(octeto(hex[0], hex[1])),
          static_cast<unsigned char>(octeto(hex[2], hex[3])),
          static_cast<unsigned char>(octeto(hex[4], hex[5]))};
}

// mistura, resolve a opacidade em côr OPACA, compondo `frente` sobre um
// `fundo` CONHECIDO. É o analogo terminal de palette.a(hex, alpha): visto que
// o SGR sómente aceita a tríade, a côr de oito dígitos jamais sahe d'aqui.
// Confina-se o alfa ao intervallo fechado [0,1], tal como na fonte.
constexpr Triade mistura(std::string_view frente, std::string_view fundo,
                         double alfa) {
  alfa = alfa < 0.0 ? 0.0 : (alfa > 1.0 ? 1.0 : alfa);
  const Triade f = rgb(frente), t = rgb(fundo);
  const auto pesa = [&](unsigned char a, unsigned char b) {
    return static_cast<unsigned char>(a * alfa + b * (1.0 - alfa) + 0.5);
  };
  return {pesa(f.r, t.r), pesa(f.g, t.g), pesa(f.b, t.b)};
}

// sgr, a sequencia de escape que veste a célula. O papel 38 é a TINTA, com
// que se pinta o glifo; o 48 é o FUNDO, que é a sua cama; e o repouso desfaz
// ambos. As componentes escrevem-se em decimal, sem zero á esquerda.
inline std::string sgr(int papel, Triade c) {
  return "\x1b[" + std::to_string(papel) + ";2;" + std::to_string(c.r) + ';' +
         std::to_string(c.g) + ';' + std::to_string(c.b) + 'm';
}
inline std::string tinta(std::string_view hex) { return sgr(38, rgb(hex)); }
inline std::string fundo_de(std::string_view hex) { return sgr(48, rgb(hex)); }
constexpr std::string_view repouso = "\x1b[0m";

}  // namespace mysong::tui::tokens

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//, Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
