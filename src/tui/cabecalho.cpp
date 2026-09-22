// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CABEÇALHO, src/tui/cabecalho.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lavra do que cabecalho.hpp promette. As taboadas primeiro, a fita depois.
//
// DOMÍNIO ......... a tecla, a secção, o Retracto e a largura.
// CONTRA-DOMÍNIO .. as abas, os gestos e os elementos do FTXUI.
// INVARIANTE ...... funcção alguma d'aqui lê o mundo: nem banco, nem relogio,
//                   nem tocador. Os `switch` não levam `default`, para que aba
//                   ou secção nova deixe de compilar em vez de sahir muda.
// Q.E.D. .......... sendo tudo funcção de valores, a bateria afere a linha em
//                   écran de papel, cella a cella, sem erguer terminal.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/cabecalho.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <ftxui/screen/string.hpp>

#include "tui/arrowline.hpp"
#include "tui/onda.hpp"
#include "tui/tokens.hpp"

namespace mysong::tui {

// Os GLIFOS, da JetBrainsMono Nerd Font que esta Casa EXIGE, conferidos um a um
// n'ella. Escrevem-se por PONTO DE CODIGO, e não pelo glifo cru: moram no plano
// supplementar de uso privado, onde editor, tubo e terminal os engolem sem dar
// signal, e o que resta é cadeia vazia, falha que passaria calada por toda a
// linha. Medido no FTXUI v7.0.3: o plano quinze não entra na taboa dos glifos
// largos, donde cada um d'estes conta UMA collunha, e a conta da fita presta.
inline constexpr std::string_view kNota = "\U000f075a";
inline constexpr std::string_view kListas = "\U000f0cb8";
inline constexpr std::string_view kBaixa = "\U000f01da";
inline constexpr std::string_view kTocar = "\U000f040a";
inline constexpr std::string_view kPausar = "\U000f03e4";
inline constexpr std::string_view kAnterior = "\U000f04ae";
inline constexpr std::string_view kSeguinte = "\U000f04ad";
inline constexpr std::string_view kEmbaralhar = "\U000f049d";
inline constexpr std::string_view kRepetirTodas = "\U000f0456";
inline constexpr std::string_view kRepetirUma = "\U000f0458";
inline constexpr std::string_view kSom = "\U000f057e";
inline constexpr std::string_view kMudo = "\U000f075f";
inline constexpr std::string_view kAjuda = "\U000f02d7";

// Quantas cellas o rotulo põe ADEANTE da palavra e ATRAZ d'ella: o espaço, o
// glifo e o espaço de um lado, o espaço do outro. Vivem ao pé do
// `rotulo_da_aba`, que é quem as escreve, e a bateria prende as duas contra
// uma linha do cabeçalho pintada em papel.
inline constexpr int kFlancoDoRotulo = 3;
inline constexpr int kCaudaDoRotulo = 1;

Secao secao_da_aba(Aba aba) noexcept {
  switch (aba) {
    case Aba::Playlists: return Secao::Rois;
    case Aba::Download: return Secao::Rede;
    case Aba::MySong: break;
  }
  return Secao::Busca;
}

Aba aba_da_secao(Secao secao) noexcept {
  switch (secao) {
    case Secao::Rois:
    case Secao::NoRol: return Aba::Playlists;
    case Secao::Rede:
    case Secao::Lista: return Aba::Download;
    case Secao::Busca:
    case Secao::Artistas:
    case Secao::Albuns:
    case Secao::Faixas: break;
  }
  return Aba::MySong;
}

Aba aba_seguinte(Aba corrente) noexcept {
  switch (corrente) {
    case Aba::MySong: return Aba::Playlists;
    case Aba::Playlists: return Aba::Download;
    case Aba::Download: break;
  }
  return Aba::MySong;  // o cyclo fecha-se: a fita não tem ponta que prenda
}

Secao vista_seguinte(Secao corrente, bool ha_artista) noexcept {
  switch (corrente) {
    case Secao::Busca: return Secao::Artistas;
    case Secao::Artistas: return ha_artista ? Secao::Albuns : Secao::Busca;
    // Dentro de um album o `o` sobe á vista plana, e não ao degrau de que se
    // veio: esta tecla cycla a VISTA das MY SONG, e não desfaz a navegação,
    // que d'isso já cuidam o Escape e o Backspace.
    case Secao::Albuns:
    case Secao::Faixas:
    case Secao::Rede:
    case Secao::Rois:
    case Secao::NoRol:
    case Secao::Lista: break;
  }
  return Secao::Busca;
}

std::string nome_da_vista(Secao secao) {
  switch (secao) {
    case Secao::Busca: return "FAIXAS";
    case Secao::Artistas: return "ARTISTAS";
    case Secao::Albuns:
    case Secao::Faixas: return "ÁLBUNS";
    // Fóra das MY SONG a vista se não cycla, e chapa que dissesse «FAIXAS»
    // n'uma lista de listas mentiria sobre o que a tecla faz alli.
    case Secao::Rede:
    case Secao::Rois:
    case Secao::NoRol:
    case Secao::Lista: break;
  }
  return {};
}

// ordem_da_aba, a taboada das teclas. Os algarismos e o `o` estavam livres, e
// o Tab vagou com a barra: era elle que a abria. Tecla que não está aqui é
// Alheia, e Alheio NÃO é queda de taboada: é o que faz o atalho de sempre
// continuar a valer sem se repetir n'este arquivo.
OrdemDaAba ordem_da_aba(const ftxui::Event& tecla) noexcept {
  namespace f = ftxui;
  if (tecla == f::Event::Character('1')) return {GestoDaAba::Vai, Aba::MySong};
  if (tecla == f::Event::Character('2'))
    return {GestoDaAba::Vai, Aba::Playlists};
  if (tecla == f::Event::Character('3'))
    return {GestoDaAba::Vai, Aba::Download};
  // O Shift+Tab cycla com o Tab, e não para traz: havendo tres abas, andar ao
  // contrario poupa um toque n'uma d'ellas, e tecla morta é peor que atalho
  // que se repete. É a mesma razão por que elles abriam juntos a barra.
  if (tecla == f::Event::Tab || tecla == f::Event::TabReverse)
    return {GestoDaAba::Cycla, Aba::MySong};
  if (tecla == f::Event::Character('o'))
    return {GestoDaAba::CyclaVista, Aba::MySong};
  return {};
}

namespace {

// vestir, o texto com o par de côres do token, na ALTURA que a fita pedir.
// Côr crua não entra n'esta obra. O fundo cobre as DUAS linhas e o texto fica
// na de CIMA sem que se pinte fileira de espaços: medido no FTXUI v7.0.3, o
// `bgcolor` assenta a côr na caixa INTEIRA antes de descer ao filho, e o `text`
// escreve sómente na fileira do alto d'ella.
ftxui::Element vestir(const std::string& texto, std::string_view tinta,
                      std::string_view fundo, std::size_t altura = 1) {
  const tokens::Triade f = tokens::rgb(tinta);
  const tokens::Triade t = tokens::rgb(fundo);
  return ftxui::text(texto) | ftxui::color(ftxui::Color::RGB(f.r, f.g, f.b)) |
         ftxui::bgcolor(ftxui::Color::RGB(t.r, t.g, t.b)) |
         ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, static_cast<int>(altura));
}

// vestir_todas, o texto repetido em TODAS as fileiras, e não sómente na de
// cima. É o que a junção pede: o `vestir` assenta o fundo na caixa inteira mas
// escreve o glifo n'uma fileira só, e seta pintada sómente em cima deixaria o
// fundo do visinho a entrar em quadrado por baixo d'ella, que é a emenda
// visivel que o tractado da fita proscreve.
ftxui::Element vestir_todas(const std::string& texto, std::string_view tinta,
                            std::string_view fundo, std::size_t altura) {
  std::vector<ftxui::Element> fileiras;
  fileiras.reserve(altura);
  for (std::size_t i = 0; i < altura; ++i)
    fileiras.push_back(vestir(texto, tinta, fundo));
  return ftxui::vbox(std::move(fileiras));
}

// pintar_fita, os pedaços em elementos, com a caixa de CADA segmento pendurada
// pela ORDEM em que a fita o juntou, e não pelo glifo que elle mostra. A irmã
// do letreiro troca a palavra da aba por uma imagem, e caixa achada por texto
// perder-se-hia n'essa troca sem que nada o accusasse.
//
// O `proprio` é a pintura que o segmento traz de si: nulla, veste-se elle pelo
// par de côres que a fita já resolveu. É por esta porta que a aba entra com a
// sua propria pintura sem que a fita deixe de resolver as junções.
ftxui::Element pintar_fita(const std::vector<Pedaco>& pedacos,
                           const std::vector<ftxui::Box*>& caixas,
                           const std::vector<ftxui::Element>& proprios,
                           std::size_t altura = 1) {
  std::vector<ftxui::Element> partes;
  partes.reserve(pedacos.size());
  std::size_t qual = 0;  // o indice do SEGMENTO, que a junção não adianta
  for (const Pedaco& pedaco : pedacos) {
    if (pedaco.juncao) {
      // O REMATE da fita assenta no `panel`, que é o fundo do meio e o da tela
      // por baixo d'ella: a côr transparente do arrowline não se pinta, e
      // pintada á lettra sahiria PRETA entre o botão e a onda.
      partes.push_back(vestir_todas(pedaco.texto, pedaco.tinta,
                                    pedaco.cauda ? tokens::panel : pedaco.fundo,
                                    altura));
      continue;
    }
    // O NEGRITO em todo segmento, e não sómente nas abas: a issue pede os
    // rotulos «em caixa alta e em mono negrito», e o tempo, o volume, o
    // EMBARALHAR e o REPETIR são rotulos como as abas o são. A junção não o
    // leva, que ella é geometria e não palavra.
    ftxui::Element parte =
        qual < proprios.size() && proprios[qual] != nullptr
            ? proprios[qual]
            : vestir(pedaco.texto, pedaco.tinta, pedaco.fundo, altura) |
                  ftxui::bold;
    if (qual < caixas.size() && caixas[qual] != nullptr)
      parte = parte | ftxui::reflect(*caixas[qual]);
    partes.push_back(std::move(parte));
    ++qual;
  }
  return ftxui::hbox(std::move(partes));
}

// As tintas da aba, e o fundo serve tambem á FITA, que d'elle tira a côr das
// junções: lidos em dous logares, a seta sahiria de uma côr e o bloco de outra.
std::string_view fundo_da_aba(EstadoDaAba estado) {
  return pintura_da_aba(estado).fundo;
}
std::string_view tinta_da_aba(EstadoDaAba estado) {
  return pintura_da_aba(estado).tinta;
}

// aceso, o segmento vestido de FOCO (issue #107). As côres sahem do MESMO
// `pintura_da_aba` que veste as abas e as chapas em XIROD: par de côres lido em
// dous logares divergiria na primeira issue que mexesse n'um d'elles. Não sendo
// a peça a que tem o foco, devolve-se intacta.
Segmento aceso(Segmento peca, bool tem_foco) {
  if (!tem_foco) return peca;
  const PinturaDaAba d_elle = pintura_da_aba(EstadoDaAba::ComFoco);
  peca.fundo = d_elle.fundo;
  peca.tinta = d_elle.tinta;
  return peca;
}

}  // namespace

// pintura_da_aba, a corrente é BLOCO SOLIDO, v600 com texto v50, que é o
// gesto do site d'elle onde o que está sob a mão vira bloco cheio; a apagada
// fica no `raised`, que é o degrau de repouso do chrome; e a que tem o FOCO
// accende em glow_core com a tinta do painel, distincta da corrente de
// proposito, que peça focada e peça eleita não são a mesma cousa.
PinturaDaAba pintura_da_aba(EstadoDaAba estado) noexcept {
  switch (estado) {
    case EstadoDaAba::Corrente: return {tokens::vacuo, tokens::launcher_glow};
    case EstadoDaAba::ComFoco: return {tokens::panel, tokens::glow_core};
    case EstadoDaAba::Apagada: break;
  }
  return {tokens::text_primary, tokens::raised};
}

std::string palavra_da_aba(Aba aba, std::size_t quantas) {
  switch (aba) {
    case Aba::Playlists: return "PLAYLISTS";
    case Aba::Download: return "DOWNLOAD";
    case Aba::MySong: break;
  }
  return "MY " + std::to_string(quantas) + " SONG's";
}

std::string rotulo_da_aba(Aba aba, std::size_t quantas) {
  // A guarnição dos flancos entra AQUI, e não na fita: o primitivo recebe o
  // rotulo como se ha de mostrar, e não lh'a accrescenta ás escondidas. E é
  // CONSTANTE de proposito: tres cellas adeante e uma atraz em toda aba, que é
  // o que faz a caixa da palavra sahir da do segmento por subtracção.
  std::string_view glifo = kNota;
  switch (aba) {
    case Aba::Playlists: glifo = kListas; break;
    case Aba::Download: glifo = kBaixa; break;
    case Aba::MySong: break;
  }
  return " " + std::string(glifo) + " " + palavra_da_aba(aba, quantas) + " ";
}

ftxui::Box caixa_da_palavra(const ftxui::Box& segmento) noexcept {
  ftxui::Box palavra = segmento;
  palavra.x_min += kFlancoDoRotulo;
  palavra.x_max -= kCaudaDoRotulo;
  return palavra.x_max >= palavra.x_min ? palavra : caixa_por_pintar();
}

std::string_view identidade_da_chapa(Aba aba) noexcept {
  switch (aba) {
    case Aba::Playlists: return "aba_playlists";
    case Aba::Download: return "aba_download";
    case Aba::MySong: break;
  }
  return "aba_mysong";
}

namespace {

// caixa_do_segmento, a caixa nomeada de cada aba. Por nome e não por indice,
// que peça nova nas caixas deslocaria o indice em silencio.
const ftxui::Box& caixa_do_segmento(const CaixasDoCabecalho& caixas, Aba aba) {
  switch (aba) {
    case Aba::Playlists: return caixas.aba_playlists;
    case Aba::Download: return caixas.aba_download;
    case Aba::MySong: break;
  }
  return caixas.aba_mysong;
}

}  // namespace

std::vector<ChapaDaAba> ordens_das_chapas(const CaixasDoCabecalho& caixas,
                                          Aba corrente, bool letreiro_de_pe,
                                          bool foco_dentro,
                                          const Aba* com_foco) {
  std::vector<ChapaDaAba> ordens;
  ordens.reserve(3);
  for (const Aba aba : {Aba::MySong, Aba::Playlists, Aba::Download}) {
    ChapaDaAba ordem;
    ordem.aba = aba;
    // O FOCO ganha da corrente: elle diz onde o dedo está, e a corrente diz
    // onde se estêve. Aba que seja as duas cousas accende como focada.
    ordem.estado = com_foco != nullptr && *com_foco == aba
                       ? EstadoDaAba::ComFoco
                   : aba == corrente ? EstadoDaAba::Corrente
                                     : EstadoDaAba::Apagada;
    const ftxui::Box palavra = caixa_da_palavra(caixa_do_segmento(caixas, aba));
    // As tres condições são de CONJUNCÇÃO, e nenhuma sobra: sem letreiro não
    // ha chapa, o foco fóra manda tirar, e caixa por pintar não tem canto.
    ordem.poe =
        letreiro_de_pe && foco_dentro && palavra.x_max >= palavra.x_min;
    if (ordem.poe) {
      ordem.collunha = palavra.x_min;
      ordem.linha = palavra.y_min;
      ordem.largura =
          static_cast<std::size_t>(palavra.x_max - palavra.x_min + 1);
      ordem.linhas =
          static_cast<std::size_t>(palavra.y_max - palavra.y_min + 1);
    }
    ordens.push_back(ordem);
  }
  return ordens;
}

nucleo::PedidoDaChapa pedido_da_chapa(const ChapaDaAba& ordem) {
  const PinturaDaAba pintura = pintura_da_aba(ordem.estado);
  nucleo::PedidoDaChapa pedido;
  pedido.texto = palavra_da_aba(ordem.aba);
  pedido.tinta = std::string(pintura.tinta);
  pedido.fundo = std::string(pintura.fundo);
  pedido.cellulas = ordem.largura;
  // As duas medidas da caixa, e não sómente a largura (issue #126): é d'ellas
  // que sahe a proporção da chapa, e é da ALTURA que sahe o corpo em que a
  // palavra se desenha. Pedidas á mão em dous logares, divergiriam.
  pedido.linhas = ordem.linhas;
  pedido.corpo = nucleo::corpo_da_altura(ordem.linhas);
  return pedido;
}

EstadoDaAba estado_da_aba(Aba qual, Aba corrente, Focavel foco) noexcept {
  const Focavel d_ella = qual == Aba::Playlists ? Focavel::AbaPlaylists
                         : qual == Aba::Download ? Focavel::AbaDownload
                                                 : Focavel::AbaMySong;
  if (foco == d_ella) return EstadoDaAba::ComFoco;
  return qual == corrente ? EstadoDaAba::Corrente : EstadoDaAba::Apagada;
}

std::optional<Aba> aba_com_foco(Focavel foco) noexcept {
  switch (foco) {
    case Focavel::AbaMySong: return Aba::MySong;
    case Focavel::AbaPlaylists: return Aba::Playlists;
    case Focavel::AbaDownload: return Aba::Download;
    // As demais peças focaveis não são abas, e o vazio é o que o
    // `ordens_das_chapas` já sabe ler: aba alguma tem o foco.
    case Focavel::Pauta:
    case Focavel::Tocar:
    case Focavel::Anterior:
    case Focavel::Seguinte:
    case Focavel::Volume:
    case Focavel::Embaralhar:
    case Focavel::Repetir:
    case Focavel::Anima:
    case Focavel::Ajuda:
    case Focavel::Trilho:
    case Focavel::Capa: break;
  }
  return std::nullopt;
}

ftxui::Element elemento_da_aba(Aba aba, EstadoDaAba estado,
                               std::size_t altura, std::size_t quantas) {
  return vestir(rotulo_da_aba(aba, quantas), tinta_da_aba(estado), fundo_da_aba(estado),
                altura) |
         ftxui::bold;
}

namespace {

// fita_da_esquerda, as FIXAS, n'uma fita só: as tres abas e, coladas a ellas,
// os tres botões do transporte (tocar, anterior, seguinte). Uma fita e não
// duas, de proposito: a junção entre a ultima aba e o primeiro botão sahe da
// mesma regra que as outras, e fita apartada rematava em preto no meio da
// linha. É a ordem d'elle (issue #134): «my song > playlists > download >
// [play] > [<<] [>>]».
Fita fita_da_esquerda(Aba corrente, Focavel foco, bool tocando,
                      std::size_t quantas) {
  Fita fita(Sentido::Dextra);
  for (const Aba qual : {Aba::MySong, Aba::Playlists, Aba::Download}) {
    const EstadoDaAba estado = estado_da_aba(qual, corrente, foco);
    fita.junta({rotulo_da_aba(qual, quantas), fundo_da_aba(estado),
                tinta_da_aba(estado)});
  }
  // Os botões vestem panel_hi com o glifo em glow_core, que é o glow CONTIDO
  // da regra: elle accende no que TOCA, e não no fundo todo. O do meio TROCA
  // de glifo com o estado, e não de logar: botão que mudasse de sitio faria o
  // dedo errar a pausa que elle proprio pediu.
  fita.junta(aceso({" " + std::string(tocando ? kPausar : kTocar) + " ",
                    tokens::panel_hi, tokens::launcher_glow},
                   foco == Focavel::Tocar));
  fita.junta(aceso({" " + std::string(kAnterior) + " ", tokens::panel_hi,
                    tokens::launcher_glow},
                   foco == Focavel::Anterior));
  fita.junta(aceso({" " + std::string(kSeguinte) + " ", tokens::panel_hi,
                    tokens::launcher_glow},
                   foco == Focavel::Seguinte));
  return fita;
}

// fita_da_direita, o tempo, o volume, os dous modos e o HELP, em setas para a
// ESQUERDA, e sómente as `quantas` primeiras. Quem não cabe sahe INTEIRO, e da
// direita para a esquerda: o HELP cede primeiro (o `?` continua a abri-lo),
// depois o REPETIR, e o tempo por ultimo, que é a ordem do menos util ao mais. Aparar ao meio partiria um par de tinta e
// fundo, que é a emenda visivel que o aceite proscreve.
std::vector<Segmento> segmentos_da_direita(const Retracto& retracto,
                                           Focavel foco, bool animacao_travada,
                                           bool mostrar_animacao) {
  const bool repete = retracto.repeticao != nucleo::Repeticao::Nenhuma;
  const bool calado = retracto.mudo;
  const bool no_zero = retracto.volume == 0;
  const std::string tempo =
      " " + mm_ss(retracto.posicao) + " / " + mm_ss(retracto.duracao) + " ";
  const std::string conta = std::to_string(retracto.volume);
  const std::string som =
      calado ? " " + std::string(kMudo) + " MUDO "
             : " " + std::string(no_zero ? kMudo : kSom) + " " +
                   std::string(3 - std::min<std::size_t>(3, conta.size()), ' ') +
                   conta + "% ";
  const std::string baralha = " " + std::string(kEmbaralhar) + " EMBARALHAR ";
  const std::string torna =
      " " +
      std::string(retracto.repeticao == nucleo::Repeticao::Uma
                      ? kRepetirUma
                      : kRepetirTodas) +
      " REPETIR ";
  const std::string anima = animacao_travada ? " 󰏤 PARADO " : " 󰐊 ANIMA ";
  const Segmento todos[6] = {
      {tempo, tokens::raised, tokens::text_bright},
      aceso({som, tokens::raised,
             calado ? tokens::glow_hot
                    : (no_zero ? tokens::text_muted : tokens::text_primary)},
            foco == Focavel::Volume),
      aceso({baralha, tokens::raised,
             retracto.embaralhado ? tokens::glow_core : tokens::text_muted},
            foco == Focavel::Embaralhar),
      aceso({torna, tokens::raised,
             repete ? tokens::glow_core : tokens::text_muted},
            foco == Focavel::Repetir),
      aceso({anima, tokens::raised,
             animacao_travada ? tokens::glow_hot : tokens::glow_core},
            foco == Focavel::Anima),
      aceso({" " + std::string(kAjuda) + " HELP ", tokens::raised,
             tokens::text_primary},
            foco == Focavel::Ajuda)};
  std::vector<Segmento> fita;
  if (mostrar_animacao) {
    for (std::size_t i = 0; i < 6; ++i) fita.push_back(todos[i]);
  } else {
    const std::size_t indices_sem_animacao[5] = {0, 1, 2, 3, 5};
    for (std::size_t i = 0; i < 5; ++i)
      fita.push_back(todos[indices_sem_animacao[i]]);
  }
  return fita;
}

Fita fita_da_direita(const std::vector<Segmento>& segs, std::size_t quantas) {
  Fita fita(Sentido::Esquerda);
  for (std::size_t i = 0; i < quantas && i < segs.size(); ++i) {
    fita.junta(segs[i]);
  }
  return fita;
}

// caixas_da, os punhos das caixas na ORDEM em que a fita junta os segmentos.
// Punho nullo em toda a lista quer dizer «esta chamada não quer saber».
std::vector<ftxui::Box*> caixas_da_esquerda(CaixasDoCabecalho* c) {
  if (c == nullptr) return {};
  return {&c->aba_mysong,  &c->aba_playlists,  &c->aba_download,
          &c->botao_tocar, &c->botao_anterior, &c->botao_seguinte};
}
std::vector<ftxui::Box*> caixas_da_direita(CaixasDoCabecalho* c,
                                           bool mostrar_animacao) {
  if (c == nullptr) return {};
  if (mostrar_animacao)
    return {&c->tempo, &c->volume, &c->embaralhar, &c->repetir, &c->anima,
            &c->ajuda};
  return {&c->tempo, &c->volume, &c->embaralhar, &c->repetir, &c->ajuda};
}

}  // namespace

ContaDaFita conta_da_fita(std::size_t largura, std::size_t fixas,
                          const std::vector<std::size_t>& direita) {
  ContaDaFita conta;
  if (direita.empty()) return conta;
  // A ponta direita cede do FIM para o principio, que é a ordem do menos util
  // ao mais: o HELP primeiro, e o tempo por ultimo. Cede emquanto ao meio não
  // sobrarem as suas collunhas minimas; com zero segmentos, o meio toma o que
  // as fixas deixarem, pouco ou nada.
  conta.quantas = direita.size() - 1;
  while (conta.quantas > 0 &&
         fixas + direita[conta.quantas] + MEIO_MINIMO > largura)
    --conta.quantas;
  const std::size_t gasto = fixas + direita[conta.quantas];
  conta.meio = largura > gasto ? largura - gasto : 0;
  return conta;
}

namespace {

// elemento_do_meio, a ONDA da faixa (issue #131), ou a barra chata do
// progresso emquanto onda não ha: quem decide é o `elemento_da_onda`, que
// recebe os pontos e a posição, e pinta o andado em v600 (glow_core com o
// foco) e o que falta em line_dim, sobre o fundo `panel` que é o da tela por
// baixo da fita. A caixa é a do clique que busca: a fracção da collunha em
// que o dedo pousa é a posição pedida. Fita alta: a onda toma a fileira de
// cima, e o fundo enche as demais.
ftxui::Element elemento_do_meio(const Retracto& retracto,
                                const std::vector<float>& onda,
                                std::size_t largura, bool com_foco,
                                ftxui::Box* caixa, std::size_t altura) {
  if (largura == 0) return ftxui::emptyElement();
  ftxui::Element meio = elemento_da_onda(onda, retracto.posicao,
                                         retracto.duracao, largura, com_foco,
                                         caixa);
  if (altura <= 1) return meio;
  const tokens::Triade cama = tokens::rgb(tokens::panel);
  return ftxui::vbox({std::move(meio), ftxui::filler()}) |
         ftxui::bgcolor(ftxui::Color::RGB(cama.r, cama.g, cama.b)) |
         ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, static_cast<int>(altura));
}

}  // namespace

ftxui::Element elemento_do_cabecalho(const Retracto& retracto, Aba corrente,
                                     const std::vector<float>& onda,
                                     std::size_t largura,
                                     CaixasDoCabecalho* caixas, Focavel foco,
                                     std::size_t altura, bool animacao_travada,
                                     bool mostrar_animacao) {
  // Esvaziam-se á entrada, e antes de toda sahida antecipada: linha que se não
  // pintou não ha de deixar caixa do quadro anterior a apanhar cliques.
  if (caixas != nullptr) *caixas = CaixasDoCabecalho();
  if (largura == 0 || altura == 0) return ftxui::text("");
  const Fita esquerda = fita_da_esquerda(
      corrente, foco, retracto.estado == nucleo::Estado::Tocando,
      retracto.tamanho);
  const std::vector<Segmento> segs_direita = segmentos_da_direita(
      retracto, foco, animacao_travada, mostrar_animacao);
  std::vector<std::size_t> pede(segs_direita.size() + 1, 0);
  for (std::size_t q = 1; q < pede.size(); ++q) {
    pede[q] = fita_da_direita(segs_direita, q).largura_exigida();
  }
  const ContaDaFita conta =
      conta_da_fita(largura, esquerda.largura_exigida(), pede);
  const Fita direita = fita_da_direita(segs_direita, conta.quantas);
  return ftxui::hbox(
      {pintar_fita(
           esquerda.compor(), caixas_da_esquerda(caixas),
           {elemento_da_aba(Aba::MySong,
                            estado_da_aba(Aba::MySong, corrente, foco), altura,
                            retracto.tamanho),
            elemento_da_aba(Aba::Playlists,
                            estado_da_aba(Aba::Playlists, corrente, foco),
                            altura),
            elemento_da_aba(Aba::Download,
                            estado_da_aba(Aba::Download, corrente, foco),
                            altura)},
           altura),
       elemento_do_meio(retracto, onda, conta.meio, foco == Focavel::Trilho,
                        caixas != nullptr ? &caixas->trilho : nullptr, altura),
       pintar_fita(direita.compor(), caixas_da_direita(caixas, mostrar_animacao),
                   {}, altura)});
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
