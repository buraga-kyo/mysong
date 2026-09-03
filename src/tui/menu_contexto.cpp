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
  // A seta direita e o Enter abrem o submenu no JUNTAR, e é o MESMO gesto: o
  // item leva «▸», e quem lê a seta na tela carrega n'ella ou no Enter sem
  // pensar. Sem lista alguma elle não abre: caixa vazia não é resposta, e a
  // sahida está na linha de baixo, que cria uma lista já com esta faixa.
  const bool no_juntar =
      menu.item == static_cast<std::size_t>(ItemDoMenu::JuntaALista);
  const bool ha_listas = quantas > 0;
  if (tecla == ftxui::Event::ArrowRight || tecla == ftxui::Event::Return) {
    if (no_juntar) {
      if (ha_listas) {
        menu.submenu = true;
        menu.lista = 0;
      }
      return {};
    }
    if (tecla == ftxui::Event::ArrowRight) return {};  // a direita só abre
    // ESCOLHEU-SE: o menu fecha ANTES de a janella cumprir. Ficando aberto por
    // cima da pergunta do apagar, o operador respondia «s» ao menu e não á
    // pergunta, que é o defeito de quem fecha depois do desfecho.
    if (menu.submenu) {
      const int qual = menu.lista < quantas ? menu.listas[menu.lista].id : 0;
      menu.aberto = false;
      return {PedidoDoMenu::Junta, qual};
    }
    menu.aberto = false;
    switch (static_cast<ItemDoMenu>(menu.item)) {
      case ItemDoMenu::Toca: return {PedidoDoMenu::Toca, 0};
      case ItemDoMenu::NovaLista: return {PedidoDoMenu::NovaLista, 0};
      case ItemDoMenu::Renomeia: return {PedidoDoMenu::Renomeia, 0};
      case ItemDoMenu::Apaga: return {PedidoDoMenu::Apaga, 0};
      case ItemDoMenu::JuntaALista: break;  // tratado acima, e não cae aqui
    }
    return {};
  }
  return {};  // tecla que a taboada não conhece consome-se, e nada mais
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
