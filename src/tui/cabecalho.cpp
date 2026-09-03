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
    // Uma collunha se guarda para o «…», e sómente quando ha corte de facto.
    if (gastas + vale > largura - 1) return feito + "…";
    feito += letra;
    gastas += vale;
    i = fim;
  }
  return feito + std::string(largura - gastas, ' ');
}

}  // namespace

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
