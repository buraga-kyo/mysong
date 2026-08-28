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
#include <string_view>

#include <unistd.h>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/terminal.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>

#include "api/mpris.hpp"

#include "nucleo/ajustes.hpp"
#include "nucleo/analisador.hpp"
#include "nucleo/capa.hpp"
#include "nucleo/catalogo.hpp"
#include "nucleo/estaleiro.hpp"
#include "nucleo/aquisicao.hpp"
#include "nucleo/fila.hpp"
#include "nucleo/letra.hpp"
#include "nucleo/marca.hpp"
#include "nucleo/motor.hpp"
#include "nucleo/rol.hpp"
#include "nucleo/video.hpp"
#include "nucleo/tocador.hpp"
#include "nucleo/varredura.hpp"
#include "nucleo/sonda.hpp"
#include "tui/commando.hpp"
#include "tui/correio.hpp"
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

// raiz_do_acervo — `$MYSONG_ACERVO`, e sem ella `~/Música`. A variavel existe para
// que o operador com monte de rede não tenha de mover o acervo para casa.
std::filesystem::path raiz_do_acervo() {
  const char* posto = std::getenv("MYSONG_ACERVO");
  if (posto != nullptr && posto[0] != '\0') return std::filesystem::path(posto);
  const char* casa = std::getenv("HOME");
  if (casa == nullptr) return {};
  return std::filesystem::path(casa) / "Música";
}


// QUANTOS achados a busca na rede pede. Quinze: cabe n'uma tabella de terminal sem
// rolar muito, e o `--flat-playlist` faz d'isso uma sonda de rede só.
constexpr int ACHADOS_POR_BUSCA = 15;

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
  marca += agora.faixa;
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
  retracto.tamanho = agora.tamanho;
  if (agora.tamanho > 0) {
    retracto.indice = agora.indice;
    retracto.titulo = agora.faixa;
  }
  return retracto;
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
      break;
  }
}

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
      [](const nucleo::Pedido& pedido, std::filesystem::path* ficou) {
        return nucleo::baixa(raiz_do_acervo(), pedido, ficou);
      });


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
  // A PROCURA entra no mesmo enum, pela mesma razão: tres destinos, e não tres
  // booleanos, que tres booleanos admittem o estado «os tres», que não existe.
  // O NOME e a CONFIRMAÇÃO entram no mesmo enum: são dous destinos mais, e a razão
  // é a mesma que fez a Procura entrar aqui em vez de n'um booleano ao lado.
  enum class Digita {
    Nada, Busca, Url, Procura, NomeNovo, NomeOutro, Confirma, Lista
  } digita = Digita::Nada;
  std::string termo_em_curso;
  // O aviso da rede vive SÓMENTE no fio da tela: quem o escreve é a colheita do
  // correio, que corre no pintor, e quem o lê é o pintor. Fio de fundo algum lhe
  // toca, e por isso elle não pede tranca.
  std::string aviso_da_rede;
  std::size_t primeira_linha = 0;
  // A LETRA carrega-se do disco UMA vez por faixa, e não a cada quadro: ler
  // arquivo vinte vezes por segundo seria gastar disco para nada. A faixa de que
  // ella é guarda-se ao lado, e é a mudança d'essa que dispara a releitura.
  std::vector<nucleo::LinhaDaLetra> letra;
  std::string letra_de_qual;
  // ATOMICO, e não bool nú: o fio do relogio lê-o para saber se as bandas entram na
  // assignatura, e o fio da tela troca-o na tecla `l`.
  std::atomic<bool> mostra_letra{false};
  nucleo::Galeria galeria;  // a capa converte-se uma vez por album e por tamanho

  // Os fios de fundo são POSSUIDOS, e juntam-se antes de esta pilha se desfazer. Antes
  // corriam soltos por `detach()`, e o corpo d'elles referencia objectos d'esta pilha:
  // sahindo o programma primeiro, liam memoria morta. Fio solto que aponta para pilha
  // alheia não se justifica, e agora não ha nenhum.
  // A ultima assignatura do que se vê. Vazia de saida, para que o primeiro quadro sahia.
  std::string ultima_assignatura;
  std::vector<std::thread> ao_fundo;
  // A VARREDURA, em fio permanente que espera por pedido. A conducção por passos da
  // issue #34 existe justamente para isto: o fio pode parar entre dous passos, e a
  // bandeira `sahir` é onde elle olha.
  ao_fundo.emplace_back([&] {
    while (!sahir.load()) {
      if (pede_varrer.exchange(false)) {
        varrida.store(false);
        nucleo::Varredura varredura(banco, {raiz_do_acervo()});
        while (!sahir.load() && varredura.passo()) {
        }
        varrida.store(true);
        acervo_novo.store(true);
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
    std::vector<nucleo::Achado> achados;
    std::string recado;
    if (correio.colhe(&achados, &recado)) {
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
    if (correio_do_catalogo.colhe(&lidos, &recado_da_lista)) {
      if (!lidos.empty() && !lidos.front().faixas.empty())
        navegador.mostra_catalogo(std::move(lidos.front()));
      aviso_da_rede = recado_da_lista;
    }
    // A varredura concluiu: o navegador recarrega UMA vez. A bandeira do acervo novo
    // CONSOME-SE na leitura, donde isto corre uma vez por varredura.
    //
    // Isto corria no fio do RELOGIO, e mudou-se para cá. O navegador é mutado pelo
    // tratador de teclas, que corre no fio da tela; recarregá-lo do relogio era
    // mutá-lo de um fio e lê-lo de outro. O pintor corre no mesmo fio do tratador,
    // donde a corrida sahe. Não é embelleçamento: é o defeito da corrida a fechar-se.
    if (acervo_novo.exchange(false)) {
      livraria.reabre();
      navegador.recarrega();
    }
    const tui::Retracto retracto = retracto_do(tocador, projector);
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
    // A fonte no titulo da secção, SEMPRE: a lista pode ser da fonte anterior por
    // um instante (a busca é assynchrona), e o cabeçalho é a verdade da vigente.
    if (navegador.secao() == tui::Secao::Rede)
      trilha = "NET · " + std::string(nucleo::nome_da_fonte(fonte_da_busca));
    if (navegador.secao() == tui::Secao::Lista) {
      trilha = "SPOTIFY";
      if (!navegador.nome_do_catalogo().empty())
        trilha += "  \ue0b1  " + navegador.nome_do_catalogo();
    }
    if (navegador.secao() == tui::Secao::Rois) trilha = "LISTS";
    if (navegador.secao() == tui::Secao::NoRol) {
      trilha = "LISTS";
      for (const std::string& degrau : navegador.trilha())
        trilha += "  \ue0b1  " + degrau;
    }
    if (digita == Digita::Busca) trilha = "/" + termo_em_curso;
    else if (digita == Digita::Url) trilha = "URL: " + termo_em_curso;
    else if (digita == Digita::Procura)
      trilha = "BUSCA NA REDE (" +
               std::string(nucleo::nome_da_fonte(fonte_da_busca)) +
               "): " + termo_em_curso;
    else if (digita == Digita::Lista) trilha = "PLAYLIST DO SPOTIFY: " + termo_em_curso;
    else if (digita == Digita::NomeNovo) trilha = "LISTA NOVA: " + termo_em_curso;
    else if (digita == Digita::NomeOutro) trilha = "NOME: " + termo_em_curso;
    else if (digita == Digita::Confirma)
      trilha = "apagar «" + navegador.nome_do_rol_eleito() + "»? s/n";
    else if (!navegador.termo().empty()) trilha += "   [" + navegador.termo() + "]";
    // A lista ALVO diz-se sempre que houver alguma, e em toda secção: é para onde o
    // `a` manda a faixa, e o operador não ha de o adivinhar.
    if (navegador.rol_corrente() != 0 &&
        navegador.secao() != tui::Secao::NoRol)
      trilha += "   [\ue0b1 " + navegador.nome_corrente() + "]";
    // A janella do video diz-se enquanto ella viver. Deixando de viver, a linha
    // cala-se por si: é a pergunta ao processo que o diz, e não bandeira nossa que
    // pudesse ficar a mentir.
    if (projector.rodando())
      trilha += "   [video: " + projector.faixa().filename().string() + "]";
    if (!varrida.load()) trilha += "   (a varrer o acervo...)";
    if (!aviso_da_rede.empty()) trilha += "   " + aviso_da_rede;
    const std::string andamento = nucleo::texto_do_andamento(estaleiro.andamento());
    if (!andamento.empty()) trilha += "   " + andamento;

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
               mostra_letra.load()
                   ? tui::elemento_da_letra(
                         letra,
                         nucleo::linha_corrente(letra, retracto.posicao), 8, larg)
                   : tui::elemento_do_espectro(quadro),
               tui::elemento_do_transporte(retracto, larg),
               ftxui::text("↑↓ anda · → entra · ← volta · / filtra · s busca na rede"
                           " · f fonte · b baixa por URL · r varre · l letra · espaço pausa"
                           " · n/p faixa · P listas · c cria · a junta · t retira"
                           " · K/J move · R renomeia · D apaga · v video"
                           " · I spotify · T baixa todas · q sahe") |
                   ftxui::dim,
           }) |
           ftxui::border;
  });

  auto janella = ftxui::CatchEvent(pintor, [&](const ftxui::Event& tecla) {
    // O MODO DE DIGITAR trata-se PRIMEIRO, e por inteiro: assim não ha caminho
    // por onde uma tecla chegue ás duas leituras.
    // A CONFIRMAÇÃO não é modo de digitar: é uma pergunta de uma tecla. Trata-se
    // antes do resto para que a letra «s» não vá parar ao termo em curso.
    if (digita == Digita::Confirma) {
      if (tecla == ftxui::Event::Character('s') ||
          tecla == ftxui::Event::Character('S')) {
        digita = Digita::Nada;
        if (!navegador.apaga_rol()) aviso_da_rede = "não se pôde apagar";
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
          // A baixa vae ao ESTALEIRO, e não a um fio erguido aqui. Elle tem o limite
          // declarado, conta o andamento, e a tela lê-o: duas encommendas seguidas
          // não se atropelam, e a segunda espera em vez de disputar a rede.
          nucleo::Pedido pedido;
          pedido.url = termo_em_curso;  // o resto vem da rede: o operador não disse
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

    const tui::Ordem ordem =
        tui::ordem_da_tecla(tecla, retracto_do(tocador, projector), false);
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
        mostra_letra.store(!mostra_letra.load());
        return true;
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
      case tui::Verbo::SobeNoRol:
        navegador.sobe_no_rol();
        return true;
      case tui::Verbo::DesceNoRol:
        navegador.desce_no_rol();
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
      // Colheu-se faixa nova: pede-se varredura. A bandeira do estaleiro CONSOME-SE
      // na leitura, donde isto sahe uma vez por colheita, e não a cada quadro.
      if (estaleiro.colheu()) pede_varrer.store(true);
      analisador.pulsa();
      mpris.pulsa();
      // SÓMENTE quando o que se vê muda. Parado, isto não pede repintura alguma, e a
      // tela escreve zero: é a correcção da issue #48.
      const std::string agora = assignatura_do_visivel(
          tocador, nucleo::texto_do_andamento(estaleiro.andamento()),
          mostra_letra.load(), varrida.load(),
          correio.geracao() + correio_do_catalogo.geracao(),
          projector.rodando());
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
  if (argc > 1 && std::string_view(argv[1]) == "--sonda") {
    std::cout << tui::texto_do_relatorio(relatorio)
              << nucleo::texto_dos_ajustes(ajustes);
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

  return erguer_tocador(faixas, ajustes);
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
