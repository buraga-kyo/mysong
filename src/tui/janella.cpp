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
#include "nucleo/sonda.hpp"
#include "tui/tela_requisitos.hpp"

namespace nucleo = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

// erguer_tocador — o tocador de hoje, palavra por palavra como estava no main.
// Extrahe-se para funcção propria porque agora ha caminho que NÃO chega aqui: o
// impedimento pinta outra tela e sahe, e convem que o olho veja num relance
// que aquelle caminho não toca nesta.
int erguer_tocador() {
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

// recusar_e_sahir — pinta a tela dos requisitos, espera tecla e sahe com codigo
// differente de zero. É a UNICA cousa que apparece havendo impedimento: o
// tocador não se ergue nem por um quadro, e por isso esta funcção não o chama.
// Aqui a espera de tecla fica, ao contrario do caminho do aviso: o programa não
// vae abrir de jeito nenhum, e a interrupção é a propria mensagem.
int recusar_e_sahir(const nucleo::Relatorio& relatorio) {
  auto tela = ftxui::ScreenInteractive::FitComponent();
  auto pintor = ftxui::Renderer(
      [&relatorio] { return tui::elemento_dos_requisitos(relatorio); });
  auto quadro = ftxui::CatchEvent(pintor, [&](const ftxui::Event& tecla) {
    if (!tecla.is_character() && tecla != ftxui::Event::Return &&
        tecla != ftxui::Event::Escape)
      return false;
    tela.Exit();
    return true;
  });
  tela.Loop(quadro);
  return 1;
}

}  // namespace

int main() {
  const nucleo::Relatorio relatorio =
      nucleo::sondar(nucleo::inquerito_do_systema());
  if (relatorio.ha_impedimento()) return recusar_e_sahir(relatorio);
  return erguer_tocador();
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
