// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROMPT — src/tui/prompt.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada dos modos, e a pintura do topo. O contracto está em prompt.hpp.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/prompt.hpp"

#include <utility>

#include "tui/tokens.hpp"

namespace mysong::tui {

namespace {

ftxui::Element pinta(const std::string& texto, std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::text(texto) | ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

// rabo — o FIM da cadeia, em `largura` collunhas, contando CODEPOINTS: é o
// par do `apara` da tabella, que guarda o começo. Aqui guarda-se o fim, que é
// o que o operador acabou de teclar; e contando bytes, termo acentuado sahiria
// cortado ao meio de um codepoint.
std::string rabo(const std::string& crua, std::size_t largura) {
  std::size_t contadas = 0;
  for (std::size_t i = crua.size(); i > 0;) {
    --i;
    if ((static_cast<unsigned char>(crua[i]) & 0xC0) != 0x80) {
      if (contadas == largura) return crua.substr(i + 1);
      ++contadas;
    }
  }
  return crua;
}

// O CARET: a cella de UMA collunha onde o cursor do terminal pousa, na posição
// logo a seguir ao ultimo caractere digitado. Barra QUIETA, e não a piscar: a
// queixa que abriu a issue irmã #78 foi «o meu cursor fica piscando», e dar-lhe
// caret que pisca seria responder á queixa com a propria queixa.
//
// É o UNICO nó d'esta obra que pede foco, e ha de continuar a ser: o `Render` do
// FTXUI elege UM nó focado por quadro e cala os outros sem aviso, donde dous
// pedidos seriam um pedido a perder-se em silencio.
//
// Espaço, e não cadeia vazia: o cursor pousa no `x_min` da caixa d'este nó, e nó
// de largura zero não tem caixa que sirva de endereço. E pinta-se tambem, que o
// terminal que não mostre cursor deixaria o campo mudo.
//
// Mergeada a #78, o corpo d'esta funcção passa a ser `tui::caret_do_campo()`,
// que é a mesma cella e o mesmo decorador: a forma do cursor decide-se n'um
// logar só, e o merge não escolhe ao acaso qual das duas versões fica.
ftxui::Element caret(ftxui::Color fundo) {
  return ftxui::text(" ") | ftxui::bgcolor(fundo) | ftxui::focusCursorBar;
}

}  // namespace

bool aceita_letra(Modo modo) noexcept {
  return modo != Modo::Nada && modo != Modo::Confirma;
}

std::size_t linhas_do_topo(Modo modo) noexcept {
  return modo == Modo::Nada ? 1u : 2u;
}

bool assenta_novidade(Modo modo) noexcept { return modo == Modo::Nada; }

// Os ROTULOS. Ficam as palavras que a janella já dizia, e sómente o filtro
// ganha palavra sua: uma barra sozinha n'uma linha nova lê-se como sujeira da
// tela, e não como campo em que se digita.
//
// O switch não tem `default`, de proposito: modo novo que se acrescente ao enum
// deixa de compilar aqui, em vez de sahir mudo na tela.
std::string rotulo_do_prompt(Modo modo, std::string_view contexto) {
  switch (modo) {
    case Modo::Busca: return "FILTRO:";
    case Modo::Url: return "URL:";
    case Modo::Procura: return "BUSCA NA REDE (" + std::string(contexto) + "):";
    case Modo::Lista: return "PLAYLIST DO SPOTIFY:";
    case Modo::NomeNovo: return "LISTA NOVA:";
    case Modo::NomeOutro: return "NOME:";
    case Modo::Confirma: return "apagar «" + std::string(contexto) + "»? s/n";
    case Modo::Nada: break;
  }
  return {};
}

// O TOPO. Duas linhas havendo prompt, e a trilha não é uma d'ellas por
// accidente: ella entra e sahe INTACTA. O prompt distingue-se por tres signaes
// ao mesmo tempo, a marca á esquerda, o fundo proprio e a tinta viva, que a
// trilha é apagada e sem fundo. Não cabendo tudo, perde-se o COMEÇO: duas
// collunhas vão para a marca e uma fica de reserva para o caret.
ftxui::Element elemento_do_topo(const std::string& trilha, Modo modo,
                                std::string_view contexto,
                                const std::string& termo,
                                std::size_t largura) {
  ftxui::Element a_trilha = ftxui::text(trilha) | ftxui::dim;
  if (modo == Modo::Nada) return a_trilha;
  const std::string rotulo = rotulo_do_prompt(modo, contexto);
  const std::string junto = aceita_letra(modo) ? rotulo + " " + termo : rotulo;
  const std::size_t cabe = largura > 3 ? largura - 3 : 1;
  const tokens::Triade fundo = tokens::rgb(tokens::panel_hi);
  const tokens::Triade viva = tokens::rgb(tokens::v500);
  ftxui::Elements campo{pinta("\u258c ", tokens::v500),
                        pinta(rabo(junto, cabe), tokens::text_bright)};
  // O Confirma não leva caret: n'elle não se digita, responde-se com uma tecla.
  if (aceita_letra(modo))
    campo.push_back(caret(ftxui::Color::RGB(viva.r, viva.g, viva.b)));
  campo.push_back(ftxui::filler());
  return ftxui::vbox(
      {a_trilha, ftxui::hbox(std::move(campo)) |
                     ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g,
                                                      fundo.b))});
}

}  // namespace mysong::tui
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
