// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO EXEMPLO DA FITA — exemplos/fita_arrowline.cpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta uma fita de TRES segmentos, para se olhar. Não é prova: é a peça que o
// olho ha de julgar, n'um Alacritty com JetBrainsMono Nerd Font.
//
// DOMÍNIO ......... o vacuo. A fita é armada aqui mesmo, sem entrada alguma.
// CONTRA-DOMÍNIO .. uma linha na sahida padrão, e o status zero.
// INVARIANTE ...... o par tinta/fundo sahe IMMEDIATAMENTE antes do texto que
//                   veste, sem repouso pelo meio: é o que fecha a emenda
//                   entre o enchimento de um segmento e a ponta do outro.
// Q.E.D. .......... redigida a sahida a um arquivo, os bytes dizem quaes
//                   glifos sahiram, onde, e com que par de côres. Que a fonte
//                   os resolva é materia que sómente o olho decide.
// ══════════════════════════════════════════════════════════════════════════
#include <cstdio>
#include <string>

#include "tui/arrowline.hpp"
#include "tui/tokens.hpp"

namespace tk = mysong::tui::tokens;
namespace al = mysong::tui;

int main() {
  // A guarnição dos flancos entra no proprio rotulo, que é do desenhista. E o
  // segundo segmento veste o degrau REBAIXADO do primeiro: a regra (c) á vista.
  al::Fita fita;
  fita.junta({" TOCANDO ", tk::v500, tk::v50})
      .junta({" A NOITE INTEIRA ", al::rebaixar(tk::v500), tk::text_bright})
      .junta({" 03:41 ", tk::v900, tk::text_muted});

  std::string tela;
  for (const al::Pedaco& pedaco : fita.compor()) {
    tela += pedaco.fundo == tk::transparent ? std::string(tk::repouso)
                                            : tk::fundo_de(pedaco.fundo);
    tela += tk::tinta(pedaco.tinta);
    tela += pedaco.texto;
  }
  tela += tk::repouso;
  std::printf("%s\n", tela.c_str());
  return 0;
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
