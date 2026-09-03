// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MENU DE CONTEXTO — src/tui/menu_contexto.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação do estado, da taboada e da pintura. Vale aqui a regra do
// rato: cousa alguma d'este arquivo sabe o que é um Navegador ou um Tocador.
//
// DOMÍNIO ......... o estado do menu, a tecla, e a geometria da tela.
// CONTRA-DOMÍNIO .. o estado mudado, o Pedido, e elementos do FTXUI.
// INVARIANTE ...... funcção alguma d'aqui lança, nem toca em estado que viva
//                   fóra dos seus parametros.
// Q.E.D. .......... sendo tudo funcção de valores, a bateria arma o menu á mão
//                   e afere a tecla contra o pedido escripto.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/menu_contexto.hpp"

#include <utility>

#include <ftxui/component/mouse.hpp>

namespace mysong::tui {

namespace {

// anda — o eleito uma casa, EM RODA. A volta ao principio é o que o menu do
// tmux d'elle faz, e é o que poupa á mão sete setas para tornar ao alto de uma
// lista de listas comprida. Zero itens fica em zero, que não ha onde andar.
std::size_t anda(std::size_t onde, std::size_t quantos, bool desce) noexcept {
  if (quantos == 0) return 0;
  if (desce) return (onde + 1) % quantos;
  return onde == 0 ? quantos - 1 : onde - 1;
}

}  // namespace

void abre_o_menu(MenuDeContexto& menu, std::size_t faixa, std::string titulo,
                 std::vector<nucleo::Rol> listas) {
  menu.aberto = true;
  menu.faixa = faixa;
  menu.titulo = std::move(titulo);
  menu.item = 0;
  menu.submenu = false;
  menu.lista = 0;
  menu.listas = std::move(listas);
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
