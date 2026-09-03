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

RespostaDoMenu tecla_no_menu(MenuDeContexto& menu, const ftxui::Event& tecla) {
  if (!menu.aberto) return {};
  // O RATO: o botão a DESCER fecha, e o a subir NÃO. O modo 1000 manda sempre o
  // soltar, e sem esta guarda o menu fechava-se no mesmo clique que o abriu.
  if (tecla.is_mouse()) {
    ftxui::Event copia = tecla;  // o punho do rato é não const no FTXUI
    if (copia.mouse().motion == ftxui::Mouse::Pressed) menu.aberto = false;
    return {};
  }
  // O ESCAPE fecha TUDO, e não sómente o submenu: é o que a issue diz, e é o
  // que a mão espera de quem quer sahir de uma vez. Quem quer sahir SÓMENTE do
  // submenu tem a seta esquerda, que é a que o abriu ao contrario. E o `m`
  // fecha tambem: a tecla que abre o menu ha de o fechar, senão ella dá duas
  // cousas differentes conforme o estado, que é o que a mão não adivinha.
  if (tecla == ftxui::Event::Escape || tecla == ftxui::Event::Character('m')) {
    menu.aberto = false;
    return {};
  }
  const std::size_t quantas = menu.listas.size();
  if (tecla == ftxui::Event::ArrowDown || tecla == ftxui::Event::ArrowUp) {
    const bool desce = tecla == ftxui::Event::ArrowDown;
    if (menu.submenu) menu.lista = anda(menu.lista, quantas, desce);
    else menu.item = anda(menu.item, QUANTOS_ITENS, desce);
    return {};
  }
  if (tecla == ftxui::Event::ArrowLeft) {
    menu.submenu = false;  // e o menu FICA: quem o fecha é o Escape
    return {};
  }
  return {};  // tecla que a taboada não conhece consome-se, e nada mais
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
