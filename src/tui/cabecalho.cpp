// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CABEÇALHO — src/tui/cabecalho.cpp
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

#include <string_view>
#include <utility>
#include <vector>

#include <ftxui/screen/string.hpp>

#include "tui/arrowline.hpp"
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
// O trilho: o traço PESADO, que é o que o esboço mostra. Traço leve some no
// fundo violaceo a esta opacidade, e trilho que se não vê não diz onde a
// faixa vae.
inline constexpr std::string_view kTraco = "\u2501";

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

// ordem_da_aba — a taboada das teclas. Os algarismos e o `o` estavam livres, e
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

// vestir — o texto com o par de côres do token. Côr crua não entra n'esta obra.
ftxui::Element vestir(const std::string& texto, std::string_view tinta,
                      std::string_view fundo) {
  const tokens::Triade f = tokens::rgb(tinta);
  const tokens::Triade t = tokens::rgb(fundo);
  return ftxui::text(texto) | ftxui::color(ftxui::Color::RGB(f.r, f.g, f.b)) |
         ftxui::bgcolor(ftxui::Color::RGB(t.r, t.g, t.b));
}

// aparar_nome — o nome do que sôa, na largura que sobrou. Mede-se em COLLUNHAS
// pelo `string_width`, e não em pontos de codigo como a fita: o nome vem do
// acervo d'elle, e ha titulo com kanji e com emoji, que valem duas. Cabendo,
// enche-se de espaços: o fundo do segmento veste a collunha inteira, e nome
// curto deixaria buraco no meio da linha. Não cabendo, corta-se com «…».
std::string aparar_nome(const std::string& nome, std::size_t largura) {
  if (largura == 0) return {};
  const std::size_t inteiro =
      static_cast<std::size_t>(ftxui::string_width(nome));
  // O «…» só entra HAVENDO corte: a collunha d'elle guarda-se depois de se
  // saber que ha corte, e não antes. Guardada sempre, o nome que cabia
  // exactamente sahia cortado na ultima lettra, e a prova em papel accusa-o.
  if (inteiro <= largura) return nome + std::string(largura - inteiro, ' ');
  std::string feito;
  std::size_t gastas = 0;
  for (std::size_t i = 0; i < nome.size();) {
    std::size_t fim = i + 1;
    while (fim < nome.size() &&
           (static_cast<unsigned char>(nome[fim]) & 0xC0) == 0x80)
      ++fim;
    const std::string letra = nome.substr(i, fim - i);
    const std::size_t vale =
        static_cast<std::size_t>(ftxui::string_width(letra));
    if (gastas + vale > largura - 1) break;
    feito += letra;
    gastas += vale;
    i = fim;
  }
  // O que sobrar depois do «…» enche-se: o kanji de duas collunhas pode deixar
  // uma por gastar, e essa collunha sem fundo seria buraco na fita.
  return feito + "…" + std::string(largura - gastas - 1, ' ');
}

// pintar_fita — os pedaços em elementos, com a caixa de CADA segmento pendurada
// pela ORDEM em que a fita o juntou, e não pelo glifo que elle mostra. A irmã
// do letreiro troca a palavra da aba por uma imagem, e caixa achada por texto
// perder-se-hia n'essa troca sem que nada o accusasse.
//
// O `proprio` é a pintura que o segmento traz de si: nulla, veste-se elle pelo
// par de côres que a fita já resolveu. É por esta porta que a aba entra com a
// sua propria pintura sem que a fita deixe de resolver as junções.
ftxui::Element pintar_fita(const std::vector<Pedaco>& pedacos,
                           const std::vector<ftxui::Box*>& caixas,
                           const std::vector<ftxui::Element>& proprios) {
  std::vector<ftxui::Element> partes;
  partes.reserve(pedacos.size());
  std::size_t qual = 0;  // o indice do SEGMENTO, que a junção não adianta
  for (const Pedaco& pedaco : pedacos) {
    if (pedaco.juncao) {
      partes.push_back(vestir(pedaco.texto, pedaco.tinta, pedaco.fundo));
      continue;
    }
    ftxui::Element parte =
        qual < proprios.size() && proprios[qual] != nullptr
            ? proprios[qual]
            : vestir(pedaco.texto, pedaco.tinta, pedaco.fundo);
    if (qual < caixas.size() && caixas[qual] != nullptr)
      parte = parte | ftxui::reflect(*caixas[qual]);
    partes.push_back(std::move(parte));
    ++qual;
  }
  return ftxui::hbox(std::move(partes));
}

// As tintas da aba, n'um logar só: a corrente é BLOCO SOLIDO, v600 com texto
// v50, que é o gesto do site d'elle onde o que está sob a mão vira bloco cheio;
// as outras ficam no `raised`, que é o degrau de repouso do chrome. O fundo
// serve tambem á FITA, que d'elle tira a côr das junções: lidos em dous
// logares, a seta sahiria de uma côr e o bloco de outra.
std::string_view fundo_da_aba(bool corrente) {
  return corrente ? tokens::v600 : tokens::raised;
}
std::string_view tinta_da_aba(bool corrente) {
  return corrente ? tokens::v50 : tokens::text_primary;
}

}  // namespace

std::string rotulo_da_aba(Aba aba) {
  // A guarnição dos flancos entra AQUI, e não na fita: o primitivo recebe o
  // rotulo como se ha de mostrar, e não lh'a accrescenta ás escondidas.
  switch (aba) {
    case Aba::Playlists: return " " + std::string(kListas) + " PLAYLISTS ";
    case Aba::Download: return " " + std::string(kBaixa) + " DOWNLOAD ";
    case Aba::MySong: break;
  }
  return " " + std::string(kNota) + " MY SONG ";
}

ftxui::Element elemento_da_aba(Aba aba, bool corrente) {
  return vestir(rotulo_da_aba(aba), tinta_da_aba(corrente),
                fundo_da_aba(corrente)) |
         ftxui::bold;
}

namespace {

// fita_da_esquerda — as tres abas e os tres botões, na ordem d'elle: tocar,
// anterior, seguinte. Os botões vestem panel_hi com o glifo em glow_core, que
// é o glow CONTIDO da regra: elle accende no que TOCA, e não no fundo todo.
Fita fita_da_esquerda(Aba corrente, bool tocando) {
  Fita fita(Sentido::Dextra);
  for (const Aba qual : {Aba::MySong, Aba::Playlists, Aba::Download})
    fita.junta({rotulo_da_aba(qual), fundo_da_aba(qual == corrente),
                tinta_da_aba(qual == corrente)});
  // O botão do meio TROCA de glifo com o estado, e não de logar: botão que
  // mudasse de sitio faria o dedo errar a pausa que elle proprio pediu.
  fita.junta({" " + std::string(tocando ? kPausar : kTocar) + " ",
              tokens::panel_hi, tokens::glow_core});
  fita.junta({" " + std::string(kAnterior) + " ", tokens::panel_hi,
              tokens::glow_core});
  fita.junta({" " + std::string(kSeguinte) + " ", tokens::panel_hi,
              tokens::glow_core});
  return fita;
}

// fita_da_direita — o tempo, o volume e os dous modos, em setas para a
// ESQUERDA, e sómente as `quantas` primeiras. Quem não cabe sahe INTEIRO, e da
// direita para a esquerda: o REPETIR cede primeiro, e o tempo por ultimo, que
// é a ordem do menos util ao mais. Aparar ao meio partiria um par de tinta e
// fundo, que é a emenda visivel que o aceite proscreve.
Fita fita_da_direita(const Retracto& retracto, std::size_t quantas) {
  const bool repete = retracto.repeticao != nucleo::Repeticao::Nenhuma;
  const bool mudo = retracto.volume == 0;
  const std::string tempo =
      " " + mm_ss(retracto.posicao) + " / " + mm_ss(retracto.duracao) + " ";
  const std::string som = " " + std::string(mudo ? kMudo : kSom) + " " +
                          std::to_string(retracto.volume) + "% ";
  const std::string baralha = " " + std::string(kEmbaralhar) + " EMBARALHAR ";
  const std::string torna =
      " " +
      std::string(retracto.repeticao == nucleo::Repeticao::Uma
                      ? kRepetirUma
                      : kRepetirTodas) +
      " REPETIR ";
  // Aceso é glow_core, apagado é text_muted, e o segmento fica PRESENTE nos
  // dous casos: modo que sommisse mudaria a largura da linha a cada tecla, e o
  // nome da faixa saltaria de logar debaixo do olho.
  const Segmento todos[4] = {
      {tempo, tokens::raised, tokens::text_bright},
      {som, tokens::raised, mudo ? tokens::text_muted : tokens::text_primary},
      {baralha, tokens::raised,
       retracto.embaralhado ? tokens::glow_core : tokens::text_muted},
      {torna, tokens::raised, repete ? tokens::glow_core : tokens::text_muted}};
  Fita fita(Sentido::Esquerda);
  for (std::size_t i = 0; i < quantas && i < 4; ++i) fita.junta(todos[i]);
  return fita;
}

// O que se guarda ao nome quando a tela aperta: seis collunhas e um «…». Menos
// que isso não é nome, é ruido, e ahi as peças da direita hão de ceder antes.
constexpr std::size_t kNomeMinimo = 7;

// caixas_da — os punhos das caixas na ORDEM em que a fita junta os segmentos.
// Punho nullo em toda a lista quer dizer «esta chamada não quer saber».
std::vector<ftxui::Box*> caixas_da_esquerda(CaixasDoCabecalho* c) {
  if (c == nullptr) return {};
  return {&c->aba_mysong,   &c->aba_playlists,  &c->aba_download,
          &c->botao_tocar,  &c->botao_anterior, &c->botao_seguinte};
}
std::vector<ftxui::Box*> caixas_da_direita(CaixasDoCabecalho* c) {
  if (c == nullptr) return {};
  return {&c->tempo, &c->volume, &c->embaralhar, &c->repetir};
}

}  // namespace

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
