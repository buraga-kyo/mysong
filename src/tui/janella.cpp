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
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#include <string>

#include <unistd.h>

#include <curl/curl.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/string.hpp>
#include <ftxui/screen/terminal.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>

#include "api/mpris.hpp"
#include "api/socket.hpp"

#include "nucleo/ajustes.hpp"
#include "nucleo/analisador.hpp"
#include "nucleo/caa.hpp"
#include "nucleo/capa.hpp"
#include "nucleo/catalogo.hpp"
#include "nucleo/estaleiro.hpp"
#include "nucleo/aquisicao.hpp"
#include "nucleo/fila.hpp"
#include "nucleo/letra.hpp"
#include "nucleo/lixeira.hpp"
#include "nucleo/letreiro.hpp"
#include "nucleo/lousa.hpp"
#include "nucleo/linha.hpp"
#include "nucleo/marca.hpp"
#include "nucleo/motor.hpp"
#include "nucleo/rol.hpp"
#include "nucleo/video.hpp"
#include "nucleo/tocador.hpp"
#include "nucleo/varredura.hpp"
#include "nucleo/sonda.hpp"
#include "tui/cabecalho.hpp"
#include "tui/commando.hpp"
#include "tui/correio.hpp"
#include "tui/espectro.hpp"
#include "tui/foco.hpp"
#include "tui/letra_viva.hpp"
#include "nucleo/onda.hpp"
#include "tui/ajuda.hpp"
#include "tui/menu_contexto.hpp"
#include "tui/navegador.hpp"
#include "tui/prompt.hpp"
#include "tui/rato.hpp"
#include "tui/sala.hpp"
#include "tui/tabella.hpp"
#include "tui/tela_requisitos.hpp"
#include "tui/transporte.hpp"
#include "tui/vigilia.hpp"

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

// caminho_das_listas — `rol.sqlite3` ao lado do índice, e NÃO dentro d'elle: o
// índice é reconstruido a cada varredura por temporario e rename, e taboa de lista
// lá dentro sahiria com a varredura.
std::filesystem::path caminho_das_listas(const std::filesystem::path& indice) {
  if (indice.empty()) return {};
  return indice.parent_path() / "rol.sqlite3";
}

// raiz_do_soquete — onde o soquete de commando da janella do video mora.
// `$XDG_RUNTIME_DIR` primeiro, que é o logar que o systema apaga ao fim da sessão;
// `/tmp` sem elle, que soquete tem de morar em algum logar e recusar abrir video
// por falta de directorio de tempo seria recusa que ninguem entende.
std::filesystem::path raiz_do_soquete() {
  const char* posto = std::getenv("XDG_RUNTIME_DIR");
  if (posto != nullptr && posto[0] != '\0') return std::filesystem::path(posto);
  return std::filesystem::path("/tmp");
}

// QUANTOS achados a busca na rede pede. Quinze: cabe n'uma tabella de terminal sem
// rolar muito, e o `--flat-playlist` faz d'isso uma sonda de rede só.
constexpr int ACHADOS_POR_BUSCA = 15;

// assignatura_do_visivel — uma cadeia barata que resume TUDO o que a tela mostra. O fio do
// relogio sómente pede repintura quando ella muda.
//
// Sem isto, medido n'um pty de quarenta por cento e vinte: cento e trinta e oito KiB por
// segundo com a fila VAZIA e nada a tocar. São sete KiB por quadro a vinte quadros por
// segundo, a tela inteira, repintada porque o relogio bateu.
//
// DO PISCAR, e do que a medida achou (issue #78). Esta linha dizia, sem qualificar, que
// «dentro de tmux é o cursor do operador a piscar»; foi-se medir, e é falso para o painel
// que tem o foco. O FTXUI manda «ESC[?25l» na cabeça de CADA quadro, e só torna a mostrar
// o cursor quando algum nó do documento pede foco; esta obra só o pede no caret do campo
// de digitar. Cinco segundos de musica dão cem quadros, cem «ESC[?25l» e «ESC[?25h»
// nenhum, e o tmux não manda escape de cursor algum ao terminal de fóra.
//
// O que pisca é o cursor de OUTRO painel. Tocando o mysong n'um painel e trabalhando o
// operador n'outro, o tmux manda «cnorm» quarenta vezes por segundo ao terminal de fóra e
// arrasta o cursor VISIVEL do painel d'elle por dous mil e quinhentos reposicionamentos em
// cinco segundos. Aquelle cursor é do painel activo, e o mysong não é dono d'elle:
// esconder mais o nosso não apaga o alheio. O remedio foi o da issue #82, e mora
// na VIGILIA: pedido o foco ao terminal (modo 1004), perdendo o painel os olhos
// o relogio dorme e batida alguma pede repintura; sem repintura nossa, o tmux
// não tem quadro que pintar lá fóra, e cnorm algum arrasta o cursor alheio.
//
// A posição entra em SEGUNDOS inteiros, e não em decimos: a barra e o relogio mostram
// segundos, e a fracção mudaria a assignatura sem mudar um pixel.
// O que ella NÃO olha, e por que. A secção, o eleito, a trilha, o termo, o modo de
// digitar e o recado da rede mudam SÓMENTE em resposta a tecla, e o FTXUI repinta
// depois de toda tecla tratada, por construcção: o `RunOnce` d'elle chama `Draw`
// sempre que executou tarefa. Postos aqui, esse estado seria LIDO d'este fio e MUTADO
// no fio da tela, e cadeia lida enquanto outro fio a muta não é engano benigno.
std::string assignatura_do_visivel(nucleo::Tocador& tocador,
                                   const std::string& recado, bool mostra_letra,
                                   bool varrida, unsigned long geracao,
                                   bool video) {
  // O instante sahe de UMA tomada da tranca do tocador (issue #50): cada
  // campo do mesmo momento, e a faixa já copiada, sem vista crua da fila.
  const nucleo::Retracto agora = tocador.retracto();
  std::string marca;
  marca.reserve(128);
  marca += std::to_string(static_cast<int>(agora.estado));
  marca += ':';
  marca += std::to_string(static_cast<long>(agora.posicao));
  marca += ':';
  marca += std::to_string(static_cast<long>(agora.duracao));
  marca += ':';
  marca += std::to_string(agora.volume);
  marca += ':';
  marca += std::to_string(agora.tamanho);
  marca += ':';
  // Os DOUS MODOS (issue #62). Sem elles aqui, teclar `z` com a musica pausada
  // mudava o modo e a fita ficava como estava até o operador carregar n'outra
  // tecla por acaso: é o defeito que a issue #49 já apanhou uma vez n'esta Casa.
  marca += agora.embaralhado ? 'E' : '.';
  marca += static_cast<char>('0' + static_cast<int>(agora.repeticao));
  marca += ':';
  marca += agora.faixa;
  marca += ':';
  // As bandas entram SEMPRE, desde a issue #109. Até ella, a letra tomava o logar
  // do espectro e o painel da letra pagava a animação que não mostrava; agora a
  // letra mora POR CIMA do espectro, e o espectro está sempre á vista. É tambem
  // por estas bandas que o rio anda entre um segundo e o seguinte: a posição
  // entra na marca em segundos inteiros, e sem ellas o rio subiria aos saltos.
  for (const float banda : tocador.bandas())
    marca += static_cast<char>(
        static_cast<int>((banda < 0.0f ? 0.0f : (banda > 1.0f ? 1.0f : banda)) *
                         99.0f) + 32);
  marca += ':';
  marca += recado;
  marca += mostra_letra ? 'L' : 'e';
  marca += varrida ? 'v' : '.';
  // A GERAÇÃO do correio. Sem ella, achado que chegasse com a tela quieta não pediria
  // repintura alguma, e o fio da tela nunca colheria o recado: a busca respondia, e
  // nada apparecia até o operador carregar n'uma tecla por acaso.
  marca += ':';
  marca += std::to_string(geracao);
  // A janella do video acaba POR SI quando a faixa acaba, ou quando o operador a
  // fecha com o rato. Nenhuma d'essas duas cousas é tecla, donde sem esta linha a
  // trilha continuaria a dizer «video: tal» depois de a janella se ter ido.
  marca += video ? 'V' : '.';
  return marca;
}

// retracto_do — colhe o instante do tocador n'uma cópia. O nucleo colhe o seu
// proprio retracto de UMA tomada da tranca (issue #50); aqui só se veste a
// tela por cima d'elle.
tui::Retracto retracto_do(nucleo::Tocador& tocador,
                          nucleo::Projector& projector) {
  tui::Retracto retracto;
  // A janella entra no retracto, e não n'uma consulta á parte: a taboada das teclas
  // é funcção PURA do retracto, e o que ella não vê n'elle não pode governar.
  retracto.video = projector.rodando();
  retracto.video_pausada = projector.pausada();
  const nucleo::Retracto agora = tocador.retracto();
  retracto.estado = agora.estado;
  retracto.posicao = agora.posicao;
  retracto.duracao = agora.duracao;
  retracto.volume = agora.volume;
  retracto.mudo = agora.mudo;
  retracto.tamanho = agora.tamanho;
  retracto.embaralhado = agora.embaralhado;
  retracto.repeticao = agora.repeticao;
  if (agora.tamanho > 0) {
    retracto.indice = agora.indice;
    retracto.titulo = agora.faixa;
  }
  return retracto;
}

// apaga_a_faixa — o arquivo á LIXEIRA, e a faixa fóra do índice e de todas as
// listas. As tres peças juntam-se aqui porque é aqui que as tres se têm na mão.
// Nada se desliga do disco: o que se apaga por engano volta pelo gerenciador.
std::string apaga_a_faixa(const std::string& caminho,
                          nucleo::Biblioteca& livraria,
                          nucleo::Roleiro& roleiro) {
  const nucleo::DaLixeira desfecho = nucleo::manda_a_lixeira(caminho);
  if (!desfecho.feita) return "não se apagou: " + desfecho.razao;
  livraria.esquece(caminho);
  roleiro.retira_de_todos(caminho);
  std::string recado = "«" + desfecho.nome + "» foi para a lixeira";
  if (desfecho.levou_a_letra) recado += ", com a letra";
  if (desfecho.copiada) recado += " (outro volume: copiada e apagada)";
  return recado;
}

// renomeia_a_faixa — o titulo na ETIQUETA primeiro, e no índice depois. N'esta
// ordem, e não na contraria: gravado o índice antes, a etiqueta que recusasse
// deixava a pauta a mostrar nome que o arquivo não tem, e a proxima varredura
// desfazia-o sem o operador entender porquê.
std::string renomeia_a_faixa(const std::string& caminho,
                             const std::string& titulo,
                             nucleo::Biblioteca& livraria) {
  const nucleo::DoTitulo desfecho = nucleo::renomeia_titulo(caminho, titulo);
  if (!desfecho.feito) return "não se renomeou: " + desfecho.razao;
  livraria.muda_o_titulo(caminho, desfecho.titulo);
  return "agora chama-se «" + desfecho.titulo + "»";
}

// cumprir — a ordem em chamada. O `switch` é exhaustivo de proposito: verbo novo
// na taboada acende aviso do compilador aqui, e não passa calado.
// O ROTEAMENTO das ordens de transporte. Havendo janella de video de pé, é ELLA
// que pausa, retoma, busca e muda de volume: o motor de audio está calado, e mandar
// a ordem a quem está calado seria a tecla não fazer nada. Sem janella, vae ao
// motor, que é o caminho de sempre.
void cumprir(const tui::Ordem& ordem, nucleo::Tocador& tocador,
             nucleo::Projector& projector, std::atomic<bool>& sahir) {
  const bool na_janella = projector.rodando();
  switch (ordem.verbo) {
    case tui::Verbo::Nada: break;
    case tui::Verbo::Pausar:
      if (na_janella) projector.pausar();
      else tocador.pausar();
      break;
    case tui::Verbo::Retomar:
      if (na_janella) projector.retomar();
      else tocador.retomar();
      break;
    case tui::Verbo::Proxima: tocador.proxima(); break;
    case tui::Verbo::Anterior: tocador.anterior(); break;
    case tui::Verbo::Buscar:
      if (na_janella) {
        if (ordem.relativo) projector.buscar_relativo(ordem.alvo);
        else projector.buscar(ordem.alvo);
      } else {
        tocador.buscar(ordem.alvo);
      }
      break;
    case tui::Verbo::Volume:
      if (na_janella) projector.volume(static_cast<int>(ordem.alvo));
      else tocador.volume(static_cast<int>(ordem.alvo));
      break;
    case tui::Verbo::Sahir: sahir.store(true); break;
    // Os DOUS MODOS (issue #62). Alternar e ciclar são punhos do tocador, e não
    // «ler o retracto e depois escrever»: entre a leitura e a escripta caberia o
    // socket ou o barramento, e a tecla assentaria o contrario do que se viu.
    case tui::Verbo::Embaralhar: tocador.alterna_embaralhar(); break;
    case tui::Verbo::Repetir: tocador.cicla_repetir(); break;
    // O MUDO (issue #106) cala o MOTOR de audio, e não a janella do video: essa
    // tem o volume d'ella pelo soquete, e calá-la sem lh'o dizer deixaria o
    // segundo F9 a devolver um volume que ella nunca teve.
    case tui::Verbo::Mudo: tocador.alterna_mudo(); break;
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
    case tui::Verbo::AbreRois:
    case tui::Verbo::CriaRol:
    case tui::Verbo::RenomeiaRol:
    case tui::Verbo::ApagaRol:
    case tui::Verbo::JuntaAoRol:
    case tui::Verbo::RetiraDoRol:
    case tui::Verbo::SobeNoRol:
    case tui::Verbo::DesceNoRol:
    case tui::Verbo::AbreVideo:
    case tui::Verbo::AbreCatalogo:
    case tui::Verbo::BaixaTudo:
    case tui::Verbo::TrocaFonte:
    case tui::Verbo::RenomeiaFaixa:
    case tui::Verbo::ApagaFaixa:
    case tui::Verbo::Ajuda:
      break;
  }
}

// AS DICAS do rodapé, n'uma linha. Ficam aqui, e não no meio da composição:
// ellas mudam a cada issue que dá tecla nova, e assim quem as procura sabe
// onde estão. As tres primeiras são as da issue #102, que são as unicas
// teclas d'esta tela que ninguem conhece de outra casa.
// Coube em CENTO E QUINZE collunhas emquanto as dicas eram nove, e o piso do
// esboço é cento e vinte. Linha que transborde apara-se em silencio, e o que se
// perde é o FIM; por isso o fim é o README, que é onde mora o que não coube.
//
// As seis de funcção (issue #106) entraram, e para lhes caber o logar sahiram
// o Enter, o espaço e o `n`/`p`, que dizem o que o F7, o F6 e o F8 já dizem, e
// sahiu o `o` da vista, que a chapa por cima da pauta annuncia por si.
//
// O `m` do menu (issue #96) entrou, e para lhe caber o logar o ponto do meio
// cedeu o passo a DOUS ESPAÇOS, que é o separador do proprio esboço da tela.
//
// E declara-se o que se MEDIU, que a promessa das cento e vinte já se não
// cumpre: com as setas e o Enter da issue #107 a linha ficou em cento e
// quarenta e duas collunhas, e o `m` com o ponto punha-a em cento e cincoenta
// e uma. Com os dous espaços mede cento e quarenta, que é MENOS do que ella
// media sem o `m`; menos do que isso pedia apagar dica que uma lavra irmã
// acabou de escrever. Em tela de cento e vinte perde-se o FIM, e o fim é o
// README, que é justamente onde mora o que na linha não coube.
constexpr const char* kDicas =
    "? HELP  ↑↓←→ anda  Enter aperta  1 2 3 abas  Tab cicla"
    "  F6 F7 F8 transporte  F9 mudo  F10 F11 volume  q sahe";

// A CADENCIA do relogio. Cincoenta milesimos, que são vinte quadros por segundo:
// o bastante para a barra andar sem salto visivel, e longe do sessenta que faz a
// fita tremer por diff de buffer. O risco do tremor está declarado no plano, e
// esta é a primeira defesa contra elle.
constexpr int MILESIMOS_DO_QUADRO = 50;

// encommenda_do_catalogo — o Pedido que uma faixa do catalogo dá. A URL vae VAZIA de
// proposito: é isso que manda a aquisição buscar o audio por si e casá-lo pela
// duração. E as etiquetas vêm todas do CATALOGO, que é a razão de esta tarefa
// existir: o metadado do YouTube põe o nome do canal por artista.
//
// O ALBUM é o nome da LISTA, e fica declarado por que: a pagina publica de embutir
// não publica album algum, e o disco de onde a faixa sahiu o Spotify não dá sem chave
// nem conta. Nome de lista por album é o melhor que ha sem quebrar a fronteira.
nucleo::Pedido encommenda_do_catalogo(const nucleo::FaixaDoCatalogo& faixa,
                                      const std::string& lista) {
  nucleo::Pedido pedido;
  pedido.artista = faixa.artista;
  pedido.album = lista;
  pedido.titulo = faixa.titulo;
  pedido.numero = faixa.numero;
  // O id do track vae junto (issue #57): é por elle que o MusicBrainz acha a
  // gravação exacta, e o album e o numero acima viram canonicos quando ella casa.
  pedido.id_spotify = faixa.id_do_track;
  // Milesimos a segundos, arredondando ao mais proximo: truncar perderia meio segundo
  // em cada faixa, e a tolerancia do casamento é de doze.
  pedido.duracao = (faixa.duracao_ms + 500) / 1000;
  // A FONTE estampa-se (issue #56): pedido sem URL busca na fonte d'elle, e a do
  // catalogo é o Spotify, que no nucleo mapeia para o ytsearch de sempre.
  pedido.fonte = nucleo::Fonte::Spotify;
  return pedido;
}

// erguer_tocador — o laço de verdade. Ergue o motor, o tocador e o analisador,
// enche a fila com o que veio da linha de commando, e pinta a barra de baixo com
// o espectro por cima. Esta funcção NÃO se prova em bateria: ella abre terminal,
// abre som e depende de relogio. O que se prova são as duas peças que ella usa,
// e é por isso que ellas vivem fóra d'aqui.
int erguer_tocador(const std::vector<std::string>& faixas,
                   const nucleo::Ajustes& ajustes) {
  // O acervo em cópia: dous fios o lêem, e a cópia n'esta pilha vive mais que
  // elles, que se juntam antes de esta funcção voltar.
  const std::filesystem::path acervo = ajustes.acervo.valor;
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
  // O volume dos ajustes entra ANTES da primeira faixa: posto depois, ella já
  // teria arrancado no volume de fabrica, e ouvir-se-ia o salto.
  tocador.volume(ajustes.volume.valor);
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

  // O SOCKET DE COMMANDO (issue #69). Ergue-se depois de o tocador estar de pé,
  // e vive n'esta pilha: declarado ANTES dos fios, o destructor d'elle corre
  // DEPOIS de todos se juntarem, e é elle quem fecha os clientes e desliga o
  // arquivo, por qualquer caminho de sahida. Recusado, diz-se por que e o tocador
  // sobe do mesmo modo, que é o padrão do MPRIS acima e do analisador da issue
  // #5: porta que não abriu não cala musica que já toca.
  // O bloco desceu para depois da livraria e do estaleiro; veja abaixo.

  for (const std::string& faixa : faixas) tocador.junta(faixa);
  if (!faixas.empty()) tocador.tocar_corrente();

  // O ÍNDICE e a VARREDURA. A varredura corre em fio proprio e o navegador
  // recarrega quando ella concluir: assim a tela abre de pronto, com o acervo da
  // corrida anterior, em vez de esperar pelo disco.
  const std::filesystem::path banco = caminho_do_indice();
  nucleo::Biblioteca livraria(banco);
  nucleo::Roleiro roleiro(caminho_das_listas(banco));
  // O PROJECTOR do video. Vive nesta pilha, e o destructor d'elle FECHA a janella:
  // é isso que faz `pgrep` sahir vazio depois de a TUI fechar.
  nucleo::Projector projector(raiz_do_soquete());
  tui::Navegador navegador(livraria, &roleiro);
  std::atomic<bool> varrida{false};
  // O PEDIDO de varredura e o AVISO de que o acervo mudou. Bandeiras, e não fio novo
  // por cada pedido: fio erguido de dentro do tratador de teclas e de dentro do fio da
  // baixa mexeria no mesmo vector de fios de dous lados, e isso é corrida.
  std::atomic<bool> pede_varrer{true};
  std::atomic<bool> acervo_novo{false};

  // O ESTALEIRO das baixas. A obra que elle cumpre é a aquisição da issue #11, e é a
  // MESMA para a URL colada á mão e para o achado eleito na rede: o caminho
  // reaproveita-se inteiro, em vez de se duplicar.
  nucleo::Estaleiro estaleiro(
      ajustes.baixas_simultaneas.valor,
      [acervo](const nucleo::Pedido& pedido, std::filesystem::path* ficou) {
        return nucleo::baixa(acervo, pedido, ficou);
      });

  // O SOCKET DE COMMANDO (issue #69), e elle assenta AQUI, e não acima, por duas
  // razões que se somam. A primeira: os Arredores que a issue #65 lhe deu
  // apontam a livraria e o estaleiro, e acima d'esta linha elles ainda não
  // existem. A segunda, que é a que morde: quem empresta ha de morrer DEPOIS de
  // quem toma emprestado, e em C++ destroe-se ao contrario de como se declara,
  // donde o servidor declarado abaixo d'elles é o primeiro dos tres a cahir.
  //
  // Continua declarado ANTES dos fios, que é o que faz o destructor d'elle
  // correr DEPOIS de todos se juntarem: é elle quem fecha os clientes e desliga
  // o arquivo, por qualquer caminho de sahida. Recusado, diz-se por que e o
  // tocador sobe do mesmo modo, que é o padrão do MPRIS e do analisador: porta
  // que não abriu não cala musica que já toca.
  std::string razao_do_socket;
  const api::Arredores arredores{&livraria, &estaleiro};
  std::optional<api::Servidor> servidor = api::Servidor::abrir(
      tocador, api::caminho_padrao_do_socket(), &razao_do_socket, arredores);
  if (!servidor)
    std::cerr << "mysong: sem socket de commando: " << razao_do_socket << "\n";


  // O CORREIO da busca na rede, e o pedido que o fio d'ella espera. Carrega os
  // ACHADOS do nucleo (issue #56): quem constroe linhas é o navegador, que é
  // quem sabe guardar o achado inteiro para a encommenda.
  tui::CorreioDe<nucleo::Achado> correio;
  std::mutex tranca_do_termo;
  std::string termo_da_rede;
  // A FONTE vigente da busca (issue #56): pegajosa na sessão, YouTube de saida.
  // Vive sob a MESMA tranca do termo, e o fio da busca copia os dous n'um golpe:
  // assim não ha quadro em que o termo seja de uma fonte e a busca de outra.
  nucleo::Fonte fonte_da_busca = ajustes.fonte_da_busca.valor;
  std::atomic<bool> pede_buscar{false};

  // O CORREIO do catalogo do Spotify, e o pedido d'elle. Carrega UM catalogo n'um
  // vector de um: o gabarito do correio carrega vector, e um catalogo é uma cousa.
  tui::CorreioDe<nucleo::Catalogo> correio_do_catalogo;
  std::string url_da_lista;
  std::atomic<bool> pede_catalogo{false};
  tui::CorreioDe<nucleo::Pedido> correio_da_playlist;
  std::string url_da_playlist;
  nucleo::Fonte fonte_da_playlist = nucleo::Fonte::YouTube;
  std::atomic<bool> pede_playlist{false};
  std::atomic<bool> baixa_playlist_ao_chegar{false};

  auto tela = ftxui::ScreenInteractive::Fullscreen();
  // O RATO PEDE-SE Á MÃO (issue #95), e o rastreio do FTXUI fica desligado. Não é
  // desconfiança: elle liga QUATRO modos de uma vez, e um d'elles é o `ESC[?1003h`,
  // que manda uma sequencia de escape a cada MEXIDA do rato, ainda que ninguem
  // carregue em botão algum. Dentro de tmux essas sequencias vazam, e o que o
  // operador vê é o teclado a cuspir lixo e a comer teclas. Medido no FTXUI v7.0.3,
  // em `app.cpp`: com o rastreio ligado sahem o 1000, o 1003, o 1015 e o 1006.
  //
  // D'esses quatro esta Casa consome DOUS: o 1000, que manda o botão a descer e a
  // subir, e o 1006, que os manda no formato SGR, de coordenada sem o tecto de
  // duzentas e vinte e tres collunhas do formato velho. Ligam-se abaixo, ao lado do
  // modo do foco, e desfazem-se logo depois do laço.
  tela.TrackMouse(false);
  std::atomic<bool> sahir{false};
  // O MODO de digitar. Um enum, e não booleanos ao lado: dous booleanos
  // admittem o estado «ambos», que não existe. Mudou-se de casa na issue #79 e
  // vive agora em `tui::Modo`, que o TOPO da tela depende d'elle e o topo tem
  // de se provar, ao passo que a janella não se prova. O apelido fica para o
  // despacho de teclas continuar a dizer `Digita::Busca` sem mudar uma linha.
  using Digita = tui::Modo;
  Digita digita = Digita::Nada;
  // O titulo que a pergunta do apagar mostra. Guardado quando se pergunta: com
  // a pergunta de pé o relogio repinta vinte vezes por segundo, e perguntá-lo
  // ao índice no pintor seriam vinte consultas por segundo.
  std::string titulo_em_causa;
  std::string termo_em_curso;
  // O aviso da rede vive SÓMENTE no fio da tela: quem o escreve é a colheita do
  // correio, que corre no pintor, e quem o lê é o pintor. Fio de fundo algum lhe
  // toca, e por isso elle não pede tranca.
  std::string aviso_da_rede;
  std::size_t primeira_linha = 0;
  // AS CAIXAS da tela (issue #95). Vivem n'esta pilha, e não dentro do pintor: o
  // `reflect` guarda referencia para ellas, e o tratador de eventos lê-as DEPOIS
  // do quadro. Nascem vazias, donde clique algum acha alvo antes da primeira
  // pintura.
  tui::CaixasDaTela caixas;
  // A PEÇA COM FOCO (issue #107). Nasce na PAUTA, que é onde o operador está
  // quando abre o programa: foco de nascença n'uma aba faria a primeira seta
  // andar no cabeçalho em vez de andar na lista, que é o que elle veio fazer.
  tui::Focavel foco = tui::Focavel::Pauta;
  // O MENU DE CONTEXTO (issue #96). Vive n'esta pilha, ao lado das caixas: o
  // tratador muta-o e o pintor lê-o, e os dous correm no fio da tela.
  tui::MenuDeContexto menu;
  // O HELP (issue #133): o estado da janella da ajuda e a caixa d'ella, que o
  // clique de fóra consulta para a fechar. Mora aqui pela razão do menu.
  tui::Ajuda ajuda;
  // O ARRASTO (issue #153): a faixa que está na mão do rato. Mora aqui, ao
  // lado do foco, que é estado da SESSÃO e não do quadro.
  tui::Arrasto arrasto;
  // A CHAPA DO VERSO que está na tela (issue #163), e se ha alguma: é por ellas
  // que se sabe quando a janella da lousa precisa de se limpar.
  tui::AssignaturaDaChapa assignatura_posta;
  bool chapa_posta = false;
  ftxui::Box caixa_da_ajuda = tui::caixa_por_pintar();
  // A LETRA carrega-se do disco UMA vez por faixa, e não a cada quadro: ler
  // arquivo vinte vezes por segundo seria gastar disco para nada. A faixa de que
  // ella é guarda-se ao lado, e é a mudança d'essa que dispara a releitura.
  std::vector<nucleo::LinhaDaLetra> letra;
  std::string letra_de_qual;
  // A FICHA da faixa que sôa, guardada como a letra e pela mesma razão: o
  // indice consultado a cada quadro seriam vinte perguntas por segundo ao
  // banco por uma cousa que sómente muda quando a faixa muda.
  tui::Ficha ficha;
  // A ONDA da faixa (issue #131), para o meio da fita: vazia emquanto não
  // chega, e ahi o meio mostra a barra chata do progresso.
  std::vector<float> onda_da_faixa;
  // O correio por onde a onda chega do fio de fundo: o recado é o caminho da
  // faixa, para que onda de faixa que já sahiu não assente na que entrou.
  tui::CorreioDe<nucleo::Onda> correio_da_onda;
  // OS PICOS do espectro (issue #132): um por banda, e são o estado de que a
  // batida forte precisa e que a composição, sendo pura, não guarda. Vivem
  // aqui, ao lado da ficha, e o relogio monotonico diz-lhes quanto passou.
  std::vector<float> picos;
  std::chrono::steady_clock::time_point quadro_anterior =
      std::chrono::steady_clock::now();

  // O RIO Á VISTA por omissão (issue #109). Nasce mostrando, e não escondendo:
  // ella pediu a letra sempre á vista, e o `l` passou de alternar espectro e
  // letra a esconder e mostrar o rio. O espectro nunca some por causa d'elle.
  //
  // ATOMICO, e não bool nú: o fio do relogio lê-o para o pôr na assignatura, e o
  // fio da tela troca-o na tecla `l`.
  std::atomic<bool> mostra_letra{true};
  // Uma conversão por album e por tamanho; o sextante vem dos ajustes (#94).
  nucleo::Galeria galeria(nucleo::sextante_de(ajustes.capa_sextantes.valor));
  // A LOUSA (issue #103) e o arquivario que a serve. Vivem n'esta pilha, ao
  // lado da Galeria: a lousa ergue o filho ao nascer e mata-o ao morrer, e é
  // por viver aqui que a sahida da tela leva a janella d'ella junto.
  nucleo::Lousa lousa(ajustes.lousa.valor);
  nucleo::Arquivario arquivario;
  // O LETREIRO (issue #108) vive ao lado d'ella, e pela mesma chave: a chapa
  // que elle rasteriza é a lousa quem a põe, e desligada ella não ha onde.
  nucleo::Letreiro letreiro(ajustes.lousa.valor);

  // Os fios de fundo são POSSUIDOS, e juntam-se antes de esta pilha se desfazer. Antes
  // corriam soltos por `detach()`, e o corpo d'elles referencia objectos d'esta pilha:
  // sahindo o programma primeiro, liam memoria morta. Fio solto que aponta para pilha
  // alheia não se justifica, e agora não ha nenhum.
  // A ultima assignatura do que se vê. Vazia de saida, para que o primeiro quadro sahia.
  std::string ultima_assignatura;
  // A VIGILIA do desenho (issue #82): o fio da tela a escreve (foco e tecla)
  // e o fio do relogio a lê. Vive ao lado da assignatura que ella governa.
  tui::Vigilia vigilia(true);
  std::vector<std::thread> ao_fundo;
  // A VARREDURA, em fio permanente que espera por pedido. A conducção por passos da
  // issue #34 existe justamente para isto: o fio pode parar entre dous passos, e a
  // bandeira `sahir` é onde elle olha.
  ao_fundo.emplace_back([&] {
    while (!sahir.load()) {
      if (pede_varrer.exchange(false)) {
        varrida.store(false);
        nucleo::Varredura varredura(banco, {acervo});
        while (!sahir.load() && varredura.passo()) {
        }
        varrida.store(true);
        acervo_novo.store(true);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(MILESIMOS_DO_QUADRO));
    }
  });

  // O FIO DAS PLAYLISTS da tecla `b`. A enumeração acontece fora da tela, e a
  // fila só recebe pedidos depois que a lista inteira foi lida.
  ao_fundo.emplace_back([&] {
    while (!sahir.load()) {
      if (pede_playlist.exchange(false)) {
        std::string url;
        nucleo::Fonte fonte = nucleo::Fonte::YouTube;
        {
          std::lock_guard<std::mutex> chave(tranca_do_termo);
          url = url_da_playlist;
          fonte = fonte_da_playlist;
        }
        std::vector<nucleo::Pedido> pedidos;
        std::string recado;
        if (fonte == nucleo::Fonte::Spotify) {
          recado = "playlist Spotify usa o caminho do catalogo";
        } else {
          std::vector<std::string> urls;
          const bool falou = nucleo::busca_playlist_na_rede(url, &urls);
          for (const std::string& faixa : urls) {
            nucleo::Pedido pedido;
            pedido.url = faixa;
            pedido.fonte = fonte;
            pedidos.push_back(std::move(pedido));
          }
          recado = !falou ? "a playlist não respondeu"
                          : (pedidos.empty() ? "a playlist veio vazia"
                                             : std::to_string(pedidos.size()) +
                                                   " faixas encontradas");
        }
        correio_da_playlist.poe(std::move(pedidos), std::move(recado));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(MILESIMOS_DO_QUADRO));
    }
  });


  // A BUSCA NA REDE, em fio permanente do mesmo modo. Elle NÃO toca a tela nem o
  // navegador: deixa o que achou no correio, e o fio da tela colhe-o.
  ao_fundo.emplace_back([&] {
    while (!sahir.load()) {
      if (pede_buscar.exchange(false)) {
        std::string termo;
        nucleo::Fonte fonte = nucleo::Fonte::YouTube;
        {
          std::lock_guard<std::mutex> chave(tranca_do_termo);
          termo = termo_da_rede;
          fonte = fonte_da_busca;
        }
        // A fonte Spotify NUNCA corre aqui: a tela responde do catalogo, no
        // proprio quadro e sem correio. Um pedido que envelheceu na flag (dous
        // f seguidos com busca em voo) viraria um ytsearch de REDE a pousar
        // por cima do catalogo, sob um cabeçalho que diz Spotify.
        if (fonte == nucleo::Fonte::Spotify) continue;
        std::vector<nucleo::Achado> achados;
        const bool falou =
            nucleo::busca_na_rede(termo, fonte, ACHADOS_POR_BUSCA, &achados);
        // Tres desfechos, e tres recados: a rede muda, a rede que nada achou, e os
        // achados. «Nada se achou» e «não respondeu» são cousas differentes, e dizer
        // a mesma palavra ás duas faria o operador buscar outra vez em vão.
        std::string recado =
            !falou ? "a busca não respondeu: ha yt-dlp e ha rede?"
                   : (achados.empty() ? "nada se achou" : "achados na rede");
        // A resposta só se entrega se o pedido ainda for o VIGENTE: o operador
        // pode ter trocado de fonte ou de termo com esta busca em voo, e a
        // lista velha pousando por cima da nova ficaria a mentir sob um
        // cabeçalho que já diz outra fonte.
        bool vigente = false;
        {
          std::lock_guard<std::mutex> chave(tranca_do_termo);
          vigente = termo == termo_da_rede && fonte == fonte_da_busca;
        }
        if (vigente) correio.poe(std::move(achados), std::move(recado));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(MILESIMOS_DO_QUADRO));
    }
  });

  // O FIO DO CATALOGO, permanente como os outros, e por a mesma razão: fio erguido
  // por cada pedido mexeria no vector de fios de dous lados.
  ao_fundo.emplace_back([&] {
    while (!sahir.load()) {
      if (pede_catalogo.exchange(false)) {
        std::string qual;
        {
          std::lock_guard<std::mutex> chave(tranca_do_termo);
          qual = url_da_lista;
        }
        nucleo::Catalogo lido;
        const bool falou = nucleo::busca_catalogo(qual, &lido);
        // Tres desfechos, e tres recados. «Não é playlist do Spotify» e «a rede não
        // respondeu» são cousas differentes, e dizer a mesma palavra ás duas faria o
        // operador collar a mesma URL outra vez em vão.
        std::string recado;
        if (!falou)
          recado = "não é playlist do Spotify, ou a rede não respondeu";
        else if (lido.faixas.empty())
          recado = "a lista veio vazia";
        else
          recado = std::to_string(lido.faixas.size()) +
                   " faixas: enter baixa a eleita, T baixa todas";
        correio_do_catalogo.poe({std::move(lido)}, std::move(recado));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(MILESIMOS_DO_QUADRO));
    }
  });

  // A BUSCA DA FONTE SPOTIFY, e é SYNCHRONA de proposito: o catalogo é local
  // (issue #13), rede alguma se toca, e responder no proprio quadro poupa o
  // correio. Sem catalogo importado a vista fica como está: trocá-la por vazia
  // apagaria achados uteis por um erro do fluxo, e não do termo. Corre no fio da
  // tela, que é o unico que toca o navegador.
  const auto busca_no_catalogo = [&](const std::string& termo) {
    if (navegador.faixas_do_catalogo().empty()) {
      aviso_da_rede = "importa uma lista do Spotify primeiro (I)";
      return;
    }
    std::vector<nucleo::Achado> achados = nucleo::achados_do_catalogo(
        {navegador.nome_do_catalogo(), navegador.faixas_do_catalogo()}, termo);
    const std::string recado =
        achados.empty() ? "nada se achou no catalogo"
                        : std::to_string(achados.size()) + " achados no catalogo";
    navegador.mostra_rede(std::move(achados));
    aviso_da_rede = recado;
  };

  auto pintor = ftxui::Renderer([&] {
    // Os achados da rede chegam AQUI, no fio da tela, que é o unico que pode tocar o
    // navegador. O fio da busca não o toca: elle põe no correio, e o correio consome-se
    // na colheita, donde a lista se assenta UMA vez e o eleito não volta ao alto a
    // cada quadro.
    //
    // E SÓMENTE COM O CAMPO FECHADO. As tres colheitas d'este pintor que mutam o
    // navegador guardam-se pelo `assenta_novidade`: com o prompt de pé, a secção
    // congela. A tela a mudar debaixo do operador sem elle mandar É o defeito da
    // issue #79, e não importa se a mudança vem de tecla ou de fio de fundo.
    // Nada se perde: o correio guarda o recado até ser colhido, e a bandeira do
    // acervo novo só se consome na leitura. A novidade espera, e assenta no
    // primeiro quadro depois de o campo fechar.
    //
    // O MENU ABERTO congela pela mesma razão, e por uma peor (issue #96): com
    // elle de pé o alvo é a faixa ELEITA, e varredura que assentasse aqui
    // refazia a vista debaixo d'elle. O APAGAR seguinte mandava á lixeira o
    // arquivo que ficou n'aquella linha, e não o que a orla do menu nomeia.
    const bool assenta = tui::assenta_novidade(digita) && !menu.aberto;
    std::vector<nucleo::Achado> achados;
    std::string recado;
    if (assenta && correio.colhe(&achados, &recado)) {
      // A guarda da COLHEITA, par da do fio: entre a checagem de lá e o pouso
      // aqui cabe um f, e a resposta que já não é da fonte vigente cai. Os
      // achados vêm estampados; a resposta VAZIA é sempre de fonte de rede,
      // donde sob a fonte Spotify ella é velha por construcção (o catalogo
      // responde no proprio quadro, sem passar por este correio).
      const bool casa = achados.empty()
                            ? fonte_da_busca != nucleo::Fonte::Spotify
                            : achados.front().fonte == fonte_da_busca;
      if (casa) {
        navegador.mostra_rede(std::move(achados));
        aviso_da_rede = recado;
      }
    }
    // O CATALOGO chega pelo mesmo caminho, e no mesmo fio: mostra-se ANTES de se
    // baixar cousa alguma, que é o que a tarefa pede quando manda devolver a lista
    // para se conferir.
    std::vector<nucleo::Catalogo> lidos;
    std::string recado_da_lista;
    if (assenta && correio_do_catalogo.colhe(&lidos, &recado_da_lista)) {
      if (!lidos.empty() && !lidos.front().faixas.empty()) {
        navegador.mostra_catalogo(std::move(lidos.front()));
        if (baixa_playlist_ao_chegar.exchange(false)) {
          const std::string lista = navegador.nome_do_catalogo();
          std::size_t quantas = 0;
          for (const nucleo::FaixaDoCatalogo& faixa :
               navegador.faixas_do_catalogo()) {
            estaleiro.encommenda(encommenda_do_catalogo(faixa, lista));
            ++quantas;
          }
          recado_da_lista = std::to_string(quantas) + " faixas encomendadas";
        }
      } else {
        baixa_playlist_ao_chegar.store(false);
      }
      aviso_da_rede = recado_da_lista;
    }
    std::vector<nucleo::Pedido> pedidos_da_playlist;
    std::string recado_da_playlist;
    if (assenta && correio_da_playlist.colhe(&pedidos_da_playlist,
                                             &recado_da_playlist)) {
      if (baixa_playlist_ao_chegar.exchange(false)) {
        for (nucleo::Pedido& pedido : pedidos_da_playlist)
          estaleiro.encommenda(std::move(pedido));
        if (!pedidos_da_playlist.empty())
          recado_da_playlist = std::to_string(pedidos_da_playlist.size()) +
                              " faixas encomendadas";
      }
      aviso_da_rede = recado_da_playlist;
    }
    // A varredura concluiu: o navegador recarrega UMA vez. A bandeira do acervo novo
    // CONSOME-SE na leitura, donde isto corre uma vez por varredura.
    //
    // Isto corria no fio do RELOGIO, e mudou-se para cá. O navegador é mutado pelo
    // tratador de teclas, que corre no fio da tela; recarregá-lo do relogio era
    // mutá-lo de um fio e lê-lo de outro. O pintor corre no mesmo fio do tratador,
    // donde a corrida sahe. Não é embelleçamento: é o defeito da corrida a fechar-se.
    if (assenta && acervo_novo.exchange(false)) {
      livraria.reabre();
      navegador.recarrega();
    }
    const tui::Retracto retracto = retracto_do(tocador, projector);
    // A tela INTEIRA, sem desconto algum. A conta velha tirava-lhe quatro
    // collunhas de orla e cinco linhas de guarnição (a marca, o topo, o
    // transporte, o rodapé e as duas da orla); a sala da issue #102 não tem
    // orla, e as linhas que ella gasta reparte-as ella propria.
    const ftxui::Dimensions tela = ftxui::Terminal::Size();
    const tui::Sala sala = tui::sala_da_tela(
        tela.dimx > 0 ? static_cast<std::size_t>(tela.dimx) : 0,
        tela.dimy > 0 ? static_cast<std::size_t>(tela.dimy) : 0,
        digita != Digita::Nada);
    primeira_linha =
        tui::primeira_a_mostrar(navegador.eleito(), navegador.vista().size(),
                                sala.pauta.altura, primeira_linha);
    caixas.primeira_linha = primeira_linha;  // a rolagem d'este quadro

    // A letra e a ficha relêem-se sómente quando a faixa muda.
    if (retracto.titulo != letra_de_qual) {
      letra_de_qual = retracto.titulo;
      letra = retracto.titulo.empty()
                  ? std::vector<nucleo::LinhaDaLetra>()
                  : nucleo::le_lrc_do_disco(retracto.titulo);
      nucleo::Faixa d_ella;
      livraria.acha_por_caminho(retracto.titulo, d_ella);
      ficha = tui::ficha_da_faixa(retracto.titulo, d_ella.titulo, d_ella.artista,
                                  d_ella.album);
      // Os PICOS morrem com a faixa (issue #132): o pico da que sahiu accendia
      // a primeira batida da que entra, e na côr da familia errada.
      picos.clear();
      // A CHAPA DO PRIMEIRO VERSO (issue #161) rasteriza-se assim que a faixa
      // muda, e não quando elle chega: o pango-view corre duas vezes na
      // primeira chamada, e esperá-lo com a musica já a andar é o que fazia a
      // primeira fala chegar tarde. As seguintes já se adiantavam.
      if (!letra.empty() && !sala.letra.vazio())
        letreiro.chapa(tui::pedido_da_chapa_da_letra(
            tui::verso_do_bloco(letra, -1, sala.letra.largura),
            static_cast<std::size_t>(ftxui::string_width(
                tui::verso_do_bloco(letra, -1, sala.letra.largura)))));
      // A ONDA da faixa (issue #131) colhe-se n'um fio de fundo: o ffmpeg leva
      // um segundo na primeira vez, e o quadro não espera por elle. Até chegar,
      // o meio da fita mostra a barra chata.
      onda_da_faixa.clear();
      if (!retracto.titulo.empty()) {
        const std::string caminho = retracto.titulo;
        ao_fundo.emplace_back([caminho, &correio_da_onda] {
          std::vector<nucleo::Onda> colhida;
          colhida.push_back(nucleo::colhe_onda(caminho));
          correio_da_onda.poe(std::move(colhida), caminho);
        });
      }
    }
    // A onda que chegou (issue #131) assenta só se for da faixa que AINDA toca:
    // a que sahiu entretanto morre no correio.
    {
      std::vector<nucleo::Onda> chegadas;
      std::string de_qual;
      if (correio_da_onda.colhe(&chegadas, &de_qual) &&
          de_qual == retracto.titulo && !chegadas.empty())
        onda_da_faixa = std::move(chegadas.front().pontos);
    }
    // O CONTEXTO que o rotulo pede: a fonte na busca da rede, o nome da lista na
    // pergunta do apagar. Os demais modos ignoram-no.
    const std::string contexto_do_campo =
        digita == Digita::ConfirmaFaixa ? titulo_em_causa
        : digita == Digita::Confirma
            ? navegador.nome_do_rol_eleito()
            : std::string(nucleo::nome_da_fonte(fonte_da_busca));
    // A CHAPA por cima da pauta. A somma é das linhas Á VISTA, e não do
    // acervo: com filtro posto, o operador ha de ler a conta do que VÊ.
    tui::Chapa chapa;
    chapa.onde = tui::onde_da_chapa(navegador.secao(), navegador.trilha(),
                                    navegador.nome_do_catalogo());
    chapa.quantas = navegador.vista().size();
    chapa.especie = tui::especie_da_secao(navegador.secao());
    for (const tui::Linha& qual : navegador.vista())
      chapa.duracao += qual.duracao;
    chapa.vista = tui::nome_da_vista(navegador.secao());
    // A pauta vazia não pinta recado algum (issue #111): o conselho sobe á
    // chapa, que é a linha em que se lê o estado do logar em que se está.
    if (navegador.vista().empty())
      chapa.conselho = tui::conselho_do_vazio(navegador.secao(),
                                              !navegador.termo().empty());
    // As ENCOMMENDAS ganham logar proprio na DOWNLOAD, que é onde a issue as
    // pede por cima da lista. Nas outras abas ellas descem ao recado, que alli
    // a linha não é d'ellas e o que importa é a secção em que se está.
    const std::string andamento =
        nucleo::texto_do_andamento(estaleiro.andamento());
    const bool na_baixa =
        tui::aba_da_secao(navegador.secao()) == tui::Aba::Download;
    if (na_baixa) chapa.encommendas = andamento;
    // O RECADO da chapa: o que a trilha carregava á direita. Junta-se por
    // ordem de urgencia, e cada pedaço sahe INTEIRO ou não sahe: o que não
    // couber no que a chapa deixa fica de fóra, em vez de se cortar a meio da
    // palavra. Mediu-se nos dumps, antes d'esta conta: «a rede está vazia:
    // busca prime», e n'outro o «(s)» perdido, que é a tecla a apertar.
    const std::size_t sobra = tui::espaco_do_recado(chapa, sala.chapa.largura);
    std::string dito;
    const auto junta = [&dito, sobra](const std::string& pedaco) {
      if (pedaco.empty()) return;
      const std::size_t pede =
          static_cast<std::size_t>(ftxui::string_width(dito)) +
          (dito.empty() ? 0u : 2u) +
          static_cast<std::size_t>(ftxui::string_width(pedaco));
      if (pede > sobra) return;
      if (!dito.empty()) dito += "  ";
      dito += pedaco;
    };
    // A ordem é a da URGENCIA, e é ella que decide quem fica de fóra. O que
    // está a CORRER vem antes do que já aconteceu: a varredura e as baixas
    // antes do aviso, que o aviso é desfecho e elles são obra em curso.
    if (!varrida.load()) junta("a varrer o acervo...");
    if (!na_baixa) junta(andamento);
    // A FONTE diz-se na secção da rede, e sempre: a lista pode ser da fonte
    // anterior por um instante, que a busca é assynchrona.
    if (navegador.secao() == tui::Secao::Rede)
      junta(std::string(nucleo::nome_da_fonte(fonte_da_busca)));
    junta(aviso_da_rede);
    // A lista ALVO diz-se havendo alguma, e em toda secção: é para onde o `a`
    // manda a faixa, e o operador não ha de o adivinhar.
    if (navegador.rol_corrente() != 0 &&
        navegador.secao() != tui::Secao::NoRol)
      junta("\ue0b1 " + navegador.nome_corrente());
    if (!navegador.termo().empty()) junta("[" + navegador.termo() + "]");
    // A janella do video cala-se por si quando ella morre: é a pergunta ao
    // processo que o diz, e não bandeira nossa que pudesse ficar a mentir.
    if (projector.rodando())
      junta("video: " + projector.faixa().filename().string());
    chapa.recado = dito;
    // A LOUSA de pé toma a capa (issue #103), e ahi o chafa NÃO corre: o render
    // d'elle ficaria por baixo da janella e ninguem o veria, e cada troca de
    // faixa pagaria dezenas de milesimos por um desenho invisivel.
    const bool pela_lousa = lousa.disponivel() && !sala.capa.vazio();
    static const nucleo::CapaPintada kSemArte;
    const nucleo::CapaPintada& arte =
        pela_lousa ? kSemArte
                   : galeria.capa(retracto.titulo, sala.capa.largura,
                                  sala.capa.altura);
    // O rectangulo da lousa nasce DENTRO do da sala, e guarda a proporção da
    // imagem: é o mesmo tecto de quarenta e cinco por cento que o chafa recebe.
    nucleo::Retangulo rectangulo;
    if (pela_lousa)
      rectangulo = nucleo::rectangulo_da_capa(
          arquivario.de(retracto.titulo).medida, sala.capa.largura,
          sala.capa.altura, nucleo::CELLULA_DA_CASA);
    // A ARTE mede-se pelo que se vae pintar, e não pelo tecto: a capa de 16 por
    // 9 sahe mais baixa, e o que ella deixa fica para o espectro.
    const std::size_t alt_arte =
        pela_lousa ? rectangulo.linhas
                   : tui::linhas_da_arte(arte, sala.capa.altura);
    // A ORLA do foco come duas linhas (issue #107), e a conta do espectro
    // desconta-as: sem o desconto, o pé do painel sahia aparado em silencio
    // emquanto a capa tivesse o foco.
    const bool capa_com_foco = foco == tui::Focavel::Capa;
    const tui::Rectangulo abaixo =
        tui::espectro_abaixo_da(sala, alt_arte + (capa_com_foco ? 2 : 0));
    // Os centros em hertz (issue #104), colhidos UMA vez: a escala é do
    // contracto do analisador, e o punho d'elle não abre as bordas que o
    // nucleo assentou.
    static const std::vector<float> centros_em_hertz =
        tui::centros_da_escala(nucleo::QUANTAS_BANDAS);
    // O TEMPO REAL do quadro (issue #132), e não os cincoenta milesimos do
    // compasso: quadro que se atrasa derrubaria o pico de menos, e a meia-vida
    // é conta de segundos. Calado, os picos zerão-se, que batida não ha no que
    // se não ouve.
    const std::chrono::steady_clock::time_point instante =
        std::chrono::steady_clock::now();
    const double lapso =
        std::chrono::duration<double>(instante - quadro_anterior).count();
    quadro_anterior = instante;
    const std::vector<float> bandas = tocador.bandas();
    if (retracto.mudo) picos.clear();
    tui::avanca_picos(picos, bandas, lapso);
    const tui::Quadro quadro = tui::compor(bandas, abaixo.largura,
                                           abaixo.altura, false,
                                           centros_em_hertz, picos);
    // Tela estreita não pinta painel algum, e com elle vão-se a capa, o
    // espectro e a letra: roubar da pauta, que é onde se navega, para mostrar
    // arte seria trocar o que serve pelo que enfeita.
    //
    // A CAIXA da arte (issue #95) pendura-se aqui, no punho que a sala recebe:
    // assim a lavra irmã da lousa troca o INTERIOR do rectangulo sem tocar na
    // caixa. Esvazia-se a cada quadro, que painel que se não pinta não ha de
    // deixar caixa velha a apanhar cliques.
    // A ORDEM á lousa vae com a caixa do quadro ANTERIOR, que é a unica que o
    // `reflect` já encheu: elle escreve DEPOIS de o pintor devolver o quadro.
    // Custa UM quadro de atraso ao redimensionar, e não custa mais nada, que a
    // mesma ordem repetida não manda cousa alguma pelo cano.
    const std::filesystem::path capa_do_painel =
        pela_lousa ? arquivario.de(retracto.titulo).caminho
                   : std::filesystem::path();
    // O FOCO manda aqui, e em todo quadro: o `tira_tudo` do tratador desfaz-se
    // no desenho que o FTXUI faz logo a seguir ao evento, e a capa voltava.
    if (nucleo::ordem_da_capa(pela_lousa, vigilia.pede_batida(),
                              !capa_do_painel.empty(),
                              caixas.capa.x_max >= caixas.capa.x_min) ==
        nucleo::OrdemDaCapa::Tira)
      lousa.tira("capa");
    else
      // O canto é do quadro ANTERIOR e o rectangulo é d'este: encolhendo-se o
      // terminal, o canto velho cae fóra da tela nova e a imagem sahia meia
      // por fóra por um quadro. Cinge-se á borda, que atraso de um quadro se
      // corrige na batida seguinte e imagem fóra da tela não se corrige.
      lousa.poe("capa", capa_do_painel,
                std::min(caixas.capa.x_min,
                         std::max(0, static_cast<int>(sala.cabecalho.largura) -
                                         static_cast<int>(
                                             rectangulo.collunas))),
                caixas.capa.y_min, rectangulo.collunas, rectangulo.linhas);
    caixas.capa = tui::caixa_por_pintar();
    // O RIO (issue #109). A letra não toma mais o logar do espectro: nasce na
    // base d'elle e sobe por cima. Escondido o rio pelo `l`, o quadro d'elle sae
    // VAZIO, e a composição devolve o espectro tal qual; faixa sem `.lrc` faz o
    // mesmo por si, que letra alguma se inventa.
    //
    // Resolve-se ANTES das chapas (issue #110) por ser d'elle que sae a caixa da
    // chapa da linha corrente, que vae pela mesma lousa d'ellas.
    // O VERSO CORRENTE (issue #157): elle é funcção da posição, e de mais nada.
    // O rio que subia morreu; o bloco fica QUIETO debaixo da capa.
    const int verso_corrente =
        mostra_letra.load() ? nucleo::linha_corrente(letra, retracto.posicao)
                            : -1;
    // AS CHAPAS DAS ABAS (issue #108), pela MESMA lousa e com a mesma
    // disciplina: a ordem sae do QUADRO, e as caixas são as do quadro
    // anterior, que são as unicas que o `reflect` já encheu.
    const std::size_t escritas = lousa.escritas();
    std::filesystem::path ultima_chapa;
    // A aba com FOCO (issue #107) entra na ordem: assim a chapa em XIROD da
    // aba focada sae do MESMO degrau que pinta a cella debaixo d'ella, e as
    // duas não se desencontram. Vazio quer dizer que o foco está fóra da fita.
    const std::optional<tui::Aba> focada = tui::aba_com_foco(foco);
    for (const tui::ChapaDaAba& ordem : tui::ordens_das_chapas(
             caixas.cabecalho, tui::aba_da_secao(navegador.secao()),
             lousa.disponivel() && letreiro.disponivel(),
             vigilia.pede_batida(), focada ? &*focada : nullptr)) {
      const std::filesystem::path* chapa = nullptr;
      if (ordem.poe) chapa = &letreiro.chapa(tui::pedido_da_chapa(ordem));
      // Chapa que não veio TIRA a que estava, e não a deixa: a aba trocou de
      // degrau, e a imagem velha mentiria sobre onde o operador está.
      if (chapa == nullptr || chapa->empty()) {
        lousa.tira(tui::identidade_da_chapa(ordem.aba));
        continue;
      }
      // A caixa INTEIRA do rotulo (issue #126): a linha de partida é a de
      // CIMA do segmento, e a chapa toma as fileiras que a ordem traz, que na
      // fita do pé são duas. Pedida com altura UM, a imagem parava na fileira
      // de cima e a de baixo ficava com o fundo pelado.
      lousa.poe(tui::identidade_da_chapa(ordem.aba), *chapa, ordem.collunha,
                ordem.linha, ordem.largura, ordem.linhas);
      ultima_chapa = *chapa;
    }
    // A CHAPA DA LINHA CORRENTE (issue #110), ao lado das das abas e pela mesma
    // lousa: o verso que se canta cristaliza em XIROD sobre as cellas d'elle. O
    // mono continua pintado por baixo, e por isso sem lousa nada falta.
    // A caixa do bloco vem do quadro ANTERIOR (issue #161): é a unica que diz
    // onde elle está DE FACTO, que a sala conta a capa pelo tecto e a de 16 por
    // 9 sahe mais baixa. Medida pela sala, a imagem cahia abaixo do bloco, e o
    // verso apparecia duas vezes.
    const tui::ChapaDaLetra da_letra = tui::ordem_da_chapa_parada(
        letra, verso_corrente, caixas.letra,
        lousa.disponivel() && letreiro.disponivel(), vigilia.pede_batida(),
        mostra_letra.load());
    // A CHAPA DE PÉ (issue #165) é a MESMA condição que manda pô-la, e não uma
    // segunda conta: assim a cella nunca fica em branco sem que a imagem venha.
    bool chapa_de_pe = false;
    const std::filesystem::path* cristal = nullptr;
    if (da_letra.poe)
      cristal = &letreiro.chapa(
          tui::pedido_da_chapa_da_letra(da_letra.verso, da_letra.cellulas));
    if (cristal == nullptr || cristal->empty()) {
      lousa.tira(tui::IDENTIDADE_DA_LETRA);
      chapa_posta = false;
    } else {
      // LIMPAR ANTES DE PÔR (issue #163): a janella do Überzug++ conserva o que
      // a imagem anterior pintou FÓRA da nova, e o verso que sae é quasi sempre
      // mais largo que o que entra: ficavam as duas pontas d'elle na tela, uma
      // de cada lado. Tira-se sómente quando a chapa é OUTRA, que tirar a mesma
      // a cada quadro faria a lettra piscar.
      const tui::AssignaturaDaChapa agora = tui::assignatura_da(da_letra);
      if (tui::limpa_antes_de_por(assignatura_posta, agora, chapa_posta))
        lousa.tira(tui::IDENTIDADE_DA_LETRA);
      assignatura_posta = agora;
      chapa_posta = true;
      chapa_de_pe = true;
      // TRES fileiras (issue #157), que é o corpo GRANDE que elle pediu: a
      // chapa cobre as cellas que a sala reservou ao verso.
      lousa.poe(tui::IDENTIDADE_DA_LETRA, *cristal, da_letra.collunha,
                da_letra.linha, da_letra.cellulas, tui::FILEIRAS_DO_VERSO);
      // E a chapa do VERSO não fica de empurrão, o que é MEDIÇÃO d'esta prova,
      // e não escrupulo: o empurrão encolhe a imagem a UMA cella, e a do verso
      // é dez vezes mais larga que alta; a altura arredonda a ZERO, e o
      // Überzug++ aborta na redimensão (a asserção `inv_scale_x > 0` do OpenCV)
      // levando comsigo a capa e as tres abas. A das abas é quasi quadrada, e
      // é ella que o `ultima_chapa` guarda.
    }
    // A PROXIMA rasteriza-se ao NASCER d'ella, e não no instante em que se
    // canta: o pango-view corre duas vezes na primeira chamada, e esperá-lo com
    // a voz já a cantar deixaria em mono o quadro em que a linha cristaliza.
    // O VERSO SEGUINTE rasteriza-se adeantado, para que a troca não apanhe o
    // pango-view a frio: elle corre duas vezes na primeira chamada.
    if (verso_corrente >= 0 &&
        static_cast<std::size_t>(verso_corrente) + 1 < letra.size() &&
        !sala.letra.vazio()) {
      const std::string proximo = tui::verso_do_bloco(
          letra, verso_corrente + 1, sala.letra.largura);
      if (!proximo.empty())
        letreiro.chapa(tui::pedido_da_chapa_da_letra(
            proximo, static_cast<std::size_t>(ftxui::string_width(proximo))));
    }
    // O EMPURRÃO, e sómente havendo ordem nova: a chapa do VERSO tem uma
    // linha, e a janella de uma linha do Überzug++ fica preta até que outra
    // ordem chegue. As das abas passaram a duas (issue #126) e desenham-se
    // sósinhas; é a d'ellas que empurra, que a do verso aborta na reducção.
    if (!ultima_chapa.empty() && lousa.escritas() != escritas)
      lousa.empurra(ultima_chapa);
    // Com a lousa de pé, as célullas debaixo da imagem pintam o FUNDO do
    // painel, e marcador algum: a janella d'ella chega um quadro depois, e
    // n'esse quadro o operador não ha de ver nota musical por baixo da capa.
    // O vão mede o RECTANGULO da imagem, e não a largura do painel: é o
    // `hcenter` do `elemento_do_painel` que o centra, e a caixa que d'elle sahe
    // é a que a lousa lê para saber onde pôr a janella. Medindo o painel
    // inteiro, a caixa daria o canto esquerdo e a imagem sahiria encostada.
    ftxui::Element quadro_da_arte =
        pela_lousa ? ftxui::text(std::string(rectangulo.collunas, ' ')) |
                         ftxui::size(ftxui::HEIGHT, ftxui::EQUAL,
                                     static_cast<int>(alt_arte))
                   : tui::elemento_da_arte(arte, sala.capa.largura, alt_arte);
    // A CAIXA fica por DENTRO da orla do foco (issue #107), e é de proposito:
    // é ella que a lousa lê para saber onde pôr a janella da imagem, e medida
    // por fóra a imagem sahiria por cima do quadro que a assignala.
    ftxui::Element quadro_com_caixa =
        std::move(quadro_da_arte) | ftxui::reflect(caixas.capa);
    if (capa_com_foco)
      quadro_com_caixa = tui::orla_do_foco(std::move(quadro_com_caixa));
    // O RIO já se resolveu lá em cima, antes das chapas (issue #110): é d'elle
    // que sae a caixa da chapa da linha corrente, e a chapa vae pela mesma
    // lousa das abas. Aqui só se compõe o que d'elle sahiu.
    // A FICHA do que sôa (issue #134) na primeira fileira do painel, por cima
    // da arte: o nome deixou a fita, que a ordem d'elle não lhe deixou logar.
    ftxui::Element painel =
        sala.painel.vazio()
            ? ftxui::emptyElement()
            : ftxui::vbox(
                  {tui::elemento_da_ficha(ficha, sala.ficha.largura),
                   tui::elemento_do_painel(
                       std::move(quadro_com_caixa),
                       ftxui::vbox({tui::elemento_da_letra_parada(
                                        mostra_letra.load()
                                            ? letra
                                            : std::vector<nucleo::LinhaDaLetra>(),
                                        verso_corrente, sala.letra.largura,
                                        sala.letra.altura, chapa_de_pe) |
                                        ftxui::reflect(caixas.letra),
                                    tui::elemento_do_espectro(quadro)}),
                       sala.painel.largura)});
    // AS DUAS METADES. O `size` na altura mede EXACTAMENTE o que a sala contou,
    // pela razão que a composição velha ensinou: por menos, o pé da tela fica
    // em branco; por mais, o rodapé sahe d'ella.
    std::vector<ftxui::Element> metades = {ftxui::vbox(
        {tui::elemento_da_chapa(chapa, sala.chapa.largura),
         // A caixa da PAUTA INTEIRA pendura-se aqui (issue #107), e não dentro
         // da tabella: é a caixa que o FOCO lê para saltar ás visinhas.
         //
         // O cinge da ALTURA vem antes d'ella, e é o que a faz existir sempre:
         // a pauta vazia da issue #111 devolve `emptyElement`, que não pede
         // linha alguma, e caixa de altura zero não é candidata a salto. Sem o
         // cinge, o foco não tornava á pauta d'uma lista de listas vazia.
         tui::elemento_da_tabella(navegador, primeira_linha, sala.pauta.altura,
                                  sala.pauta.largura, retracto.titulo,
                                  &caixas.linhas, &arrasto) |
             ftxui::size(ftxui::HEIGHT, ftxui::EQUAL,
                         static_cast<int>(sala.pauta.altura)) |
             ftxui::reflect(caixas.pauta)})};
    if (!sala.painel.vazio()) {
      metades.push_back(tui::elemento_do_divisor(sala.divisor.altura));
      metades.push_back(std::move(painel));
    }
    // A FITA leva a onda da faixa ao meio (issue #134), e com ella a caixa do
    // trilho: o clique que busca é agora um clique na fita.
    ftxui::Element fita = tui::elemento_do_cabecalho(
        retracto, tui::aba_da_secao(navegador.secao()), onda_da_faixa,
        sala.cabecalho.largura, &caixas.cabecalho, foco, sala.cabecalho.altura);
    // A ORDEM da tela nova (issue #125): o corpo abre na PRIMEIRA linha, e o pé
    // toma as ultimas, de cima para baixo o campo, a fita e as dicas. O trilho
    // morreu na issue #134: a onda no meio da fita é o progresso.
    std::vector<ftxui::Element> tudo = {
        ftxui::hbox(std::move(metades)) |
        ftxui::size(ftxui::HEIGHT, ftxui::EQUAL,
                    static_cast<int>(sala.pauta.altura +
                                     (sala.chapa.vazio() ? 0 : 1)))};
    if (!sala.campo.vazio())
      tudo.push_back(tui::elemento_do_campo(digita, contexto_do_campo,
                                            termo_em_curso,
                                            sala.campo.largura));
    tudo.push_back(std::move(fita));
    if (!sala.rodape.vazio())
      tudo.push_back(ftxui::text(kDicas) | ftxui::dim);
    ftxui::Element corpo = ftxui::vbox(std::move(tudo));
    if (menu.aberto) {
      // A LINHA ALVO em coordenadas da tela. Vem da SALA, e não das caixas do
      // rato: ellas enchem-se no `reflect`, que corre DEPOIS d'esta composição, e
      // n'este ponto do quadro estão todas por pintar. A conta é a mesma que a
      // pauta faz, e não outra: a linha visivel é a absoluta menos a rolagem.
      ftxui::Box linha_alvo = tui::caixa_por_pintar();
      if (menu.faixa >= primeira_linha &&
          menu.faixa - primeira_linha < sala.pauta.altura)
        linha_alvo = {
            static_cast<int>(sala.pauta.x),
            static_cast<int>(sala.pauta.x + sala.pauta.largura) - 1,
            static_cast<int>(sala.pauta.y + menu.faixa - primeira_linha),
            static_cast<int>(sala.pauta.y + menu.faixa - primeira_linha)};
      corpo = ftxui::dbox(
          {std::move(corpo),
           tui::flutuante_do_menu(
               menu, linha_alvo, sala.cabecalho.largura,
               tela.dimy > 0 ? static_cast<std::size_t>(tela.dimy) : 0)});
    }
    // O HELP (issue #133) flutua por cima de TUDO, menu incluido: é a peça
    // mais de cima da tela, e é a ultima a compor-se por isso mesmo.
    if (!ajuda.aberta) return corpo;
    return ftxui::dbox(
        {std::move(corpo),
         tui::flutuante_da_ajuda(
             ajuda, sala.cabecalho.largura,
             tela.dimy > 0 ? static_cast<std::size_t>(tela.dimy) : 0,
             &caixa_da_ajuda)});
  });

  // vai_para_aba — o caminho das teclas `1` `2` `3`, n'um logar só. Sahe do
  // ramo d'ellas porque o clique na aba (issue #95) percorre o MESMO caminho:
  // duas copias d'estes recados divergiriam na primeira issue que mexesse
  // n'uma d'ellas, e o dedo veria um aviso e a tecla outro.
  //
  // Aba sem chão AVISA e fica onde está. A DOWNLOAD é a unica que o pode não
  // ter: ella abre a lista dos achados, e antes da primeira busca não ha
  // achado algum. As outras duas abrem sempre.
  const auto vai_para_aba = [&](tui::Aba qual) {
    if (navegador.vai_para(tui::secao_da_aba(qual))) return;
    aviso_da_rede = "a rede está vazia: busca primeiro (s)";
  };

  // abre_o_menu_na — o menu sobre a faixa de indice `qual`, que se ELEGE
  // primeiro. Eleger ao abrir é o que o gerenciador de arquivos d'elle faz com
  // o botão direito, e é o que deixa o menu chamar as ordens que já existem:
  // ellas trabalham todas sobre a ELEITA, e menu que abrisse n'outra faixa
  // pediria um segundo caminho para cada uma d'ellas.
  // elege_a_linha — leva a eleição ao indice pedido pelo sobe e pelo desce, que
  // SATURAM. É o mesmo caminho que o clique já anda, e não punho novo.
  const auto elege_a_linha = [&](std::size_t qual) {
    while (navegador.eleito() != qual) {
      const std::size_t antes = navegador.eleito();
      antes < qual ? navegador.desce() : navegador.sobe();
      if (navegador.eleito() == antes) break;  // saturou: acabou a lista
    }
  };

  // arruma_a_faixa — cumpre o que o arrasto pediu (issue #153). No ACERVO pela
  // ordem propria da bibliotheca (issue #152); dentro de uma LISTA pelos passos
  // que o `K` e o `J` já dão, um de cada vez, que é o punho que o roleiro tem.
  // Fóra d'essas duas vistas nada se move, e o arrasto nem chega aqui.
  const auto arruma_a_faixa = [&](std::size_t de, std::size_t para) {
    if (de == para) return;
    const std::vector<tui::Linha>& vista = navegador.vista();
    if (de >= vista.size() || para >= vista.size()) return;
    if (navegador.secao() == tui::Secao::Busca) {
      const std::string qual = vista[de].chave;
      if (!livraria.move_faixa(qual, para)) {
        aviso_da_rede = "não se pôde arrumar essa faixa";
        return;
      }
      navegador.recarrega();
      elege_a_linha(para);
      return;
    }
    if (navegador.secao() != tui::Secao::NoRol) return;
    elege_a_linha(de);
    const std::size_t passos = de < para ? para - de : de - para;
    for (std::size_t passo = 0; passo < passos; ++passo)
      if (!(de < para ? navegador.desce_no_rol() : navegador.sobe_no_rol())) {
        aviso_da_rede = "não se pôde arrumar essa faixa";
        return;
      }
  };

  const auto abre_o_menu_na = [&](std::size_t qual) {
    while (navegador.eleito() != qual) {
      const std::size_t antes = navegador.eleito();
      antes < qual ? navegador.desce() : navegador.sobe();
      if (navegador.eleito() == antes) break;  // saturou: acabou a lista
    }
    const std::string caminho = navegador.caminho_eleito();
    if (caminho.empty()) {
      aviso_da_rede = "o menu é da faixa: elege uma primeiro";
      return;
    }
    nucleo::Faixa d_ella;
    livraria.acha_por_caminho(caminho, d_ella);
    tui::abre_o_menu(menu, navegador.eleito(),
                     tui::ficha_da_faixa(caminho, d_ella.titulo, d_ella.artista,
                                         d_ella.album)
                         .titulo,
                     navegador.rois());
  };

  // cria_a_lista_com — a lista NOVA já com a faixa eleita dentro, que é o que o
  // item NOVA LISTA COM ESTA promette (issue #96). Vae pelo roleiro, e não pelo
  // `cria_rol` do navegador: esse não devolve o id da que nasceu, e sem o id
  // não ha onde juntar. Passa-se ás listas a seguir, como o `c` já passava:
  // quem cria uma lista quer vê-la, e vê-la é o modo de conferir que nasceu.
  const auto cria_a_lista_com = [&](const std::string& nome) {
    const std::string caminho = navegador.caminho_eleito();
    const int qual = roleiro.cria(nome);
    if (qual == 0) {
      aviso_da_rede = "esse nome já existe, ou é vazio";
      return;
    }
    roleiro.junta(qual, caminho);
    navegador.mostra_rois();
    aviso_da_rede = "«" + nome + "» criada com a faixa";
  };

  // junta_na_lista — a faixa eleita na lista de id `qual`, que é a que o submenu
  // escolheu. Chama o roleiro, e não o `junta_ao_rol` do navegador: esse junta
  // na lista CORRENTE, e a corrente não é a que se escolheu; mudar a corrente
  // por um item de menu faria o `a` seguinte juntar n'outra lista sem que
  // ninguem lh'o tivesse pedido.
  const auto junta_na_lista = [&](int qual) {
    const std::string caminho = navegador.caminho_eleito();
    if (qual == 0 || caminho.empty() || !roleiro.junta(qual, caminho)) {
      aviso_da_rede = "não se pôde juntar á lista";
      return;
    }
    navegador.recarrega();  // dentro d'ella, a faixa apparece no mesmo quadro
    for (const nucleo::Rol& rol : menu.listas)
      if (rol.id == qual) aviso_da_rede = "juntada a «" + rol.nome + "»";
  };

  auto janella = ftxui::CatchEvent(pintor, [&](const ftxui::Event& tecla) {
    // O FOCO DO PAINEL trata-se ANTES até do modo de digitar (issue #82):
    // escape de foco não é tecla, e não ha de virar «não» de confirmação nem
    // letra no termo em curso.
    switch (tui::gesto_do_foco(tecla)) {
      case tui::GestoDoFoco::Ganha: vigilia.ganha(); return true;
      case tui::GestoDoFoco::Perde:
        vigilia.perde();
        // A janella da lousa NÃO segue o foco do terminal: perdido elle, a
        // imagem ficaria por cima do que o operador foi ver. Quem manda de
        // facto é o `ordem_da_capa` do pintor, que lê a mesma vigilia em todo
        // quadro; este é a redundancia barata que tira as demais identidades.
        lousa.tira_tudo();
        return true;
      case tui::GestoDoFoco::Alheio: break;
    }
    // Tecla de gente só chega a painel focado: adormecida, a vigilia acorda
    // aqui e a tecla SEGUE ao seu fluxo de sempre. Desperta ou sem noticia,
    // nada ha que acordar, e noticia de foco tecla nenhuma dá.
    if (!vigilia.pede_batida() && tui::eh_tecla_de_gente(tecla))
      vigilia.ganha();
    // O AVISO vale por UM gesto. Elle diz o DESFECHO do que se acabou de
    // fazer, e a tecla seguinte apaga-o ANTES de correr, para que o ramo que
    // ella tomar escreva o seu. Sem este prazo o aviso ficava pregado na chapa
    // a sessão inteira, e comia o logar que a conta do recado reservaria ao
    // andamento das baixas, que é obra em curso e não desfecho velho.
    if (tui::eh_tecla_de_gente(tecla)) aviso_da_rede.clear();
    // O MENU DE CONTEXTO (issue #96) toma TODA tecla emquanto está aberto, e
    // por isso trata-se ANTES do rato e do modo de digitar. É o mesmo logar em
    // que o campo já consome, e pela mesma razão: sem elle, a seta andava na
    // pauta por baixo do menu, e o alvo mudava sem que ninguem o visse.
    // O HELP (issue #133) toma TODA tecla e todo clique emquanto está aberto,
    // e trata-se ANTES do menu e do campo: é a peça mais de cima da tela, e o
    // que está por baixo d'ella não ha de mudar debaixo de quem lê.
    if (ajuda.aberta) {
      const ftxui::Dimensions tela = ftxui::Terminal::Size();
      const std::size_t maxima = tui::rolagem_maxima(tui::medida_da_ajuda(
          tela.dimx > 0 ? static_cast<std::size_t>(tela.dimx) : 0,
          tela.dimy > 0 ? static_cast<std::size_t>(tela.dimy) : 0));
      if (tecla.is_mouse()) {
        ftxui::Event copia = tecla;  // o punho do rato é não const no FTXUI
        tui::rato_na_ajuda(ajuda, caixa_da_ajuda, copia.mouse(), maxima);
        return true;
      }
      tui::tecla_na_ajuda(ajuda, tecla, maxima);
      return true;
    }
    tui::Ordem ordem_do_menu;
    if (menu.aberto) {
      const tui::RespostaDoMenu escolha = tui::tecla_no_menu(menu, tecla);
      switch (escolha.pedido) {
        case tui::PedidoDoMenu::Nada: return true;  // consumida, e nada mais
        // Os TRES que a tecla já cumpre desaguam na taboada de sempre, e não
        // ganham caminho proprio: TOCAR é o Entra da eleita, e os outros dous
        // são o F2 e o Delete. Dous caminhos para renomear divergiriam na
        // primeira issue que mexesse n'um d'elles.
        case tui::PedidoDoMenu::Toca: ordem_do_menu = {tui::Verbo::Entra}; break;
        case tui::PedidoDoMenu::Renomeia:
          ordem_do_menu = {tui::Verbo::RenomeiaFaixa};
          break;
        case tui::PedidoDoMenu::Apaga:
          ordem_do_menu = {tui::Verbo::ApagaFaixa};
          break;
        case tui::PedidoDoMenu::Junta:
          junta_na_lista(escolha.lista);
          return true;
        case tui::PedidoDoMenu::NovaLista:
          digita = Digita::NomeComEsta;
          termo_em_curso.clear();
          return true;
      }
    }
    // O RATO (issue #95) trata-se AQUI, antes do modo de digitar: dentro do modo
    // toda tecla se engole, e o clique nunca chegaria a fechar o campo.
    tui::Ordem ordem_do_rato;
    // O ALVO vem do RATO quando o evento é do rato, e do FOCO quando é o Enter
    // ou o Espaço n'uma peça que não é a pauta (issue #107). A taboada do gesto
    // é a MESMA, e é isso que faz a tecla apertar o botão exactamente como o
    // dedo o aperta: caminho proprio daria duas verdades sobre o que cada peça
    // faz, e ellas desencontrar-se-hiam na primeira issue que mexesse n'uma.
    //
    // A guarda do `digita` é o que deixa o campo e a pergunta ficarem com o
    // Enter d'elles: com modo modal aberto, este ramo não corre.
    const bool pelo_foco = !tecla.is_mouse() && digita == Digita::Nada &&
                           foco != tui::Focavel::Pauta &&
                           (tecla == ftxui::Event::Return ||
                            tecla == ftxui::Event::Character(' '));
    // O ARRASTO (issue #153) trata-se ANTES da taboada do clique: elle governa
    // o botão a descer, a mão a andar e o botão a subir, e sómente deixa seguir
    // o caminho de sempre quando não ha movimento a cumprir. A vista diz se se
    // deixa arrumar: o acervo e o dentro de uma lista sim; os artistas, os
    // albuns e os achados da rede não, que alli a ordem não é do operador.
    if (tecla.is_mouse() && digita == Digita::Nada) {
      ftxui::Event d_agora = tecla;
      const ftxui::Mouse mao = d_agora.mouse();
      const bool pode_arrumar = navegador.secao() == tui::Secao::Busca ||
                                navegador.secao() == tui::Secao::NoRol;
      const tui::RespostaDoArrasto d_elle = tui::gesto_do_arrasto(
          arrasto, tui::alvo_do_ponto(caixas, mao.x, mao.y), mao.button,
          mao.motion, pode_arrumar);
      switch (d_elle.gesto) {
        case tui::GestoDoArrasto::Arrasta: return true;  // sómente o pintor muda
        case tui::GestoDoArrasto::Larga:
          arruma_a_faixa(d_elle.de, d_elle.para);
          return true;
        // O PEGA e o DESISTE seguem á taboada do clique: pegar é eleger, e
        // desistir é o clique simples, que toca a faixa já eleita.
        case tui::GestoDoArrasto::Pega:
        case tui::GestoDoArrasto::Desiste:
        case tui::GestoDoArrasto::Nada: break;
      }
    }
    if (tecla.is_mouse() || pelo_foco) {
      ftxui::Event copia = tecla;  // o punho do rato é não const no FTXUI
      const ftxui::Mouse rato = pelo_foco ? ftxui::Mouse{} : copia.mouse();
      const tui::Retracto agora = retracto_do(tocador, projector);
      const tui::GestoDoRato gesto = tui::gesto_do_alvo(
          pelo_foco ? tui::alvo_do_foco(foco)
                    : tui::alvo_do_ponto(caixas, rato.x, rato.y),
          pelo_foco ? ftxui::Mouse::Left : rato.button,
          pelo_foco ? ftxui::Mouse::Pressed : rato.motion,
          {digita != Digita::Nada, navegador.eleito(),
           navegador.vista().size(),
           // A duração vae ZERO com a janella do video de pé, e a guarda do
           // rato faz o resto: o clique na barra fica INERTE. A duração que
           // este retracto sabe é a do AUDIO pausado, e o video que corre é a
           // ELEITA, que nem sempre é a mesma faixa: video de cinco minutos com
           // audio de tres mandaria o video ao minuto tres por um clique no fim
           // da barra. A tecla já decidiu o mesmo por outro caminho, a busca
           // RELATIVA: d'ella não se sabe a posição sem lhe perguntar pelo
           // soquete, e a barra tambem não anda emquanto ella corre.
           agora.video ? 0.0 : agora.duracao});
      switch (gesto.gesto) {
        case tui::Gesto::Nada: return true;  // consumido: lixo que não vaza
        case tui::Gesto::FechaCampo:
          digita = Digita::Nada;
          termo_em_curso.clear();
          return true;
        case tui::Gesto::VaiParaAba: {
          // O indice vem da caixa que o dedo achou, e a ordem d'ellas é a da
          // fita. Aba fóra das tres não ha: a taboada do alvo só nomeia as
          // caixas que existem, e o `default` cahiria na primeira, calado.
          constexpr tui::Aba kAbas[3] = {tui::Aba::MySong, tui::Aba::Playlists,
                                         tui::Aba::Download};
          if (gesto.indice < 3) vai_para_aba(kAbas[gesto.indice]);
          return true;
        }
        case tui::Gesto::Elege:
        case tui::Gesto::Toca:
          // Anda-se pelo sobe e pelo desce, que SATURAM: o navegador não ganha
          // `vai_a` por isto, e nem precisa, que o indice clicado está dentro
          // da fatia á vista. No Toca a volta é de zero passos.
          while (navegador.eleito() != gesto.indice) {
            const std::size_t antes = navegador.eleito();
            antes < gesto.indice ? navegador.desce() : navegador.sobe();
            if (navegador.eleito() == antes) break;  // saturou: acabou a lista
          }
          // O Toca SEGUE, e não pára aqui: elle é o Entra, e quem o cumpre é a
          // taboada de baixo. Parando, o segundo clique elegia o que já estava
          // eleito e mais nada, que foi o que a prova no pty accusou.
          if (gesto.gesto == tui::Gesto::Elege) return true;
          ordem_do_rato = {tui::Verbo::Entra};
          break;
        case tui::Gesto::RodaSobe:
        case tui::Gesto::RodaDesce:
          for (std::size_t passo = 0; passo < gesto.indice; ++passo)
            gesto.gesto == tui::Gesto::RodaSobe ? navegador.sobe()
                                                : navegador.desce();
          return true;
        // Os DOUS MODOS pelo segmento que os mostra. Alternar e cyclar são
        // punhos do tocador, e não «ler o retracto e depois escrever»: entre a
        // leitura e a escripta caberia o socket, e o clique assentaria o
        // contrario do que se viu.
        case tui::Gesto::AbreMenu: abre_o_menu_na(gesto.indice); return true;
        case tui::Gesto::Ajuda: tui::alterna_a_ajuda(ajuda); return true;
        case tui::Gesto::Embaralha: tocador.alterna_embaralhar(); return true;
        case tui::Gesto::Repete: tocador.cicla_repetir(); return true;
        // O MUDO pelo segmento do volume, e pela mesma razão dos dous modos:
        // quem guarda o numero e quem o devolve é o TOCADOR, de uma tomada só.
        case tui::Gesto::Muda: tocador.alterna_mudo(); return true;
        // Os que viram ORDEM. Não se cumprem aqui: desaguam na taboada de
        // sempre, que é quem sabe roteá-las ao video quando elle está de pé.
        case tui::Gesto::Anterior: ordem_do_rato = {tui::Verbo::Anterior}; break;
        case tui::Gesto::Proxima: ordem_do_rato = {tui::Verbo::Proxima}; break;
        case tui::Gesto::Busca:
          ordem_do_rato = {tui::Verbo::Buscar, gesto.alvo};
          break;
        case tui::Gesto::PausaOuRetoma:
          // O ⏯ e a capa perguntam á MESMA taboada do espaço: duas taboadas
          // dariam duas verdades sobre o que alternar quer dizer.
          ordem_do_rato =
              tui::ordem_da_tecla(ftxui::Event::Character(' '), agora);
          break;
      }
      // Parado não ha o que pausar, e ahi o gesto morre aqui, consumido.
      if (ordem_do_rato.verbo == tui::Verbo::Nada) return true;
    }
    // O MODO DE DIGITAR trata-se PRIMEIRO, e por inteiro: assim não ha caminho
    // por onde uma tecla chegue ás duas leituras.
    // A CONFIRMAÇÃO não é modo de digitar: é uma pergunta de uma tecla. Trata-se
    // antes do resto para que a letra «s» não vá parar ao termo em curso.
    if (digita == Digita::Confirma || digita == Digita::ConfirmaFaixa) {
      if (tecla == ftxui::Event::Character('s') ||
          tecla == ftxui::Event::Character('S')) {
        const bool era_faixa = digita == Digita::ConfirmaFaixa;
        digita = Digita::Nada;
        if (era_faixa) {
          aviso_da_rede =
              apaga_a_faixa(navegador.caminho_eleito(), livraria, roleiro);
          navegador.recarrega();  // a faixa sae da pauta no mesmo quadro
        } else if (!navegador.apaga_rol()) {
          aviso_da_rede = "não se pôde apagar";
        }
        return true;
      }
      if (tecla.is_character() || tecla == ftxui::Event::Escape ||
          tecla == ftxui::Event::Return) {
        digita = Digita::Nada;  // qualquer outra tecla é «não»
        return true;
      }
      return true;
    }
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
        } else if (era == Digita::NomeNovo) {
          if (!navegador.cria_rol(termo_em_curso))
            aviso_da_rede = "esse nome já existe, ou é vazio";
        } else if (era == Digita::NomeComEsta) {
          cria_a_lista_com(termo_em_curso);
        } else if (era == Digita::TituloOutro) {
          const std::string qual = navegador.caminho_eleito();
          aviso_da_rede = renomeia_a_faixa(qual, termo_em_curso, livraria);
          navegador.recarrega();  // a pauta reflecte no mesmo quadro
        } else if (era == Digita::NomeOutro) {
          if (!navegador.renomeia_rol(termo_em_curso))
            aviso_da_rede = "esse nome já existe, ou é vazio";
        } else if (era == Digita::Lista) {
          if (!termo_em_curso.empty()) {
            {
              std::lock_guard<std::mutex> chave(tranca_do_termo);
              url_da_lista = termo_em_curso;
            }
            pede_catalogo.store(true);
            aviso_da_rede = "a ler a lista do Spotify...";
          }
        } else if (era == Digita::Procura) {
          if (!termo_em_curso.empty()) {
            nucleo::Fonte fonte = nucleo::Fonte::YouTube;
            {
              std::lock_guard<std::mutex> chave(tranca_do_termo);
              termo_da_rede = termo_em_curso;
              fonte = fonte_da_busca;
            }
            if (fonte == nucleo::Fonte::Spotify) {
              busca_no_catalogo(termo_em_curso);
            } else {
              pede_buscar.store(true);
              aviso_da_rede = "a perguntar á rede...";
            }
          }
        } else if (!termo_em_curso.empty()) {
          const std::string url = termo_em_curso;
          if (nucleo::eh_playlist_url(url)) {
            const nucleo::Fonte fonte =
                !nucleo::id_da_playlist(url).empty()
                    ? nucleo::Fonte::Spotify
                    : (url.find("music.youtube.com") != std::string::npos
                           ? nucleo::Fonte::YouTubeMusic
                           : nucleo::Fonte::YouTube);
            {
              std::lock_guard<std::mutex> chave(tranca_do_termo);
              url_da_playlist = url;
              fonte_da_playlist = fonte;
            }
            baixa_playlist_ao_chegar.store(true);
            if (fonte == nucleo::Fonte::Spotify) {
              {
                std::lock_guard<std::mutex> chave(tranca_do_termo);
                url_da_lista = url;
              }
              pede_catalogo.store(true);
              aviso_da_rede = "a ler a playlist do Spotify...";
            } else {
              pede_playlist.store(true);
              aviso_da_rede = "a ler a playlist...";
            }
            return true;
          }
          // A baixa vae ao ESTALEIRO, e não a um fio erguido aqui. Elle tem o limite
          // declarado, conta o andamento, e a tela lê-o: duas encommendas seguidas
          // não se atropelam, e a segunda espera em vez de disputar a rede.
          nucleo::Pedido pedido;
          pedido.url = url;  // o resto vem da rede: o operador não disse
          estaleiro.encommenda(pedido);
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

    // AS SETAS (issue #107) andam pelo LAYOUT, e deixaram de voltar e de
    // entrar. Tratam DEPOIS do campo e da pergunta, e pela mesma razão que
    // ellas: com modo modal aberto, a tela não ha de mudar debaixo de quem
    // está a responder. Modo modal que venha depois d'este (o menu de contexto
    // da issue #96) trata-se ACIMA d'esta linha, e as setas cedem-lhe sem que
    // este ramo saiba d'elle.
    //
    // Dentro da PAUTA o `↑` e o `↓` continuam a andar na LISTA, e sómente no
    // alto d'ella o `↑` sobe ao cabeçalho: sahir da lista á primeira seta
    // tiraria ao operador o gesto que elle mais faz. O `←` e o `→` sahem para
    // as visinhas, e á esquerda da pauta não ha visinha alguma: a seta FICA, e
    // não volta degrau algum, que voltar é o Escape e o Backspace.
    if (const tui::Direcao rumo = tui::rumo_da_tecla(tecla);
        rumo != tui::Direcao::Nenhuma) {
      if (foco == tui::Focavel::Pauta) {
        if (rumo == tui::Direcao::Baixo) {
          navegador.desce();
          return true;
        }
        if (rumo == tui::Direcao::Cima && navegador.eleito() > 0) {
          navegador.sobe();
          return true;
        }
      }
      foco = tui::salto(caixas, foco, rumo);
      return true;
    }

    // AS TECLAS DAS ABAS (issue #102) tratam DEPOIS do campo e ANTES da
    // taboada geral. É a ordem que a barra tinha, e pela mesma razão: com o
    // campo aberto, o `1` é o algarismo um do termo, e não a aba.
    //
    // O ramo Alheio não retorna, e é elle que faz o atalho de sempre valer:
    // tecla que o cabeçalho não conhece segue ao commando e faz o que sempre
    // fez, sem que este arquivo repita a taboada de lá.
    switch (const tui::OrdemDaAba d_ella = tui::ordem_da_aba(tecla);
            d_ella.gesto) {
      case tui::GestoDaAba::Vai: vai_para_aba(d_ella.aba); return true;
      case tui::GestoDaAba::Cycla: {
        // Tenta até TRES, e SALTA a aba sem chão. Sem o salto o Tab ficava
        // preso nas PLAYLISTS emquanto não houvesse busca na rede feita, que é
        // o estado de nascença: a tecla que a issue annuncia como «cicla as
        // tres abas» não ciclava, e carregar n'ella duas vezes dava o mesmo
        // recado e a mesma tela.
        tui::Aba qual = tui::aba_da_secao(navegador.secao());
        for (int volta = 0; volta < 3; ++volta) {
          qual = tui::aba_seguinte(qual);
          if (navegador.vai_para(tui::secao_da_aba(qual))) return true;
          // Saltou-se, e diz-se PORQUE: aba a passar em silencio deixaria o
          // operador a crer que o Tab pulou uma por engano d'elle.
          aviso_da_rede = "DOWNLOAD saltada: a rede está vazia (s busca)";
        }
        return true;
      }
      case tui::GestoDaAba::CyclaVista: {
        // Fóra das MY SONG a tecla fica MUDA. A vista só se cycla alli, e a
        // chapa di-lo calando a palavra d'ella: sem esta guarda, um `o` por
        // engano dentro de uma lista ou nos achados da rede abandonava a
        // secção, e signal algum na tela dizia que a tecla fazia cousa alguma.
        if (tui::nome_da_vista(navegador.secao()).empty()) return true;
        const tui::Secao alvo = tui::vista_seguinte(
            navegador.secao(), !navegador.vista().empty());
        // Dos ARTISTAS DESCE-SE no eleito: os albuns que a bibliotheca sabe
        // listar são os D'ELLE, e `vai_para` recusaria o degrau sem trilha.
        if (alvo == tui::Secao::Albuns) navegador.entra();
        else navegador.vai_para(alvo);
        return true;
      }
      case tui::GestoDaAba::Alheio: break;
    }

    // A TECLA `m` (issue #96): o menu sobre a ELEITA, que é o par de teclado do
    // botão direito. Trata-se AQUI, depois do campo e das abas, e não na
    // taboada do commando: ella não dá Ordem alguma, e verbo que sómente
    // abrisse caixa da tela seria verbo que o tocador nunca cumpriria.
    if (tecla == ftxui::Event::Character('m')) {
      abre_o_menu_na(navegador.eleito());
      return true;
    }
    // A ordem vem do RATO quando o evento é do rato, e da tecla quando é da
    // tecla: a `ordem_da_tecla` não vê evento de rato algum, e o `switch`
    // abaixo cumpre-a sem saber por qual das duas portas ella entrou.
    const tui::Ordem ordem =
        ordem_do_menu.verbo != tui::Verbo::Nada ? ordem_do_menu
        : tecla.is_mouse() || pelo_foco
            ? ordem_do_rato
            : tui::ordem_da_tecla(tecla, retracto_do(tocador, projector), false);
    switch (ordem.verbo) {
      case tui::Verbo::Nada: return false;  // tecla alheia segue
      case tui::Verbo::Desce: navegador.desce(); return true;
      case tui::Verbo::Sobe: navegador.sobe(); return true;
      case tui::Verbo::AoPrincipio: navegador.ao_principio(); return true;
      case tui::Verbo::AoFim: navegador.ao_fim(); return true;
      case tui::Verbo::Volta:
        // No alto, a seta esquerda fica INERTE. Até a issue #102 ella abria a
        // barra, que era o unico logar mais á esquerda que havia; a aba está
        // agora ACIMA, e quem quer trocar de secção tem os algarismos e o Tab.
        navegador.volta();
        return true;
      case tui::Verbo::AbreBusca:
        digita = Digita::Busca;
        termo_em_curso.clear();
        return true;
      case tui::Verbo::AbreBaixa:
        digita = Digita::Url;
        termo_em_curso.clear();
        return true;
      case tui::Verbo::TrocaLetra:
        mostra_letra.store(!mostra_letra.load());
        return true;
      case tui::Verbo::Ajuda: tui::alterna_a_ajuda(ajuda); return true;
      case tui::Verbo::AbreVideo: {
        // O AUDIO CALA-SE PRIMEIRO, e sómente depois a janella abre. Nesta ordem,
        // e não na contraria: abrindo primeiro, ha um instante com os dous a tocar,
        // e é justamente o dobro que a tarefa proibe.
        const std::string qual = navegador.caminho_eleito();
        if (qual.empty()) {
          aviso_da_rede = "elege uma faixa primeiro";
          return true;
        }
        if (!nucleo::tem_video(qual)) {
          aviso_da_rede = std::string(nucleo::razao_da_fita(
              nucleo::Fita::SemVideo));
          return true;
        }
        tocador.pausar();
        const nucleo::Fita fita = projector.abre(qual);
        aviso_da_rede = fita == nucleo::Fita::Rodando
                            ? std::string()
                            : std::string(nucleo::razao_da_fita(fita));
        return true;
      }
      case tui::Verbo::AbreCatalogo:
        digita = Digita::Lista;
        termo_em_curso.clear();
        return true;
      case tui::Verbo::BaixaTudo: {
        // TODAS as faixas da lista, e sómente d'esta secção: o `T` n'outro logar não
        // ha de encommendar cousa alguma por engano.
        if (navegador.secao() != tui::Secao::Lista) {
          aviso_da_rede = "isso sómente na lista do Spotify (I)";
          return true;
        }
        const std::string lista = navegador.nome_do_catalogo();
        std::size_t quantas = 0;
        for (const nucleo::FaixaDoCatalogo& faixa : navegador.faixas_do_catalogo()) {
          estaleiro.encommenda(encommenda_do_catalogo(faixa, lista));
          ++quantas;
        }
        aviso_da_rede = std::to_string(quantas) + " encommendadas";
        return true;
      }
      case tui::Verbo::AbreProcura:
        digita = Digita::Procura;
        termo_em_curso.clear();
        return true;
      case tui::Verbo::TrocaFonte: {
        // Troca-se OLHANDO a lista da rede, e sómente ahi: fóra d'ella o f diz
        // onde o gesto vale, em vez de mudar estado que não está á vista.
        if (navegador.secao() != tui::Secao::Rede) {
          aviso_da_rede = "a fonte troca-se na secção da rede (s)";
          return true;
        }
        std::string termo;
        nucleo::Fonte fonte = nucleo::Fonte::YouTube;
        {
          std::lock_guard<std::mutex> chave(tranca_do_termo);
          fonte_da_busca = nucleo::proxima_fonte(fonte_da_busca);
          fonte = fonte_da_busca;
          termo = termo_da_rede;
        }
        // Trocar de fonte NÃO apaga o termo: havendo um já buscado, re-busca-se
        // o MESMO na fonte nova, que é o que dá as tres listas para comparar.
        // Sem termo, muda só o cabeçalho, e busca alguma se dispara.
        if (termo.empty()) return true;
        if (fonte == nucleo::Fonte::Spotify) {
          busca_no_catalogo(termo);
        } else {
          pede_buscar.store(true);
          aviso_da_rede = "a perguntar á rede...";
        }
        return true;
      }
      case tui::Verbo::ApagaFaixa: {
        const std::string qual = navegador.caminho_eleito();
        if (qual.empty()) {
          aviso_da_rede = "elege uma faixa primeiro";
          return true;
        }
        nucleo::Faixa d_ella;
        livraria.acha_por_caminho(qual, d_ella);
        titulo_em_causa = d_ella.titulo;
        digita = Digita::ConfirmaFaixa;
        return true;
      }
      case tui::Verbo::RenomeiaFaixa: {
        // O campo abre já com o titulo CORRENTE, que é o que o índice guarda:
        // o operador corrige uma lettra em vez de digitar o nome outra vez.
        const std::string qual = navegador.caminho_eleito();
        if (qual.empty()) {
          aviso_da_rede = "elege uma faixa primeiro";
          return true;
        }
        nucleo::Faixa d_ella;
        livraria.acha_por_caminho(qual, d_ella);
        digita = Digita::TituloOutro;
        termo_em_curso = d_ella.titulo;
        return true;
      }
      case tui::Verbo::AbreRois:
        navegador.mostra_rois();
        return true;
      case tui::Verbo::CriaRol:
        digita = Digita::NomeNovo;
        termo_em_curso.clear();
        return true;
      case tui::Verbo::RenomeiaRol:
        if (!navegador.nome_do_rol_eleito().empty()) {
          digita = Digita::NomeOutro;
          termo_em_curso = navegador.nome_do_rol_eleito();
        }
        return true;
      case tui::Verbo::ApagaRol:
        // PERGUNTA-SE. É o unico verbo d'esta obra que apaga cousa que o operador
        // fez á mão, e apagar sem perguntar é o que a tarefa proibe.
        if (!navegador.nome_do_rol_eleito().empty()) digita = Digita::Confirma;
        return true;
      case tui::Verbo::JuntaAoRol: {
        // A faixa eleita vae á lista CORRENTE. Fóra de uma lista não ha corrente, e
        // ahi diz-se o que falta em vez de se calar.
        const std::string qual = navegador.caminho_eleito();
        if (qual.empty())
          aviso_da_rede = "elege uma faixa primeiro";
        else if (!navegador.junta_ao_rol(qual))
          aviso_da_rede = "entra n'uma lista primeiro (P)";
        return true;
      }
      case tui::Verbo::RetiraDoRol:
        if (!navegador.retira_do_rol())
          aviso_da_rede = "isso sómente dentro de uma lista";
        return true;
      // O `K` e o `J` (issue #153) valem tambem no ACERVO, pela ordem propria
      // d'elle: era arbitrario moverem item de lista e não moverem faixa, que
      // o operador arruma as duas pelo mesmo gesto.
      case tui::Verbo::SobeNoRol:
        if (navegador.secao() == tui::Secao::Busca) {
          if (navegador.eleito() > 0)
            arruma_a_faixa(navegador.eleito(), navegador.eleito() - 1);
        } else {
          navegador.sobe_no_rol();
        }
        return true;
      case tui::Verbo::DesceNoRol:
        if (navegador.secao() == tui::Secao::Busca) {
          if (navegador.eleito() + 1 < navegador.vista().size())
            arruma_a_faixa(navegador.eleito(), navegador.eleito() + 1);
        } else {
          navegador.desce_no_rol();
        }
        return true;
      case tui::Verbo::Varre:
        // Uma varredura por vez, e não vinte: o fio da varredura toma o pedido e
        // apaga-o, donde carregar dez vezes no `r` durante uma varredura não
        // enfileira dez varreduras.
        if (varrida.load()) pede_varrer.store(true);
        return true;
      case tui::Verbo::Entra: {
        // Na LISTA do Spotify, entrar é BAIXAR a eleita. A URL vae vazia, e a
        // aquisição busca o audio por si: é o casamento pela duração.
        if (navegador.ha_faixa_de_catalogo()) {
          estaleiro.encommenda(encommenda_do_catalogo(
              navegador.faixa_de_catalogo_eleita(), navegador.nome_do_catalogo()));
          return true;
        }
        // Na REDE, entrar é BAIXAR o achado eleito, pela encommenda que elle dá:
        // a URL e o que a fonte soube dizer (artista, album, titulo canonico,
        // ano, fonte). O achado comum dá o pedido só de URL, que é o de hoje; o
        // caminho_eleito segue vazio n'esta secção, para que endereço algum
        // cahia na fila do motor. Quem baixa é o estaleiro, como sempre.
        if (navegador.ha_achado()) {
          estaleiro.encommenda(
              nucleo::encommenda_do_achado(navegador.achado_eleito()));
          return true;
        }
        // Dentro de uma lista, entrar enche a fila com a lista TODA na ordem
        // gravada, e não sómente com a faixa eleita: é o que a tarefa pede quando diz
        // que tocar a lista enche a fila. Começa-se na eleita, que é onde o dedo está.
        if (navegador.secao() == tui::Secao::NoRol) {
          const std::size_t eleita = navegador.eleito();
          const std::size_t antes = tocador.retracto().tamanho;
          std::size_t quantas = 0;
          for (const tui::Linha& linha : navegador.vista()) {
            tocador.junta(linha.chave);
            ++quantas;
          }
          if (quantas > 0) {
            tocador.ir_para(antes + std::min(eleita, quantas - 1));
            tocador.tocar_corrente();
          }
          return true;
        }
        // O navegador diz SE era faixa; a decisão de tocar é d'esta funcção, que
        // é quem tem o tocador na mão.
        if (navegador.entra()) {
          const std::string caminho = navegador.caminho_eleito();
          if (!caminho.empty()) {
            // O tamanho novo vem da propria juntada: o ultimo é elle menos um.
            tocador.ir_para(tocador.junta(caminho) - 1);
            tocador.tocar_corrente();
          }
        }
        return true;
      }
      default:
        cumprir(ordem, tocador, projector, sahir);
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
      // O SOCKET bate AQUI, e não em fio proprio: é o que o cabeçalho d'elle
      // manda, e a razão é que ordem alguma se intercale no meio de uma
      // transição do nucleo. Batida alguma se bloqueia (o poll espera zero),
      // donde cliente mudo não trava nem o tocador nem os outros clientes.
      if (servidor) servidor->pulsa();
      // Colheu-se faixa nova: pede-se varredura. A bandeira do estaleiro CONSOME-SE
      // na leitura, donde isto sahe uma vez por colheita, e não a cada quadro.
      if (estaleiro.colheu()) pede_varrer.store(true);
      analisador.pulsa();
      mpris.pulsa();
      // SÓMENTE quando o que se vê muda (issue #48), e SÓMENTE com olhos no
      // painel (issue #82): a vigilia governa o desenho e nada mais; os
      // pulsos acima nunca dormem, que a musica não pára por falta de platéa.
      // Ao acordar esquece-se a assignatura, porque o que mudou dormindo não
      // se pintou, e o primeiro quadro desperto ha de sahir completo.
      if (vigilia.acordou()) ultima_assignatura.clear();
      if (vigilia.pede_batida()) {
        const std::string agora = assignatura_do_visivel(
            tocador, nucleo::texto_do_andamento(estaleiro.andamento()),
            mostra_letra.load(), varrida.load(),
            correio.geracao() + correio_do_catalogo.geracao() +
                correio_da_playlist.geracao() +
                correio_da_onda.geracao(),
            projector.rodando());
        if (agora != ultima_assignatura) {
          ultima_assignatura = agora;
          tela.PostEvent(ftxui::Event::Custom);
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(MILESIMOS_DO_QUADRO));
    }
  });

  // PEDE-SE O FOCO ao terminal (issue #82): com o modo 1004 elle manda
  // ESC [ I e ESC [ O a cada troca, e é d'esses avisos que a vigilia vive.
  // Liga-se antes do Loop e desliga-se logo depois, no mesmo assentar e
  // desfazer que o FTXUI pratica com o que é d'elle; aviso que chegue antes
  // do parser espera no buffer do tty, que o Install não descarta entrada.
  //
  // E PEDE-SE O RATO com elle (issue #95): o 1000 dá o botão a descer e a subir,
  // e o 1006 dá-os em SGR. O 1003 fica de fóra, e é esse o ponto da issue.
  std::cout << "\x1b[?1004h\x1b[?1000h\x1b[?1002h\x1b[?1006h" << std::flush;
  tela.Loop(janella);
  std::cout << "\x1b[?1006l\x1b[?1002l\x1b[?1000l\x1b[?1004l" << std::flush;
  sahir.store(true);  // a sahida pela tela tambem para o relogio
  relogio.join();
  // Os fios de fundo esperam-se TODOS: elles têm referencia para bandeiras e para o
  // banco, que vivem nesta pilha. Deixar um solto é fio a ler memoria de quadro já
  // desfeito, e isso não perdoa.
  for (std::thread& fio : ao_fundo)
    if (fio.joinable()) fio.join();
  // A FALTA DECLARA-SE (issue #82): pediu-se o aviso de foco e aviso algum
  // veio na sessão inteira. Diz-se o FATO, e não a culpa, que saber se o
  // terminal é incapaz ou se ninguem trocou de foco não se pode; e diz-se
  // depois da tela, no stderr, como o relatorio dos requisitos se diz.
  // As ordens da capa que o cano não coube (issue #103). Dizem-se DEPOIS da
  // tela, no stderr, pelo molde exacto da linha da vigilia abaixo: byte algum
  // sahe por baixo de um quadro do FTXUI.
  if (lousa.descartadas() > 0)
    std::cerr << "mysong: " << lousa.descartadas()
              << " ordens da capa não couberam no cano do ueberzugpp e"
                 " descartaram-se; a capa pode ter piscado.\n";
  if (!vigilia.ha_noticia())
    std::cerr << "mysong: evento de foco nenhum veio nesta sessão; o relogio "
                 "nunca dormiu. Dentro do tmux, «set -g focus-events on» é o "
                 "que o faz chegar.\n";
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

// O CURL DA CASA. O libcurl ergue-se UMA vez, antes de fio algum: sem esta
// chamada a inicialização implicita corre dentro do primeiro curl_easy_init, e
// os dous obreiros da baixa podem chegar lá juntos. Que o curl de hoje tolere
// isso é accidente, e não garantia. Ergue-se AQUI, e não n'um dos tres modulos
// que o usam, porque aqui é que a Casa ergue o que é do processo inteiro; e por
// objecto, porque main() tem quatro sahidas e esquecer uma seria vasamento.
struct CurlDaCasa {
  CurlDaCasa() { (void)curl_global_init(CURL_GLOBAL_DEFAULT); }
  ~CurlDaCasa() { curl_global_cleanup(); }
  CurlDaCasa(const CurlDaCasa&) = delete;
  CurlDaCasa& operator=(const CurlDaCasa&) = delete;
};

}  // namespace

int main(int argc, char** argv) {
  const CurlDaCasa curl_da_casa;
  // A linha lê-se ANTES de a sonda correr. Quem pergunta a versão ou a ajuda
  // não está a abrir o tocador, e a recusa dos requisitos não lhe cabe: é o
  // que faz `mysong --versao` responder na machina sem a Nerd Font.
  const nucleo::Invocacao invocacao = nucleo::ler_linha(argc, argv);
  if (invocacao.modo == nucleo::Modo::Ajuda) {
    std::cout << nucleo::texto_da_ajuda();
    return 0;
  }
  if (invocacao.modo == nucleo::Modo::Versao) {
    std::cout << nucleo::texto_da_versao();
    return 0;
  }
  // DOUS, e não um: o codigo 1 já é o do impedimento de requisito, e dar o
  // mesmo aqui tiraria a script alguma o meio de distinguir a falta da fonte
  // do erro de escripta na opção.
  if (invocacao.modo == nucleo::Modo::Recusa) {
    std::cerr << invocacao.razao;
    return 2;
  }

  const nucleo::Relatorio relatorio =
      nucleo::sondar(nucleo::inquerito_do_systema());

  // OS AJUSTES, colhidos antes de tudo e UMA vez só: aqui não ha fio algum
  // erguido ainda, e ler variavel de ambiente com fios a correr é corrida. A
  // fila da linha de commando colhe-se no mesmo laço, que a bandeira do acervo
  // não é faixa e não ha de cahir na fila do tocador.
  std::vector<std::string> faixas;
  std::optional<std::string> acervo_pedido;
  for (int i = 1; i < argc; ++i)
    if (!nucleo::eh_acervo(argv[i], &acervo_pedido))
      faixas.emplace_back(argv[i]);
  const nucleo::Ajustes ajustes = nucleo::ajustes_do_systema(acervo_pedido);

  // O modo de diagnostico: texto puro, tela nenhuma, e codigo differente de
  // zero havendo impedimento, para que sirva de guarda em script. Queixa de
  // configuração NÃO muda esse codigo: arquivo velho não é requisito ausente.
  if (invocacao.modo == nucleo::Modo::Sonda) {
    std::cout << tui::texto_do_relatorio(relatorio);
    std::cout << api::texto_do_socket();
    std::cout << nucleo::texto_dos_ajustes(ajustes);
    // A LOUSA (issue #103) diz-se DEPOIS dos ajustes, e não na taboa dos
    // requisitos: o `ueberzugpp` não é requisito d'esta obra, e pol-o lá faria
    // o operador sem X11 ler «falta» de uma cousa que não lhe falta.
    const std::string versao_da_lousa = nucleo::versao_da_lousa();
    std::cout << nucleo::texto_da_lousa(
        nucleo::parecer_da_lousa(ajustes.lousa.valor, nucleo::ha_display(),
                                 !versao_da_lousa.empty()),
        versao_da_lousa);
    // O LETREIRO (issue #108) logo abaixo d'ella, que d'ella depende: diz
    // «Xirod, pango-view» de pé, e a razão deitado. Tambem fóra da taboa dos
    // requisitos, e pela mesma razão: a XIROD não tranca porta alguma.
    std::cout << nucleo::texto_do_letreiro(nucleo::parecer_do_letreiro(
        ajustes.lousa.valor, nucleo::ha_pango_view(),
        nucleo::ha_familia_da_marca()));
    return relatorio.ha_impedimento() ? 1 : 0;
  }

  // O modo da capa (issue #83): texto puro, tela nenhuma, e rede SÓ por esta
  // ordem. Varre-se ANTES, com a mesma Varredura da tela: o índice se
  // reconstroe a cada varredura por desenho, e caçar sobre índice velho
  // abriria arquivo já movido.
  if (invocacao.modo == nucleo::Modo::Capa) {
    const std::filesystem::path banco = caminho_do_indice();
    if (banco.empty()) {
      std::cerr << "mysong --capa: sem XDG_DATA_HOME nem HOME nao ha indice.\n";
      return 1;
    }
    nucleo::MemoriaDeCapas memoria(banco.parent_path() / "capas.sqlite3");
    if (!memoria.aberta()) {  // sem memoria toda corrida re-tentaria a rede
      std::cerr << "mysong --capa: a memoria de capas nao abriu ao lado do "
                   "indice.\n";
      return 1;
    }
    const std::filesystem::path acervo = ajustes.acervo.valor;
    std::cout << "a varrer " << acervo.string() << "..." << std::endl;
    {
      nucleo::Varredura varredura(banco, {acervo});
      while (varredura.passo()) {
      }
    }
    nucleo::Biblioteca livraria(banco);
    const std::vector<nucleo::Faixa> fila = livraria.busca_faixa("");
    std::cout << "a caçar capa para " << fila.size() << " faixas (uma "
              << "requisição por segundo)..." << std::endl;
    const nucleo::SommaDaCaca somma = nucleo::caca_capas(
        fila, &memoria, nucleo::consulta_mb_com_estado,
        [](const nucleo::Faixa& faixa, nucleo::CacaDeCapa desfecho) {
          // Linha só para o que muda ou pede olho: mil «já tinha» seriam
          // ruido, e essas vão contadas na somma do remate.
          if (desfecho == nucleo::CacaDeCapa::JaTinha ||
              desfecho == nucleo::CacaDeCapa::JaProcurada)
            return;
          std::cout << faixa.artista << " - " << faixa.titulo << ": "
                    << nucleo::palavra_da_caca(desfecho) << std::endl;
        });
    std::cout << somma.embutidas << " embutidas · " << somma.ja_tinham
              << " já tinham · " << somma.ja_procuradas << " já procuradas · "
              << somma.duvidosas << " duvidosas · " << somma.sem_capa
              << " sem capa · " << somma.fora_do_alcance
              << " fóra do alcance · " << somma.falhas << " falhas\n";
    if (somma.parou_por_recuo)
      std::cerr << "o servidor pediu recuo: a corrida parou; o resto fica "
                   "para a proxima.\n";
    return somma.parou_por_recuo ? 1 : 0;
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

  // A fila vem da linha de commando, que ler_linha já separou das opções: é
  // assim que uma faixa entra por `mysong caminho.mp3 outro.mp3`.
  return erguer_tocador(invocacao.faixas, ajustes);
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
