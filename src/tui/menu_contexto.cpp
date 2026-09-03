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

#include <algorithm>
#include <string_view>
#include <utility>

#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/string.hpp>

#include "tui/tokens.hpp"

namespace mysong::tui {

namespace {

// Os rotulos, na ordem do enum. CAIXA ALTA, que é a regra dos rotulos d'esta
// Casa; o «▸» do JUNTAR pinta-se á parte, encostado á orla da direita.
constexpr std::string_view kRotulos[QUANTOS_ITENS] = {
    "TOCAR", "JUNTAR À LISTA", "NOVA LISTA COM ESTA", "RENOMEAR", "APAGAR"};

// O item ANTES do qual corre o filete. Elle aparta o que ESTRAGA cousa gravada
// do que a não estraga: acima toca-se e junta-se, abaixo renomeia-se e apaga-se.
constexpr std::size_t kFileteAntesDe = 3;

// Os limites do nome de lista no submenu, em collunhas. O minimo é o que o
// rotulo «LISTAS» pede na orla de cima; o maximo é o que impede que uma lista
// de nome comprido faça o menu sahir da metade da pauta.
constexpr std::size_t kNomeMinimo = 7, kNomeMaximo = 24;

// collunhas — as que a cadeia toma na tela. Pergunta-se ao FTXUI, e não se
// contam bytes nem codepoints: o acento do portuguez toma dous bytes e uma
// collunha só, e a orla da direita sahiria torta em toda faixa acentuada.
std::size_t collunhas(std::string_view crua) {
  return static_cast<std::size_t>(ftxui::string_width(std::string(crua)));
}

std::size_t campo_dos_itens() {
  std::size_t maior = 0;
  for (const std::string_view rotulo : kRotulos)
    maior = std::max(maior, collunhas(rotulo));
  return maior;
}

std::size_t campo_das_listas(const MenuDeContexto& menu) {
  std::size_t maior = kNomeMinimo;
  for (const nucleo::Rol& rol : menu.listas)
    maior = std::max(maior, collunhas(rol.nome));
  return std::min(maior, kNomeMaximo);
}

// anda — o eleito uma casa, EM RODA. A volta ao principio é o que o menu do
// tmux d'elle faz, e é o que poupa á mão sete setas para tornar ao alto de uma
// lista de listas comprida. Zero itens fica em zero, que não ha onde andar.
std::size_t anda(std::size_t onde, std::size_t quantos, bool desce) noexcept {
  if (quantos == 0) return 0;
  if (desce) return (onde + 1) % quantos;
  return onde == 0 ? quantos - 1 : onde - 1;
}

// pinta — o texto na tinta e o fundo por baixo d'elle. TODA cella do menu se
// pinta: elle flutua, e cella por pintar deixaria ver a pauta por dentro.
ftxui::Element pinta(const std::string& texto, std::string_view tinta,
                     std::string_view fundo) {
  const tokens::Triade t = tokens::rgb(tinta), f = tokens::rgb(fundo);
  return ftxui::text(texto) | ftxui::color(ftxui::Color::RGB(t.r, t.g, t.b)) |
         ftxui::bgcolor(ftxui::Color::RGB(f.r, f.g, f.b));
}

// repete — o glifo n vezes. `std::string(n, c)` não serve: o «─» tem tres bytes.
std::string repete(std::string_view glifo, std::size_t quantos) {
  std::string feita;
  for (std::size_t i = 0; i < quantos; ++i) feita += glifo;
  return feita;
}

// apara — a cadeia cortada em `largura` COLLUNHAS e enchida de espaços até
// ellas: o enchimento é que põe a orla da direita sempre na mesma collunha.
std::string apara(std::string_view crua, std::size_t largura) {
  std::string feita;
  std::size_t tem = 0;
  for (std::size_t i = 0; i < crua.size();) {
    const unsigned char byte = static_cast<unsigned char>(crua[i]);
    const std::size_t quantos =
        byte >= 0xF0 ? 4 : byte >= 0xE0 ? 3 : byte >= 0xC0 ? 2 : 1;
    const std::string_view glifo = crua.substr(i, quantos);
    if (tem + collunhas(glifo) > largura) break;
    feita += glifo;
    tem += collunhas(glifo);
    i += quantos;
  }
  while (tem++ < largura) feita += ' ';
  return feita;
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
    // A guarda do submenu vem ANTES da do item, e não depois: com o submenu de
    // pé o item eleito CONTINUA a ser o JUNTAR, e olhando primeiro o item o
    // Enter tornava a abrir o que já estava aberto. A lista escolhida nunca
    // chegava a sahir, e o menu ficava a comer Enters sem dizer porque.
    if (!menu.submenu && no_juntar) {
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

MedidaDoMenu medida_do_menu(const MenuDeContexto& menu) {
  // A linha do item é « ROTULO ▸ », e a orla põe uma collunha de cada lado; a
  // altura são as cinco linhas, o filete, e as duas da orla.
  MedidaDoMenu medida{campo_dos_itens() + 6, QUANTOS_ITENS + 3};
  if (!menu.submenu) return medida;
  medida.largura += campo_das_listas(menu) + 4;
  medida.altura = std::max(medida.altura, menu.listas.size() + 2);
  return medida;
}

ftxui::Element elemento_do_menu(const MenuDeContexto& menu) {
  const std::size_t largura = medida_do_menu(menu).largura;
  std::vector<ftxui::Element> linhas;
  linhas.push_back(pinta("┌" + repete("─", largura - 2) + "┐",
                         tokens::line_base, tokens::panel));
  for (const std::string_view rotulo : kRotulos)
    linhas.push_back(pinta("│ " + apara(rotulo, largura - 4) + " │",
                           tokens::text_primary, tokens::panel));
  linhas.push_back(pinta("└" + repete("─", largura - 2) + "┘",
                         tokens::line_base, tokens::panel));
  return ftxui::vbox(std::move(linhas));
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
