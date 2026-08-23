// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA FILA, LAVRA — src/nucleo/fila.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o que o cabecalho promette. Nada aqui abre arquivo, nada aqui fala
// com o mpv, e nada aqui ergue excepção.
//
// DOMÍNIO ......... o proprio estado da fila, e cadeias que o cliente entrega.
// CONTRA-DOMÍNIO .. faixa corrente, contagens e respostas booleanas.
// INVARIANTE ...... indice_ menor que faixas_.size(), ou faixas_ vazia. Toda
//                   funcção que move o indice verifica a borda ANTES de mover,
//                   de sorte que a invariante nunca se quebra nem por um
//                   instante intermediario.
// Q.E.D. .......... sendo tudo arithmetica de indice sobre um vector, a prova
//                   não pede placa de som, nem disco, nem rede, e corre em
//                   machina surda no tempo de um piscar.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/fila.hpp"

#include <utility>

namespace mysong::nucleo {

void Fila::junta(std::string caminho) {
  faixas_.push_back(std::move(caminho));
}

bool Fila::vazia() const noexcept {
  return faixas_.empty();
}

std::size_t Fila::tamanho() const noexcept {
  return faixas_.size();
}

std::size_t Fila::indice() const noexcept { return indice_; }

// Cadeia vazia em fila vazia: estado legitimo, retracto fiel, e não anomalia.
std::string_view Fila::corrente() const noexcept {
  if (faixas_.empty()) return {};
  return faixas_[indice_];
}

// Os tres passos. Cada um verifica a borda ANTES de mover o indice, e por isso
// a invariante nunca se quebra, nem num instante intermediario.
bool Fila::proxima() noexcept {
  if (faixas_.empty() || indice_ + 1 >= faixas_.size()) return false;
  ++indice_;
  return true;
}

bool Fila::anterior() noexcept {
  if (faixas_.empty() || indice_ == 0) return false;
  --indice_;
  return true;
}

bool Fila::ir_para(std::size_t alvo) noexcept {
  if (alvo >= faixas_.size()) return false;
  indice_ = alvo;
  return true;
}

void Fila::esvazia() noexcept {
  faixas_.clear();
  indice_ = 0;
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
