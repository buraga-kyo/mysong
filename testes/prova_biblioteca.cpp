// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA BIBLIOTHECA — testes/prova_biblioteca.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o ÍNDICE, e não o disco: nenhum caso d'este arquivo abre arquivo de
// audio, chama ffmpeg, ou toca taglib. As faixas entram amarradas á mão, o que
// deixa a prova das consultas independente da prova da varredura.
//
// E nenhum caso toca o índice do operador. Todo banco d'esta bateria nasce em
// directorio temporario proprio e morre com o caso; a ~/.local/share/mysong
// real não se lê nem se escreve, e é o C16 que o afere antes e depois.
//
// DOMÍNIO ......... bancos temporarios, enchidos linha a linha pelo caso.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... o alvo de cada asserção está ESCRIPTO no caso, e nunca se
//                   colhe da obra sob exame: prova que pergunta á obra o que a
//                   obra devia responder não prova cousa alguma.
#include <doctest/doctest.h>

#include <string>

#include "nucleo/biblioteca.hpp"

namespace nu = mysong::nucleo;

namespace {
// O caracter de substituição, por extenso e n'uma constante, porque colado a
// uma letra hexadecimal dentro de um literal elle seria lido como outro byte.
const std::string kTroca = "\xEF\xBF\xBD";
}  // namespace

TEST_CASE("saneia_utf8 conserva o valido e troca o invalido") {
  CHECK(nu::saneia_utf8("") == "");
  CHECK(nu::saneia_utf8("Ada Lovelace") == "Ada Lovelace");
  CHECK(nu::saneia_utf8("Máquina Analítica") == "Máquina Analítica");
  CHECK(nu::saneia_utf8("\xF0\x9F\x8E\xB5") == "\xF0\x9F\x8E\xB5");
  // Um byte de arranque a que falta a continuação: cae elle, e o '(' fica.
  CHECK(nu::saneia_utf8("\xC3\x28") == kTroca + "(");
  CHECK(nu::saneia_utf8("\xFF") == kTroca);
  CHECK(nu::saneia_utf8("\xC3") == kTroca);
  // Latin-1 mal etiquetado: o 0xE9 do «é» não é UTF-8, e o resto sobrevive.
  CHECK(nu::saneia_utf8("Ada\xE9Lovelace") == "Ada" + kTroca + "Lovelace");
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
