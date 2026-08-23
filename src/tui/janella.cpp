// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA JANELLA — src/tui/janella.cpp
// ══════════════════════════════════════════════════════════════════════════
// A porta do programa. Abre a janella no terminal, escreve a marca do núcleo,
// aguarda tecla, e sahe. É o ÚNICO modulo do reino que possue main(), de sorte
// que a bateria de provas, que traz o seu proprio main, jamais colida com ele.
//
// DOMÍNIO ......... as teclas que o terminal entrega enquanto a janella vive.
// CONTRA-DOMÍNIO .. o status de sahida do processo: zero, sempre.
// INVARIANTE ...... a tecla 'q' encerra a laçada e nenhuma outra o faz. O texto
//                   mostrado é o de nucleo::marca(), nunca um literal proprio.
// Q.E.D. .......... a marca vem do núcleo e a tela apenas a exibe; logo, o que
//                   a prova de fumo afirma e o que o olho vê não podem divergir.
// ══════════════════════════════════════════════════════════════════════════
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "nucleo/marca.hpp"

int main() {
  auto tela = ftxui::ScreenInteractive::FitComponent();
  auto pintor = ftxui::Renderer([] {
    return ftxui::vbox({
               ftxui::text(std::string(mysong::nucleo::marca())) | ftxui::bold,
               ftxui::text("tecle q para sahir") | ftxui::dim,
           }) |
           ftxui::border;
  });
  auto janella = ftxui::CatchEvent(pintor, [&](const ftxui::Event& tecla) {
    if (tecla != ftxui::Event::Character('q')) return false;
    tela.Exit();
    return true;
  });
  tela.Loop(janella);
  return 0;
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
