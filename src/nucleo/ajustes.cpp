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

// aparar — tira os brancos das DUAS pontas, e sómente das pontas: branco no
// MEIO do valor é do valor, que caminho com espaço é caminho legitimo.
std::string_view aparar(std::string_view texto) {
  const auto branco = [](char letra) {
    return letra == ' ' || letra == '\t' || letra == '\r' || letra == '\v' ||
           letra == '\f';
  };
  while (!texto.empty() && branco(texto.front())) texto.remove_prefix(1);
  while (!texto.empty() && branco(texto.back())) texto.remove_suffix(1);
  return texto;
}

// corta_commentario — o `#` abre commentario até o fim da linha, em QUALQUER
// ponto, e não ha aspas nem escape que o façam literal. O limite é DECLARADO, e
// não descuido: caminho que traga cerquilha fica inexprimivel, e regra com
// excepção seria regra que o operador não adivinha olhando o proprio arquivo.
std::string_view corta_commentario(std::string_view linha) {
  const std::size_t cerquilha = linha.find('#');
  if (cerquilha == std::string_view::npos) return linha;
  return linha.substr(0, cerquilha);
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
