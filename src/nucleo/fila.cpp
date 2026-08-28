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

#include <algorithm>
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

const std::vector<std::string>& Fila::todas() const noexcept { return faixas_; }

// O EMBARALHAR. Ligar sorteia UMA ordem de toda a fila; desligar deita-a fóra. Nem
// uma cousa nem outra toca no indice_, que segue a FAIXA e não a posição: é por
// isso que desligar não troca a faixa que está a tocar.
//
// A faixa corrente vae ao PRINCIPIO da permutação, e não ao logar que o sorteio
// lhe desse. É a unica lavra que honra ao mesmo tempo as duas cousas que a issue
// pede: «a faixa corrente NÃO troca» e «nenhuma faixa torna antes de todas terem
// tocado». Cahindo ella no meio, as que ficassem atraz nunca tocariam n'aquella
// passagem, e a permutação não esgotaria.
void Fila::embaralhar(bool ligado) {
  embaralhado_ = ligado;
  ordem_.clear();
  passo_ = 0;
  if (!ligado || faixas_.empty()) return;
  ordem_.reserve(faixas_.size());
  ordem_.push_back(indice_);
  for (std::size_t assento = 0; assento < faixas_.size(); ++assento)
    if (assento != indice_) ordem_.push_back(assento);
  // Sorteia-se a CAUDA, do segundo assento em deante: o primeiro é a faixa que
  // toca, e mexer n'elle seria trocá-la.
  std::shuffle(ordem_.begin() + 1, ordem_.end(), sorteio_);
}

bool Fila::embaralhado() const noexcept { return embaralhado_; }

const std::vector<std::size_t>& Fila::ordem() const noexcept { return ordem_; }

// O REPETIR. Punho secco: assenta o modo e mais nada. Não move o indice, não
// toca na permutação, e não manda tocar cousa alguma; quem o lê é o andar da
// fila, logo abaixo. Assim ligar o repetir no meio de uma faixa não a
// interrompe, que é o que o operador espera de um modo.
void Fila::repetir(Repeticao modo) noexcept { repeticao_ = modo; }

Repeticao Fila::repeticao() const noexcept { return repeticao_; }

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
