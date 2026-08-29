// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROMPT — src/tui/prompt.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada dos modos, e a pintura do topo. O contracto está em prompt.hpp.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/prompt.hpp"

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

}  // namespace

bool aceita_letra(Modo modo) noexcept {
  return modo != Modo::Nada && modo != Modo::Confirma;
}

std::size_t linhas_do_topo(Modo modo) noexcept {
  return modo == Modo::Nada ? 1u : 2u;
}

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
  return ftxui::vbox(
      {a_trilha,
       ftxui::hbox({pinta("\u258c ", tokens::v500),
                    pinta(rabo(junto, cabe), tokens::text_bright),
                    ftxui::filler()}) |
           ftxui::bgcolor(ftxui::Color::RGB(fundo.r, fundo.g, fundo.b))});
}

}  // namespace mysong::tui
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
