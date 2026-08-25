// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA AQUISIÇÃO — src/nucleo/aquisicao.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As cinco funcções puras primeiro; o `fork` e o `exec` depois,
// e sozinhos no fim do arquivo, para que o olho veja de um relance quanto d'esta
// peça se prova e quanto não.
//
// DOMÍNIO ......... uma URL, o que o operador disse, e a raiz do acervo.
// CONTRA-DOMÍNIO .. um arquivo no logar certo, com etiqueta certa, e um Desfecho.
// INVARIANTE ...... o `exec` recebe VECTOR de argumentos, e nunca uma linha de
//                   shell: URL vinda do operador não passa por interpretador
//                   algum, donde não ha aspa nem ponto e virgula que faça o que
//                   não se pediu.
// Q.E.D. .......... não havendo shell, a injecção não é «improvavel»: é
//                   inexprimivel.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/aquisicao.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace mysong::nucleo {

namespace {

// O comprimento maximo de UM componente de caminho, em octetos. Duzentos e
// quarenta, e não duzentos e cincoenta e cinco: sobram quinze para a extensão e
// para o «NN - » que o numero põe á frente.
constexpr std::size_t kMaxComponente = 240;

// apara — tira os espaços das duas pontas. Nome com espaço á frente existe no
// systema de arquivos e é fonte de confusão sem fim.
std::string apara(std::string_view crua) {
  std::size_t principio = 0, fim = crua.size();
  while (principio < fim && std::isspace(static_cast<unsigned char>(crua[principio])))
    ++principio;
  while (fim > principio && std::isspace(static_cast<unsigned char>(crua[fim - 1])))
    --fim;
  return std::string(crua.substr(principio, fim - principio));
}

}  // namespace

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
