// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TELA DOS REQUISITOS — src/tui/tela_requisitos.cpp
// ══════════════════════════════════════════════════════════════════════════
// A obra da tela. Nada se sonda aqui: recebe-se o relatorio já colhido e
// mostra-se, ou em texto puro, ou em quadro pintado.
//
// DOMÍNIO ......... um relatorio da sonda.
// CONTRA-DOMÍNIO .. cadeia de texto sem escape algum, ou elemento do FTXUI.
// INVARIANTE ...... o texto puro é PURO: nenhuma sequencia de escape, nenhuma
//                   tela alternativa, nada que suje o tubo de quem o rediriga.
// Q.E.D. .......... o texto e o quadro sahem do mesmo relatorio e da mesma
//                   frase de limite; donde não podem contar historias
//                   differentes ao mesmo operador.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/tela_requisitos.hpp"

#include <string>

namespace mysong::tui {

namespace {

// O rotulo da gravidade, guarnecido á mesma largura, para que a columna do
// estado se alinhe sem se calcular largura nenhuma.
std::string_view rotulo_da_gravidade(nucleo::Gravidade gravidade) {
  return gravidade == nucleo::Gravidade::Impedimento ? "impedimento"
                                                     : "aviso      ";
}

}  // namespace

// texto_do_relatorio — o diagnostico. Diz TODOS os requisitos, e não sómente os
// que faltam: saber que a libmpv está presente é metade do valor d'isto.
std::string texto_do_relatorio(const nucleo::Relatorio& relatorio) {
  std::string texto = "mysong: sonda dos requisitos do systema\n\n";
  for (const nucleo::Estado& estado : relatorio.estados) {
    texto += "  ";
    texto.append(rotulo_da_gravidade(estado.requisito.gravidade));
    texto += estado.presente ? "  presente  " : "  FALTA     ";
    texto.append(estado.requisito.nome);
    texto += '\n';
    if (!estado.presente) {
      texto += "                 remedio: ";
      texto.append(estado.requisito.remedio);
      texto += '\n';
    }
  }
  texto += '\n';
  texto.append(kLimiteDaSonda);
  texto += "\n\nOs requisitos minimos d'esta obra estão no README.\n";
  return texto;
}

}  // namespace mysong::tui

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
