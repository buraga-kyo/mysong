// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COMMANDO — src/tui/commando.hpp
// ══════════════════════════════════════════════════════════════════════════
// A tradução de TECLA em ORDEM. Existe á parte da janella porque a janella não
// se prova: ella abre terminal, abre motor e abre som. Esta peça é funcção pura
// de tecla e retracto para uma ordem, e a bateria afere a taboada inteira sem
// erguer cousa alguma.
//
// DOMÍNIO ......... a tecla que o terminal entrega, e o Retracto do instante.
// CONTRA-DOMÍNIO .. uma Ordem: o verbo e, quando o verbo o pede, o alvo já
//                   APARADO nas bordas.
// INVARIANTE ...... tecla que não é do mockup dá Ordem::Nada, e Ordem::Nada não
//                   chama cousa alguma no tocador. A aparadura é feita AQUI, e
//                   não no tocador: o tocador tambem apara, e ter as duas dá
//                   duas verdades; esta é a que a tela mostra.
// Q.E.D. .......... sendo a taboada uma funcção, a prova conta as chamadas por
//                   dublê e não por terminal, e a issue seguinte que acrescente
//                   tecla acrescenta uma linha da taboada e um caso.
// ══════════════════════════════════════════════════════════════════════════
#ifndef MYSONG_TUI_COMMANDO_HPP
#define MYSONG_TUI_COMMANDO_HPP

#include <ftxui/component/event.hpp>

#include "tui/transporte.hpp"

namespace mysong::tui {

// O PASSO DA BUSCA, em segundos. Cinco, que é o passo que o mpv usa nas suas
// proprias setas: quem vem do mpv não reaprende o dedo.
inline constexpr double PASSO_DA_BUSCA = 5.0;

// O DEGRAU DO VOLUME, em pontos percentuaes.
inline constexpr int DEGRAU_DO_VOLUME = 5;

enum class Verbo {
  Nada, Pausar, Retomar, Proxima, Anterior, Buscar, Volume, Sahir,
  // Os da navegação (issue #9). Entram no MESMO enum, e não n'outro: a tecla é
  // uma, e ter duas taboadas a olhar a mesma tecla faria uma delas ganhar por
  // ordem de chamada, que é decisão que ninguem escreveu.
  Desce, Sobe, AoPrincipio, AoFim, Entra, Volta, AbreBusca, Varre,
  // O da aquisição (issue #11).
  AbreBaixa,
  // A letra (issue #15): troca o painel do espectro pelo da letra.
  TrocaLetra,
  // A busca na REDE (issue #12). Verbo proprio, e não o AbreBusca com bandeira:
  // duas cousas differentes acontecem, que uma filtra o que ha e a outra pergunta
  // ao mundo, e verbo com bandeira é verbo que se lê errado n'um switch.
  AbreProcura,
  // As LISTAS (issue #10). Oito verbos, e não um com alvo: cada tecla faz uma cousa
  // differente, e verbo com alvo obrigaria o switch a olhar duas cousas para saber
  // qual d'ellas se pediu.
  AbreRois, CriaRol, RenomeiaRol, ApagaRol, JuntaAoRol, RetiraDoRol,
  SobeNoRol, DesceNoRol,
  // O VÍDEO (issue #17): abre a faixa eleita em janella propria do systema.
  AbreVideo,
  // O CATALOGO DO SPOTIFY (issue #13). Dous: ler a lista, e baixá-la toda. Baixar a
  // eleita é o Entra, que na secção do catalogo quer dizer baixar.
  AbreCatalogo, BaixaTudo,
  // A FONTE DA BUSCA (issue #56): cicla YouTube, YouTube Music, Spotify. Verbo
  // puro, como o BaixaTudo: quem guarda a fonte e a secção é a janella.
  TrocaFonte,
  // OS DOUS MODOS de reprodução (issue #62). Verbos puros, como o TrocaFonte:
  // quem alterna e quem cicla é o TOCADOR, de uma tomada só da sua tranca.
  Embaralhar, Repetir,
  // AS DUAS DA FAIXA (issue #105): renomear a musica, e mandá-la á lixeira.
  // Teclas de FUNCÇÃO, e não lettras: as duas estragam cousa gravada, e o F2 e
  // o Delete são o que o gerenciador de arquivos d'elle já faz, donde a mão não
  // reaprende. Quem pergunta ao operador é a tela; estes verbos sómente dizem
  // que se pediu.
  RenomeiaFaixa, ApagaFaixa,
};

// Uma ORDEM. `alvo` sómente presta para Buscar (segundos) e Volume (por cento),
// e nos outros verbos vale zero de proposito: ordem que não tem alvo não deve
// carregar numero que alguem possa vir a ler.
struct Ordem {
  Verbo verbo = Verbo::Nada;
  double alvo = 0.0;
  // RELATIVO diz que o alvo é um DESLOCAMENTO, e não uma posição. Existe por causa
  // da janella do video: d'ella não se sabe a posição sem lhe perguntar pelo
  // soquete e esperar resposta, e o que a tecla quer dizer é «cinco segundos
  // adeante», que não pede posição alguma. No motor de audio continua absoluto,
  // que d'esse a posição se lê de graça.
  bool relativo = false;
};

// ordem_da_tecla — a taboada. Não toca no tocador: devolve o que se HA DE fazer.
//
// `digitando` diz que a barra de busca está aberta. Estando-o, TECLA ALGUMA da
// taboada vale: o `n` que trocava de faixa passa a ser a letra `n` do termo. Sem
// esta guarda, buscar por «nova» trocaria de faixa duas vezes e abriria a busca
// n'outro logar, que é o defeito classico das TUI que esquecem o modo.
Ordem ordem_da_tecla(const ftxui::Event& tecla, const Retracto& retracto,
                     bool digitando = false);

}  // namespace mysong::tui

#endif  // MYSONG_TUI_COMMANDO_HPP

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
