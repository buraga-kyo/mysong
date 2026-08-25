// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA JANELLA — src/tui/janella.cpp
// ══════════════════════════════════════════════════════════════════════════
// A porta do programa. Sonda os requisitos ANTES de tudo, e sómente depois
// decide: havendo impedimento, pinta a tela dos requisitos e sahe; havendo
// sómente aviso, escreve-o e ergue o tocador; não havendo falta, ergue o
// tocador e nada mais apparece. É o ÚNICO modulo do reino que possue main(), de
// sorte que a bateria de provas, que traz o seu proprio main, jamais colida.
//
// DOMÍNIO ......... os argumentos da linha de commando, o estado do systema tal
//                   como a sonda o colhe, e as teclas que o terminal entrega.
// CONTRA-DOMÍNIO .. o status de sahida: ZERO abrindo o tocador ou correndo o
//                   diagnostico sem impedimento; differente de zero havendo
//                   impedimento, e tambem no diagnostico que o encontre.
// INVARIANTE ...... havendo impedimento, o tocador NÃO se ergue, nem por um
//                   quadro: a funcção da recusa não chama a do tocador, e a
//                   garantia é estructural e não de vigilancia. Sem terminal,
//                   tela alguma se ergue: o texto vae ao stderr.
// Q.E.D. .......... a sonda correndo antes do FTXUI, quem roda numa machina crua
//                   lê o que falta e o remedio, em vez de ver quadrículo vazio
//                   e adivinhar; e a decisão de abrir depende de UM predicado
//                   só, ha_impedimento(), que a bateria prova por dublê.
// ══════════════════════════════════════════════════════════════════════════
#include <chrono>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>
#include <string>
#include <string_view>

#include <unistd.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/terminal.hpp>
#include <ftxui/dom/elements.hpp>

#include "nucleo/analisador.hpp"
#include "nucleo/fila.hpp"
#include "nucleo/marca.hpp"
#include "nucleo/motor.hpp"
#include "nucleo/tocador.hpp"
#include "nucleo/sonda.hpp"
#include "tui/commando.hpp"
#include "tui/espectro.hpp"
#include "tui/tela_requisitos.hpp"
#include "tui/transporte.hpp"

namespace nucleo = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

// retracto_do — colhe o instante do tocador n'uma cópia. É a UNICA funcção que
// pergunta ao tocador, e por isso é o unico logar onde uma pergunta a mais
// poderia dar dous valores no mesmo quadro. Colhe-se tudo aqui, de uma vez.
tui::Retracto retracto_do(nucleo::Tocador& tocador) {
  tui::Retracto retracto;
  retracto.estado = tocador.estado();
  retracto.posicao = tocador.posicao();
  retracto.duracao = tocador.duracao();
  retracto.volume = tocador.volume();
  const nucleo::Fila& fila = tocador.fila();
  retracto.tamanho = fila.tamanho();
  if (!fila.vazia()) {
    retracto.indice = fila.indice();
    retracto.titulo = std::string(fila.corrente());
  }
  return retracto;
}

// cumprir — a ordem em chamada. O `switch` é exhaustivo de proposito: verbo novo
// na taboada acende aviso do compilador aqui, e não passa calado.
void cumprir(const tui::Ordem& ordem, nucleo::Tocador& tocador, bool& sahir) {
  switch (ordem.verbo) {
    case tui::Verbo::Nada: break;
    case tui::Verbo::Pausar: tocador.pausar(); break;
    case tui::Verbo::Retomar: tocador.retomar(); break;
    case tui::Verbo::Proxima: tocador.proxima(); break;
    case tui::Verbo::Anterior: tocador.anterior(); break;
    case tui::Verbo::Buscar: tocador.buscar(ordem.alvo); break;
    case tui::Verbo::Volume: tocador.volume(static_cast<int>(ordem.alvo)); break;
    case tui::Verbo::Sahir: sahir = true; break;
  }
}

// A CADENCIA do relogio. Cincoenta milesimos, que são vinte quadros por segundo:
// o bastante para a barra andar sem salto visivel, e longe do sessenta que faz a
// fita tremer por diff de buffer. O risco do tremor está declarado no plano, e
// esta é a primeira defesa contra elle.
constexpr int MILESIMOS_DO_QUADRO = 50;

// erguer_tocador — o laço de verdade. Ergue o motor, o tocador e o analisador,
// enche a fila com o que veio da linha de commando, e pinta a barra de baixo com
// o espectro por cima. Esta funcção NÃO se prova em bateria: ella abre terminal,
// abre som e depende de relogio. O que se prova são as duas peças que ella usa,
// e é por isso que ellas vivem fóra d'aqui.
int erguer_tocador(const std::vector<std::string>& faixas) {
  std::string razao;
  std::optional<nucleo::MotorMpv> motor = nucleo::MotorMpv::abrir(&razao);
  if (!motor) {
    // Motor que não abre não derruba o programa: diz o que houve e sahe. A
    // fabrica devolve um vasio, e não um objecto meio-aberto a que se tivesse de
    // perguntar se presta.
    std::cerr << "mysong: a machina de som não abriu: " << razao << "\n";
    return 1;
  }

  nucleo::Tocador tocador(*motor);
  nucleo::Analisador analisador;
  if (analisador.vivo()) {
    tocador.observa(analisador);
  } else {
    // Espectro é ornamento, e não requisito: sem elle o tocador toca. Diz-se o
    // que falta, uma vez, e segue-se.
    std::cerr << "mysong: sem espectro: " << analisador.razao() << "\n";
  }

  for (const std::string& faixa : faixas) tocador.fila().junta(faixa);
  if (!tocador.fila().vazia()) tocador.tocar_corrente();

  auto tela = ftxui::ScreenInteractive::Fullscreen();
  bool sahir = false;

  auto pintor = ftxui::Renderer([&] {
    const tui::Retracto retracto = retracto_do(tocador);
    const int largura = ftxui::Terminal::Size().dimx;
    const std::size_t larg = largura > 2 ? static_cast<std::size_t>(largura - 2) : 1;
    const tui::Quadro quadro = tui::compor(tocador.bandas(), larg, 8);
    const std::string cabeca =
        retracto.tamanho == 0 ? "fila vazia" : retracto.titulo;
    return ftxui::vbox({
               ftxui::text(std::string(nucleo::marca())) | ftxui::bold,
               ftxui::text(cabeca) | ftxui::dim,
               tui::elemento_do_espectro(quadro),
               tui::elemento_do_transporte(retracto, larg),
               ftxui::text("espaço pausa · n/p faixa · setas buscam · +/- volume · q sahe") |
                   ftxui::dim,
           }) |
           ftxui::border;
  });


  auto janella = ftxui::CatchEvent(pintor, [&](const ftxui::Event& tecla) {
    const tui::Ordem ordem = tui::ordem_da_tecla(tecla, retracto_do(tocador));
    if (ordem.verbo == tui::Verbo::Nada) return false;  // tecla alheia segue
    cumprir(ordem, tocador, sahir);
    if (sahir) tela.Exit();
    return true;
  });

  // O RELOGIO. Vive em fio proprio porque `tela.Loop` não devolve até se sahir, e
  // o `pulsa` do tocador tem de correr entre quadros: é elle que drena os
  // pregões do mpv e faz a posição andar. O fio não toca a tela: pede-lhe que
  // repinte, e a tela é que serializa.
  std::thread relogio([&] {
    while (!sahir) {
      tocador.pulsa();
      analisador.pulsa();
      tela.PostEvent(ftxui::Event::Custom);
      std::this_thread::sleep_for(std::chrono::milliseconds(MILESIMOS_DO_QUADRO));
    }
  });

  tela.Loop(janella);
  sahir = true;  // a sahida pela tela tambem para o relogio
  relogio.join();
  return 0;
}

// recusar_e_sahir — pinta a tela dos requisitos, espera tecla e sahe com codigo
// differente de zero. É a UNICA cousa que apparece havendo impedimento: o
// tocador não se ergue nem por um quadro, e por isso esta funcção não o chama.
// Aqui a espera de tecla fica, ao contrario do caminho do aviso: o programa não
// vae abrir de jeito nenhum, e a interrupção é a propria mensagem.
int recusar_e_sahir(const nucleo::Relatorio& relatorio) {
  // Fullscreen, e não Fit de altura alguma: a altura que um paragrafo pede só se
  // sabe DEPOIS de se saber a largura em que elle reflue, e nenhum ajuste
  // automatico o adivinha; com Fit, o quadro sahia cortado no pé e a nota do
  // limite perdia as ultimas linhas, que é justamente o que ella existe para
  // dizer. Tomando-se a tela toda, cabe tudo, e o pé deixa de ser sorte.
  auto tela = ftxui::ScreenInteractive::Fullscreen();
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
  // A tela cheia se desfaz ao sahir, e a mensagem iria com ella. Repete-se pois
  // o relatorio em texto, que fica no écran depois do programa: quem foi
  // installar o que falta ha de o ter debaixo dos olhos, e não de memoria.
  std::cerr << tui::texto_do_relatorio(relatorio);
  return 1;
}

}  // namespace

int main(int argc, char** argv) {
  const nucleo::Relatorio relatorio =
      nucleo::sondar(nucleo::inquerito_do_systema());

  // O modo de diagnostico: texto puro, tela nenhuma, e codigo differente de
  // zero havendo impedimento, para que sirva de guarda em script.
  if (argc > 1 && std::string_view(argv[1]) == "--sonda") {
    std::cout << tui::texto_do_relatorio(relatorio);
    return relatorio.ha_impedimento() ? 1 : 0;
  }

  if (relatorio.ha_impedimento()) {
    // Sem terminal não se ergue tela alguma: quem redirigiu a sahida a arquivo
    // receberia lixo de escape e nenhuma tecla poderia dar. Vae o texto ao
    // stderr, que é onde o diagnostico se procura, e sahe-se.
    if (isatty(STDOUT_FILENO) == 0) {
      std::cerr << tui::texto_do_relatorio(relatorio);
      return 1;
    }
    return recusar_e_sahir(relatorio);
  }

  // O aviso NÃO interrompe: escreve-se e o tocador sobe. Tecla alguma se pede,
  // porque ella se pediria em toda abertura, e o que se aperta todo dia
  // aprende-se a apertar sem ler.
  const std::string avisos = tui::texto_dos_avisos(relatorio);
  if (!avisos.empty()) std::cerr << avisos;

  // A fila vem da linha de commando. Não ha varredura de acervo ainda (issue
  // #34), e por isso é assim que uma faixa entra: `mysong caminho.mp3 outro.mp3`.
  std::vector<std::string> faixas;
  for (int i = 1; i < argc; ++i) faixas.emplace_back(argv[i]);
  return erguer_tocador(faixas);
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
