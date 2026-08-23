// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SONDA — src/nucleo/sonda.hpp
// ══════════════════════════════════════════════════════════════════════════
// Colhe o estado dos REQUISITOS de que o tocador depende, e nada mais faz:
// não pinta tela, não escreve em sahida alguma, não sahe do programa. Quem
// julga o que fazer com a falta é a tela; quem sabe o que falta é esta sonda.
//
// A FONTE DA VERDADE ENTRA POR PARÂMETRO, e isto é a substancia do modulo e
// não commodidade: numa máquina em que os quatro requisitos estão presentes, a
// sonda que só soubesse consultar o systema real nunca correria o caminho da
// FALTA, que é justamente o caminho que importa. Injectado o inquerito, os dous
// caminhos provam-se com dublê, sem se mexer no systema.
//
// DOMÍNIO ......... um INQUERITO: tres consultas que respondem se ha familia de
//                   fonte, se ha bibliotheca, se ha executavel. Nunca o systema
//                   directamente.
// CONTRA-DOMÍNIO .. um RELATORIO: a taboa dos requisitos, cada um com o seu
//                   estado, na ordem de declaração e sem buraco.
// INVARIANTE ...... sondar() é pura quanto ao mundo: não lê variavel de
//                   ambiente, não abre arquivo, não emitte byte, não lança
//                   excepção pela borda. Consulta que falhe conta-se FALTA, e
//                   jamais se confunde com requisito presente.
// Q.E.D. .......... havendo o inquerito por parâmetro, o caminho do impedimento
//                   é observavel onde nada falta; logo a recusa de abrir deixa
//                   de ser promessa e passa a ser asserção provada.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string_view>

namespace mysong::nucleo {

// A GRAVIDADE da falta, que é o que decide o destino do programa. Duas, e
// sómente duas: o Impedimento tranca a porta, o Aviso apenas a rannge.
enum class Gravidade { Impedimento, Aviso };

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
