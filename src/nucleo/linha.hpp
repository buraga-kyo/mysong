// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LINHA DE COMMANDO — src/nucleo/linha.hpp
// ══════════════════════════════════════════════════════════════════════════
// Lê os argumentos com que a obra foi invocada e diz o que ella ha de fazer.
// Funcção PURA: não lê ambiente, não abre tela e não sonda cousa alguma.
//
// DOMÍNIO ......... o argc e o argv tal como o systema os entrega.
// CONTRA-DOMÍNIO .. uma Invocacao: o modo, as faixas na ordem dada, e a razão
//                   da recusa, que sómente o modo Recusa preenche.
// INVARIANTE ...... o `--` encerra as opções, e o que vem depois d'elle é
//                   caminho VERBATIM, com traço e tudo; traço sozinho é
//                   caminho; opção que a taboa não conhece dá Recusa.
// Q.E.D. .......... a decisão sendo funcção pura, a bateria julga-a sem abrir
//                   terminal, motor nem som, e ao main() fica o despacho.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string>
#include <vector>

namespace mysong::nucleo {

// UM modo, e não pilha de bandeiras: os cinco excluem-se.
enum class Modo {
  Tocar,   // erguer o tocador com as faixas colhidas
  Sonda,   // o diagnostico dos requisitos, em texto (issue #22)
  Versao,  // o nome e o numero, e sahir
  Ajuda,   // as opções que existem, e sahir
  Recusa,  // opção que a obra não conhece; a razão diz qual
};

struct Invocacao {
  Modo modo = Modo::Tocar;
  std::vector<std::string> faixas;  // na ordem em que vieram
  std::string razao;                // preenchida SÓ no modo Recusa
};

// Lê a linha. Jamais lança: linha errada dá Modo::Recusa, e não excepção.
Invocacao ler_linha(int argc, const char* const* argv);

// O que o `--versao` escreve: a marca, o numero e o fim de linha.
std::string texto_da_versao();

// O que o `--ajuda` escreve: as opções que existem, em texto pelado.
std::string texto_da_ajuda();

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
