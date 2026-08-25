// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TABELLA — src/tui/tabella.cpp
// ══════════════════════════════════════════════════════════════════════════
// A pintura. Vale a regra do cabeçalho: pinta e sahe.
//
// DOMÍNIO ......... o Navegador por leitura, e a geometria.
// CONTRA-DOMÍNIO .. elementos do FTXUI.
// INVARIANTE ...... funcção alguma d'aqui muta o navegador. O parametro é
//                   `const&`, e o compilador guarda a regra.
// Q.E.D. .......... a decisão toda vive no navegador, e é lá que se prova; aqui
//                   sómente se traduz estado em tinta.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/tabella.hpp"

#include <string>
#include <utility>
#include <vector>

#include "tui/tokens.hpp"
#include "tui/transporte.hpp"

namespace mysong::tui {

namespace {

ftxui::Element pinta(const std::string& texto, std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::text(texto) |
         ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

// apara — a cadeia em `largura` collunhas, contando CODEPOINTS e não bytes. Sem
// isto, um titulo com acentos sahiria mais curto do que a conta diz e a tabella
// perderia o alinhamento das columnas.
std::string apara(const std::string& crua, std::size_t largura) {
  std::string feita;
  std::size_t contadas = 0;
  for (std::size_t i = 0; i < crua.size(); ++i) {
    if ((static_cast<unsigned char>(crua[i]) & 0xC0) != 0x80) {
      if (contadas == largura) break;
      ++contadas;
    }
    feita += crua[i];
  }
  while (contadas++ < largura) feita += ' ';
  return feita;
}

}  // namespace

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
