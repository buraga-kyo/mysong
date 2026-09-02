// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MENU — src/tui/menu.cpp
// ══════════════════════════════════════════════════════════════════════════
// As taboadas e a machina. O contracto e a ordem do despacho estão no .hpp.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/menu.hpp"

namespace mysong::tui {

// A taboada degrau→secção, na ordem do mockup: é a MESMA ordem da pintura da
// tabella, e mudar uma sem a outra faria o Enter abrir secção que o marcador
// não diz. Degrau fóra da conta cahe no primeiro, e não estoura.
Secao secao_do_degrau(std::size_t degrau) noexcept {
  constexpr Secao degraus[DEGRAUS_DA_BARRA] = {
      Secao::Artistas, Secao::Albuns, Secao::Faixas, Secao::Busca,
      Secao::Rede,     Secao::Rois,   Secao::Lista};
  return degrau < DEGRAUS_DA_BARRA ? degraus[degrau] : Secao::Artistas;
}

// O caminho de volta. O switch não tem `default`, de proposito: secção nova
// que se accrescente ao enum deixa de compilar aqui, em vez de abrir a barra
// com o degrau eleito n'um logar qualquer.
std::size_t degrau_da_secao(Secao secao) noexcept {
  switch (secao) {
    case Secao::Artistas: return 0;
    case Secao::Albuns: return 1;
    case Secao::Faixas: return 2;
    case Secao::Busca: return 3;
    case Secao::Rede: return 4;
    case Secao::Rois: return 5;
    // Dentro de uma lista o degrau é o das LISTAS, como na pintura de hoje.
    case Secao::NoRol: return 5;
    case Secao::Lista: return 6;
  }
  return 0;
}

// A CONTA dos degraus da bibliotheca (issue #93): um para as MINHAS MÚSICAS, um
// por lista do operador, e os quatro de navegar. Barra que não conte as listas
// d'elle não é bibliotheca, é um menu.
std::size_t degraus_da_barra(std::size_t listas) noexcept {
  return 1 + listas + 4;
}

AlvoDaBarra alvo_do_degrau(std::size_t degrau,
                           const std::vector<nucleo::Rol>& listas) {
  if (degrau == 0 || degrau >= degraus_da_barra(listas.size()))
    return {Secao::Busca, 0};
  if (degrau <= listas.size()) return {Secao::NoRol, listas[degrau - 1].id};
  constexpr Secao navegar[4] = {Secao::Artistas, Secao::Albuns, Secao::Rede,
                                Secao::Lista};
  return {navegar[degrau - listas.size() - 1], 0};
}

bool tecla_abre_menu(const ftxui::Event& tecla) noexcept {
  return tecla == ftxui::Event::Tab || tecla == ftxui::Event::TabReverse;
}

// A taboada da barra aberta. O Tab fecha porque alternar é o contracto d'elle;
// a seta esquerda, o Escape e o Backspace fecham porque na lista elles VOLTAM,
// e da barra não ha para onde voltar senão á lista. Tecla que não está aqui é
// Alheio, e o Alheio NÃO é queda de taboada: é a regra que faz o atalho valer.
GestoDaBarra gesto_da_barra(const ftxui::Event& tecla) noexcept {
  namespace f = ftxui;
  if (tecla_abre_menu(tecla) || tecla == f::Event::Escape ||
      tecla == f::Event::Backspace || tecla == f::Event::ArrowLeft)
    return GestoDaBarra::Fecha;
  if (tecla == f::Event::ArrowDown || tecla == f::Event::Character('j'))
    return GestoDaBarra::Desce;
  if (tecla == f::Event::ArrowUp || tecla == f::Event::Character('k'))
    return GestoDaBarra::Sobe;
  if (tecla == f::Event::Home || tecla == f::Event::Character('g'))
    return GestoDaBarra::AoPrincipio;
  if (tecla == f::Event::End || tecla == f::Event::Character('G'))
    return GestoDaBarra::AoFim;
  if (tecla == f::Event::Return || tecla == f::Event::ArrowRight)
    return GestoDaBarra::Entra;
  return GestoDaBarra::Alheio;
}

bool Menu::aberto() const noexcept { return aberto_; }
std::size_t Menu::degrau() const noexcept { return degrau_; }
Secao Menu::alvo() const noexcept { return secao_do_degrau(degrau_); }

void Menu::abre(Secao corrente) noexcept {
  aberto_ = true;
  degrau_ = degrau_da_secao(corrente);
}

void Menu::fecha() noexcept { aberto_ = false; }

void Menu::sobe() noexcept {
  if (degrau_ > 0) --degrau_;
}

void Menu::desce() noexcept {
  if (degrau_ + 1 < DEGRAUS_DA_BARRA) ++degrau_;
}

void Menu::ao_principio() noexcept { degrau_ = 0; }

void Menu::ao_fim() noexcept { degrau_ = DEGRAUS_DA_BARRA - 1; }

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
