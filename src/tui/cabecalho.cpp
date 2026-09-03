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

namespace mysong::tui {

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

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
