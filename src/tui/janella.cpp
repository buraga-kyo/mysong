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
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <filesystem>
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

#include <algorithm>

#include "api/mpris.hpp"

#include "nucleo/analisador.hpp"
#include "nucleo/capa.hpp"
#include "nucleo/aquisicao.hpp"
#include "nucleo/fila.hpp"
#include "nucleo/letra.hpp"
#include "nucleo/marca.hpp"
#include "nucleo/motor.hpp"
#include "nucleo/tocador.hpp"
#include "nucleo/varredura.hpp"
#include "nucleo/sonda.hpp"
#include "tui/commando.hpp"
#include "tui/espectro.hpp"
#include "tui/navegador.hpp"
#include "tui/tabella.hpp"
#include "tui/tela_requisitos.hpp"
#include "tui/transporte.hpp"

namespace api = mysong::api;
namespace nucleo = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

// caminho_do_indice — `$XDG_DATA_HOME/mysong/indice.sqlite3`, e sem elle
// `~/.local/share/...`. Cria-se o directorio com modo 0700, como no precedente
// do agenda_index.py: o que o operador escuta é dado d'elle, e não do mundo.
std::filesystem::path caminho_do_indice() {
  const char* dados = std::getenv("XDG_DATA_HOME");
  std::filesystem::path raiz;
  if (dados != nullptr && dados[0] != '\0') {
    raiz = std::filesystem::path(dados);
  } else {
    const char* casa = std::getenv("HOME");
    if (casa == nullptr) return {};
    raiz = std::filesystem::path(casa) / ".local" / "share";
  }
  const std::filesystem::path pasta = raiz / "mysong";
  std::error_code erro;
  std::filesystem::create_directories(pasta, erro);
  std::filesystem::permissions(pasta, std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace, erro);
  return pasta / "indice.sqlite3";
}

// raiz_do_acervo — `$MYSONG_ACERVO`, e sem ella `~/Música`. A variavel existe para
// que o operador com monte de rede não tenha de mover o acervo para casa.
std::filesystem::path raiz_do_acervo() {
  const char* posto = std::getenv("MYSONG_ACERVO");
  if (posto != nullptr && posto[0] != '\0') return std::filesystem::path(posto);
  const char* casa = std::getenv("HOME");
  if (casa == nullptr) return {};
  return std::filesystem::path(casa) / "Música";
}

// varre_em_fio — a varredura em fio proprio, passo a passo, sem travar a tela. A
// conducção por passos da issue #34 existe justamente para isto: o fio pode
// parar entre dous passos, e a bandeira `sahir` é onde elle olha.
void varre_em_fio(const std::filesystem::path& banco,
                  const std::filesystem::path& acervo,
                  const std::atomic<bool>* sahir,
                  std::atomic<bool>* concluida) {
  nucleo::Varredura varredura(banco, {acervo});
  while (!sahir->load() && varredura.passo()) {
  }
  concluida->store(true);
}

// assignatura_do_visivel — uma cadeia barata que resume TUDO o que a tela mostra. O fio do
// relogio sómente pede repintura quando ella muda.
//
// Sem isto, medido n'um pty de quarenta por cento e vinte: cento e trinta e oito KiB por
// segundo com a fila VAZIA e nada a tocar. São sete KiB por quadro a vinte quadros por
// segundo, a tela inteira, repintada porque o relogio bateu. Dentro de tmux é o cursor do
// operador a piscar, porque o tmux ha de reparsear e reposicionar vinte vezes por segundo
// para sempre.
//
// A posição entra em SEGUNDOS inteiros, e não em decimos: a barra e o relogio mostram
// segundos, e a fracção mudaria a assignatura sem mudar um pixel.
std::string assignatura_do_visivel(nucleo::Tocador& tocador,
                                   const tui::Navegador& navegador,
                                   int digita, const std::string& termo_em_curso,
                                   const std::string& aviso, bool mostra_letra,
                                   bool varrida) {
  std::string marca;
  marca.reserve(128);
  marca += std::to_string(static_cast<int>(tocador.estado()));
  marca += ':';
  marca += std::to_string(static_cast<long>(tocador.posicao()));
  marca += ':';
  marca += std::to_string(static_cast<long>(tocador.duracao()));
  marca += ':';
  marca += std::to_string(tocador.volume());
  marca += ':';
  const nucleo::Fila& fila = tocador.fila();
  marca += std::to_string(fila.tamanho());
  marca += ':';
  marca += fila.vazia() ? std::string() : std::string(fila.corrente());
  marca += ':';
  // As bandas SÓMENTE quando o espectro está á vista. Postas sempre, o painel da letra
  // pagava a animação que não mostrava: medido em cento e trinta e dous KiB por segundo,
  // contra dous e sete pausado. Assignatura ha de resumir o que se VÊ, e não o que ha.
  if (!mostra_letra)
    for (const float banda : tocador.bandas())
      marca += static_cast<char>(
          static_cast<int>((banda < 0.0f ? 0.0f : (banda > 1.0f ? 1.0f : banda)) *
                           99.0f) + 32);
  marca += ':';
  marca += std::to_string(digita);
  marca += termo_em_curso;
  marca += ':';
  marca += aviso;
  marca += mostra_letra ? 'L' : 'e';
  marca += varrida ? 'v' : '.';
  marca += ':';
  marca += std::to_string(static_cast<int>(navegador.secao()));
  marca += ':';
  marca += std::to_string(navegador.eleito());
  marca += ':';
  marca += std::to_string(navegador.vista().size());
  marca += ':';
  marca += navegador.termo();
  for (const std::string& degrau : navegador.trilha()) marca += degrau;
  return marca;
}

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
void cumprir(const tui::Ordem& ordem, nucleo::Tocador& tocador,
             std::atomic<bool>& sahir) {
  switch (ordem.verbo) {
    case tui::Verbo::Nada: break;
    case tui::Verbo::Pausar: tocador.pausar(); break;
    case tui::Verbo::Retomar: tocador.retomar(); break;
    case tui::Verbo::Proxima: tocador.proxima(); break;
    case tui::Verbo::Anterior: tocador.anterior(); break;
    case tui::Verbo::Buscar: tocador.buscar(ordem.alvo); break;
    case tui::Verbo::Volume: tocador.volume(static_cast<int>(ordem.alvo)); break;
    case tui::Verbo::Sahir: sahir.store(true); break;
    // Os verbos da navegação não passam por aqui: quem os cumpre é o navegador,
    // e elle não é do tocador. Ficam nomeados um a um para que o `switch`
    // continue exhaustivo, e para que verbo novo acenda aviso e não silencio.
    case tui::Verbo::Desce:
    case tui::Verbo::Sobe:
    case tui::Verbo::AoPrincipio:
    case tui::Verbo::AoFim:
    case tui::Verbo::Entra:
    case tui::Verbo::Volta:
    case tui::Verbo::AbreBusca:
    case tui::Verbo::Varre:
    case tui::Verbo::AbreBaixa:
    case tui::Verbo::TrocaLetra:
    case tui::Verbo::AbreProcura:
      break;
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

  // O MPRIS. Barramento ausente não é falha: diz-se uma vez e o tocador segue, que é o
  // mesmo padrão do analisador da issue #5.
  api::CasaDoMpris mpris(tocador);
  if (!mpris.viva())
    std::cerr << "mysong: sem MPRIS: " << mpris.razao() << "\n";

  for (const std::string& faixa : faixas) tocador.fila().junta(faixa);
  if (!tocador.fila().vazia()) tocador.tocar_corrente();

  // O ÍNDICE e a VARREDURA. A varredura corre em fio proprio e o navegador
  // recarrega quando ella concluir: assim a tela abre de pronto, com o acervo da
  // corrida anterior, em vez de esperar pelo disco.
  const std::filesystem::path banco = caminho_do_indice();
  nucleo::Biblioteca livraria(banco);
  tui::Navegador navegador(livraria);
  std::atomic<bool> varrida{false};
  bool recarregado = false;

  auto tela = ftxui::ScreenInteractive::Fullscreen();
  // O RATO NÃO SE RASTREIA. O FTXUI liga-o por defeito, e liga-o no modo mais largo
  // que existe: `ESC[?1003h`, que manda uma sequencia de escape a cada MEXIDA do rato,
  // ainda que ninguem carregue em botão algum. Dentro de tmux essas sequencias vazam, e
  // o que o operador vê é o teclado a cuspir lixo e a comer teclas.
  //
  // E esta Casa não usa rato: tratador de rato algum se ligou em issue alguma. Pagar o
  // custo inteiro de um recurso que não se consome não é neutro, é este defeito.
  tela.TrackMouse(false);
  std::atomic<bool> sahir{false};
  // O MODO de digitar tem DOUS destinos: a busca e a URL. Um enum, e não dous
  // booleanos: dous booleanos admittem o estado «ambos», que não existe.
  enum class Digita { Nada, Busca, Url } digita = Digita::Nada;
  std::string termo_em_curso;
  std::string aviso_da_baixa;
  std::size_t primeira_linha = 0;
  // A LETRA carrega-se do disco UMA vez por faixa, e não a cada quadro: ler
  // arquivo vinte vezes por segundo seria gastar disco para nada. A faixa de que
  // ella é guarda-se ao lado, e é a mudança d'essa que dispara a releitura.
  std::vector<nucleo::LinhaDaLetra> letra;
  std::string letra_de_qual;
  bool mostra_letra = false;
  nucleo::Galeria galeria;  // a capa converte-se uma vez por album e por tamanho

  // Os fios de fundo são POSSUIDOS, e juntam-se antes de esta pilha se desfazer. Antes
  // corriam soltos por `detach()`, e o corpo d'elles referencia objectos d'esta pilha:
  // sahindo o programma primeiro, liam memoria morta. Fio solto que aponta para pilha
  // alheia não se justifica, e agora não ha nenhum.
  // A ultima assignatura do que se vê. Vazia de saida, para que o primeiro quadro sahia.
  std::string ultima_assignatura;
  std::vector<std::thread> ao_fundo;
  ao_fundo.emplace_back(varre_em_fio, banco, raiz_do_acervo(), &sahir, &varrida);

  auto pintor = ftxui::Renderer([&] {
    const tui::Retracto retracto = retracto_do(tocador);
    const int col = ftxui::Terminal::Size().dimx;
    const int lin = ftxui::Terminal::Size().dimy;
    const std::size_t larg = col > 4 ? static_cast<std::size_t>(col - 4) : 1;
    // A tabella toma o que sobra em altura: cinco linhas de guarnição (marca,
    // trilha, espectro de oito, transporte, rodapé) mais a orla.
    const std::size_t alt_tab = lin > 16 ? static_cast<std::size_t>(lin - 16) : 1;
    primeira_linha = tui::primeira_a_mostrar(navegador.eleito(),
                                             navegador.vista().size(), alt_tab,
                                             primeira_linha);
    // O painel NOW PLAYING toma um quinto da largura, e nunca mais de vinte
    // collunhas nem menos de oito: a arte quer quadrado, e o quadrado n'um terminal
    // pede duas linhas por collunha, donde a altura sahe da largura e não ao
    // contrario. Terminal apertado não mostra capa alguma, que roubar da tabella
    // para mostrar arte seria trocar o que serve pelo que enfeita.
    const std::size_t larg_capa =
        larg >= 60 ? std::min<std::size_t>(20, larg / 5) : 0;
    const std::size_t alt_capa =
        larg_capa == 0 ? 0 : std::min<std::size_t>(alt_tab, larg_capa / 2 + 1);
    const std::size_t reservado_capa = larg_capa == 0 ? 0 : larg_capa + 1;
    const std::size_t larg_tab =
        larg > 11 + reservado_capa ? larg - 11 - reservado_capa : 1;

    std::string trilha = "ARTISTS";
    for (const std::string& degrau : navegador.trilha())
      trilha += "  \ue0b1  " + degrau;
    if (digita == Digita::Busca) trilha = "/" + termo_em_curso;
    else if (digita == Digita::Url) trilha = "URL: " + termo_em_curso;
    else if (!navegador.termo().empty()) trilha += "   [" + navegador.termo() + "]";
    if (!varrida.load()) trilha += "   (a varrer o acervo...)";
    if (!aviso_da_baixa.empty()) trilha += "   " + aviso_da_baixa;

    // A letra relê-se sómente quando a faixa muda.
    if (retracto.titulo != letra_de_qual) {
      letra_de_qual = retracto.titulo;
      letra = retracto.titulo.empty()
                  ? std::vector<nucleo::LinhaDaLetra>()
                  : nucleo::le_lrc_do_disco(retracto.titulo);
    }
    const tui::Quadro quadro = tui::compor(tocador.bandas(), larg, 8);
    return ftxui::vbox({
               ftxui::text(std::string(nucleo::marca())) | ftxui::bold,
               ftxui::text(trilha) | ftxui::dim,
               ftxui::hbox({
                   tui::elemento_da_barra(navegador),
                   ftxui::text("  "),
                   tui::elemento_da_tabella(navegador, primeira_linha, alt_tab,
                                            larg_tab),
                   ftxui::text(" "),
                   tui::elemento_da_capa(
                       galeria.capa(retracto.titulo, larg_capa, alt_capa),
                       larg_capa, alt_capa),
               }),
               ftxui::text(""),
               mostra_letra
                   ? tui::elemento_da_letra(
                         letra,
                         nucleo::linha_corrente(letra, retracto.posicao), 8, larg)
                   : tui::elemento_do_espectro(quadro),
               tui::elemento_do_transporte(retracto, larg),
               ftxui::text("↑↓ anda · → entra · ← volta · / busca · r varre · b baixa"
                           " · l letra · espaço pausa · n/p faixa · ,. busca no som"
                           " · +- volume · q sahe") |
                   ftxui::dim,
           }) |
           ftxui::border;
  });

  auto janella = ftxui::CatchEvent(pintor, [&](const ftxui::Event& tecla) {
    // O MODO DE DIGITAR trata-se PRIMEIRO, e por inteiro: assim não ha caminho
    // por onde uma tecla chegue ás duas leituras.
    if (digita != Digita::Nada) {
      if (tecla == ftxui::Event::Escape) {
        digita = Digita::Nada;
        termo_em_curso.clear();
        return true;
      }
      if (tecla == ftxui::Event::Return) {
        const Digita era = digita;
        digita = Digita::Nada;
        if (era == Digita::Busca) {
          navegador.filtra(termo_em_curso);
        } else if (!termo_em_curso.empty()) {
          // A baixa corre em fio SOLTO, e de proposito: ella pode levar minutos,
          // e o operador ha de continuar a ouvir o que já tem. O fio não toca a
          // tela; deixa recado no aviso, e a tela lê-o.
          const std::string url = termo_em_curso;
          aviso_da_baixa = "a baixar...";
          ao_fundo.emplace_back([&aviso_da_baixa, &varrida, &recarregado, url,
                                 banco, &sahir] {
            std::filesystem::path ficou;
            nucleo::Pedido pedido;
            pedido.url = url;  // e o resto vem da rede, que o operador não disse
            const nucleo::Colheita fim =
                nucleo::baixa(raiz_do_acervo(), pedido, &ficou);
            aviso_da_baixa = nucleo::razao_da_colheita(fim);
            if (fim == nucleo::Colheita::Colhido) {
              varrida.store(false);  // ha faixa nova: varre-se outra vez
              recarregado = false;
              nucleo::Varredura outra(banco, {raiz_do_acervo()});
              while (!sahir.load() && outra.passo()) {
              }
              varrida.store(true);
            }
          });
        }
        return true;
      }
      if (tecla == ftxui::Event::Backspace) {
        if (!termo_em_curso.empty()) termo_em_curso.pop_back();
        return true;
      }
      if (tecla.is_character()) {
        termo_em_curso += tecla.character();
        return true;
      }
      return true;  // dentro do modo, tecla alguma sahe para fóra
    }

    const tui::Ordem ordem =
        tui::ordem_da_tecla(tecla, retracto_do(tocador), false);
    switch (ordem.verbo) {
      case tui::Verbo::Nada: return false;  // tecla alheia segue
      case tui::Verbo::Desce: navegador.desce(); return true;
      case tui::Verbo::Sobe: navegador.sobe(); return true;
      case tui::Verbo::AoPrincipio: navegador.ao_principio(); return true;
      case tui::Verbo::AoFim: navegador.ao_fim(); return true;
      case tui::Verbo::Volta: navegador.volta(); return true;
      case tui::Verbo::AbreBusca:
        digita = Digita::Busca;
        termo_em_curso.clear();
        return true;
      case tui::Verbo::AbreBaixa:
        digita = Digita::Url;
        termo_em_curso.clear();
        return true;
      case tui::Verbo::TrocaLetra:
        mostra_letra = !mostra_letra;
        return true;
      case tui::Verbo::Varre:
        if (varrida.load()) {  // uma varredura por vez, e não vinte
          varrida.store(false);
          recarregado = false;
          ao_fundo.emplace_back(varre_em_fio, banco, raiz_do_acervo(), &sahir,
                                &varrida);
        }
        return true;
      case tui::Verbo::Entra:
        // O navegador diz SE era faixa; a decisão de tocar é d'esta funcção, que
        // é quem tem o tocador na mão.
        if (navegador.entra()) {
          const std::string caminho = navegador.caminho_eleito();
          if (!caminho.empty()) {
            tocador.fila().junta(caminho);
            tocador.fila().ir_para(tocador.fila().tamanho() - 1);
            tocador.tocar_corrente();
          }
        }
        return true;
      default:
        cumprir(ordem, tocador, sahir);
        if (sahir.load()) tela.Exit();
        return true;
    }
  });

  // O RELOGIO. Vive em fio proprio porque `tela.Loop` não devolve até se sahir, e
  // o `pulsa` do tocador tem de correr entre quadros: é elle que drena os
  // pregões do mpv e faz a posição andar. O fio não toca a tela: pede-lhe que
  // repinte, e a tela é que serializa.
  std::thread relogio([&] {
    while (!sahir.load()) {
      tocador.pulsa();
      analisador.pulsa();
      mpris.pulsa();
      // A varredura concluiu: o navegador recarrega UMA vez. A bandeira impede
      // que elle releia o banco vinte vezes por segundo para sempre.
      if (varrida.load() && !recarregado) {
        recarregado = true;
        livraria.reabre();
        navegador.recarrega();
      }
      // SÓMENTE quando o que se vê muda. Parado, isto não pede repintura alguma, e a
      // tela escreve zero: é a correcção da issue #48.
      const std::string agora = assignatura_do_visivel(
          tocador, navegador, static_cast<int>(digita), termo_em_curso,
          aviso_da_baixa, mostra_letra, varrida.load());
      if (agora != ultima_assignatura) {
        ultima_assignatura = agora;
        tela.PostEvent(ftxui::Event::Custom);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(MILESIMOS_DO_QUADRO));
    }
  });

  tela.Loop(janella);
  sahir.store(true);  // a sahida pela tela tambem para o relogio
  relogio.join();
  // Os fios de fundo esperam-se TODOS: elles têm referencia para bandeiras e para o
  // banco, que vivem nesta pilha. Deixar um solto é fio a ler memoria de quadro já
  // desfeito, e isso não perdoa.
  for (std::thread& fio : ao_fundo)
    if (fio.joinable()) fio.join();
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
  tela.TrackMouse(false);  // idem: esta tela é a primeira que o operador vê
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
