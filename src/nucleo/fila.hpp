// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA FILA — src/nucleo/fila.hpp
// ══════════════════════════════════════════════════════════════════════════
// Guarda a ORDEM que o cliente definir, e anda por ella nos dous sentidos. É
// fila NOSSA, e não playlist do mpv: delegar a ordem ao mpv seria entregar-lhe
// o governo d'ella, e com o governo o poder de a mudar por conta propria.
//
// DOMÍNIO ......... caminhos de arquivo, na ordem em que se juntaram.
// CONTRA-DOMÍNIO .. a faixa corrente, e resposta booleana a cada passo.
// INVARIANTE ...... o indice aponta SEMPRE para faixa existente, ou então a
//                   fila está vazia e não ha para onde apontar. Não ha indice
//                   pendurado: proxima() no ultimo e anterior() no primeiro
//                   devolvem falso e NÃO movem nada. A fila não envolve do
//                   ultimo ao primeiro, que é politica de repetição, e
//                   repetição esta fóra do que a issue pediu.
// Q.E.D. .......... sendo o indice nosso e a resposta booleana, a prova corre
//                   sem mpv, sem placa de som e sem arquivo em disco: os
//                   caminhos são meras cadeias, e a fila jamais os abre.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

class Fila {
 public:
  // Junta ao fim, preservando a ordem de chegada.
  void junta(std::string caminho);

  bool vazia() const noexcept;
  std::size_t tamanho() const noexcept;

  // O indice da faixa corrente. Sem sentido em fila vazia, e por isso a
  // vazia() se pergunta primeiro.
  std::size_t indice() const noexcept;

  // A faixa corrente. Cadeia vazia quando a fila está vazia — resposta, e não
  // erro. A vista vale enquanto a fila não se alterar.
  std::string_view corrente() const noexcept;

  bool proxima() noexcept;
  bool anterior() noexcept;
  bool ir_para(std::size_t alvo) noexcept;

  void esvazia() noexcept;

 private:
  std::vector<std::string> faixas_;
  std::size_t indice_ = 0;
};

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
