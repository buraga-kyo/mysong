// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROMPT — src/tui/prompt.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada dos modos, e a pintura do campo. O contracto está em prompt.hpp.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/prompt.hpp"

#include <utility>

#include <ftxui/screen/string.hpp>

#include "tui/tabella.hpp"
#include "tui/tokens.hpp"

namespace mysong::tui {

namespace {

ftxui::Element pinta(const std::string& texto, std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::text(texto) | ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

// collunhas — a conta de COLLUNHAS, e não de codepoints. O glifo largo (CJK,
// emoji) vale DUAS, e contá-lo por uma punha o caret uma collunha ALÉM da
// ultima: o FTXUI conta ahi `dimx - 1 - cursor_.x` e manda `ESC[-1D`, que é
// escape mal formado a sahir ao terminal do operador. Até a issue #102 a folga
// de quatro collunhas da orla o engolia; a tela nova não tem orla.
std::size_t collunhas(const std::string& crua) {
  const int medida = ftxui::string_width(crua);
  return medida < 0 ? 0 : static_cast<std::size_t>(medida);
}

// larga_a_letra — as collunhas do codepoint que começa em `i` e acaba em `fim`.
std::size_t larga_a_letra(const std::string& crua, std::size_t i,
                          std::size_t fim) {
  return collunhas(crua.substr(i, fim - i));
}

// rabo — o FIM da cadeia, em `largura` COLLUNHAS: é o par do `apara` da
// tabella, que guarda o começo. Aqui guarda-se o fim, que é o que o operador
// acabou de teclar.
std::string rabo(const std::string& crua, std::size_t largura) {
  std::size_t gastas = 0, fim = crua.size();
  for (std::size_t i = crua.size(); i > 0;) {
    --i;
    if ((static_cast<unsigned char>(crua[i]) & 0xC0) == 0x80) continue;
    const std::size_t vale = larga_a_letra(crua, i, fim);
    if (gastas + vale > largura) return crua.substr(fim);
    gastas += vale;
    fim = i;
  }
  return crua;
}

// cabeca — o COMEÇO da cadeia, nas mesmas COLLUNHAS: o par do rabo, para o
// rotulo que se apara á direita quando nem elle cabe. Do rotulo é o começo que
// diz o officio; do termo, o fim é o que se acabou de teclar.
std::string cabeca(const std::string& crua, std::size_t largura) {
  std::size_t gastas = 0;
  for (std::size_t i = 0; i < crua.size();) {
    std::size_t fim = i + 1;
    while (fim < crua.size() &&
           (static_cast<unsigned char>(crua[fim]) & 0xC0) == 0x80)
      ++fim;
    const std::size_t vale = larga_a_letra(crua, i, fim);
    if (gastas + vale > largura) return crua.substr(0, i);
    gastas += vale;
    i = fim;
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
  // A troca do R8, feita no merge: a fórma do cursor decide-se n'um logar só,
  // o caret_do_campo da tabella (issue #78); o fundo do campo pinta-se por cima.
  return caret_do_campo() | ftxui::bgcolor(fundo);
}

}  // namespace

bool aceita_letra(Modo modo) noexcept {
  return modo != Modo::Nada && modo != Modo::Confirma &&
         modo != Modo::ConfirmaFaixa;
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
    // O titulo da faixa, e não «NOME:»: o campo abre já com o titulo corrente
    // dentro, e o rotulo ha de dizer o que aquelle texto é.
    case Modo::TituloOutro: return "TITULO:";
    // A MESMA pergunta do apagar da lista, palavra por palavra. Duas perguntas
    // com a mesma resposta e redacções differentes fariam o operador ler duas
    // vezes para saber se a tecla que responde é a mesma.
    case Modo::Confirma:
    case Modo::ConfirmaFaixa:
      return "apagar «" + std::string(contexto) + "»? s/n";
    case Modo::Nada: break;
  }
  return {};
}

// O CAMPO, n'uma linha. Distingue-se do resto da tela por tres signaes ao mesmo
// tempo: a marca á esquerda, o fundo proprio e a tinta viva. Não cabendo tudo,
// quem perde o começo é o TERMO, e o rotulo mostra-se inteiro: rabo de texto
// sem nome de campo é o defeito da issue #79 por outra porta. Não cabendo nem
// o rotulo, apara-se elle á direita e o caret pousa na ultima collunha. Duas
// collunhas vão para a marca e uma fica de reserva para o caret.
ftxui::Element elemento_do_campo(Modo modo, std::string_view contexto,
                                 const std::string& termo,
                                 std::size_t largura) {
  if (modo == Modo::Nada) return ftxui::emptyElement();
  const std::string rotulo = rotulo_do_prompt(modo, contexto);
  const std::size_t cabe = largura > 3 ? largura - 3 : 1;
  const std::size_t do_rotulo = collunhas(rotulo);
  const std::string mostra =
      aceita_letra(modo) && do_rotulo + 1 < cabe
          ? rotulo + " " + rabo(termo, cabe - do_rotulo - 1)
          : cabeca(rotulo, cabe);
  const tokens::Triade fundo = tokens::rgb(tokens::panel_hi);
  const tokens::Triade viva = tokens::rgb(tokens::v500);
  ftxui::Elements campo{pinta("\u258c ", tokens::v500),
                        pinta(mostra, tokens::text_bright)};
  // O Confirma não leva caret: n'elle não se digita, responde-se com uma tecla.
  if (aceita_letra(modo))
    campo.push_back(caret(ftxui::Color::RGB(viva.r, viva.g, viva.b)));
  campo.push_back(ftxui::filler());
  return ftxui::hbox(std::move(campo)) |
         ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g, fundo.b));
}

}  // namespace mysong::tui
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
