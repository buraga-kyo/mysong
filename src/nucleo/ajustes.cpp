// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DOS AJUSTES — src/nucleo/ajustes.cpp
// ══════════════════════════════════════════════════════════════════════════
// A obra dos ajustes, em partes que se não misturam: o LEITOR, que parte o
// texto em pares e nada sabe do mundo; o RESOLVEDOR, que escolhe entre os
// quatro degraus e nada abre; e as funcções que tocam disco e ambiente.
//
// INVARIANTE ...... arquivo algum se abre para escripta n'esta unidade.
// Q.E.D. .......... puros o leitor e o resolvedor, a bateria prova o formato e
//                   a precedencia inteira sem tocar disco nem ambiente.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/ajustes.hpp"

#include <string>
#include <utility>

namespace mysong::nucleo {

// nome_da_origem — a palavra do --sonda. Switch exhaustivo, e não taboa: origem
// nova accende aviso do compilador em vez de sahir calada como «sem nome».
std::string_view nome_da_origem(Origem origem) {
  switch (origem) {
    case Origem::Argumento: return "argumento";
    case Origem::Ambiente: return "ambiente";
    case Origem::Arquivo: return "arquivo";
    case Origem::Padrao: return "padrão";
  }
  return "origem sem nome";
}

// queixa — accrescenta, até o tecto. Alcançado elle, deixa-se UMA linha a dizer
// que ha mais: é o que faz um binario passado como conf caber n'uma tela.
void Ajustes::queixa(std::string dito) {
  if (queixas.size() < QUEIXAS_NO_MAXIMO) {
    queixas.push_back(std::move(dito));
  } else if (queixas.size() == QUEIXAS_NO_MAXIMO) {
    queixas.push_back("e ha mais queixas, que o tecto de " +
                      std::to_string(QUEIXAS_NO_MAXIMO) + " se alcançou");
  }
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
