// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CORREIO — src/tui/correio.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. A colheita compara duas contagens em vez de olhar uma bandeira, e
// é isso que faz a substituição funccionar: recado posto duas vezes sem se colher uma
// vez conta duas, e a colheita ALCANÇA a contagem em vez de a seguir de um em um.
//
// DOMÍNIO ......... o recado posto.
// CONTRA-DOMÍNIO .. o recado colhido, uma vez.
// INVARIANTE ...... colhida_ nunca passa posta_, e ambas sómente crescem.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/correio.hpp"

#include <utility>

namespace mysong::tui {

void Correio::poe(std::vector<Linha> achados, std::string recado) {
  std::lock_guard<std::mutex> chave(tranca_);
  achados_ = std::move(achados);
  recado_ = std::move(recado);
  ++posta_;
}

bool Correio::colhe(std::vector<Linha>* achados, std::string* recado) {
  std::lock_guard<std::mutex> chave(tranca_);
  if (colhida_ == posta_) return false;
  colhida_ = posta_;
  // MOVE-SE, e não se copia: a lista pode ter vinte achados, e o fio da tela é o
  // unico que a vae ler. Deixá-la aqui seria guardar cópia que ninguem lê.
  if (achados != nullptr) *achados = std::move(achados_);
  if (recado != nullptr) *recado = std::move(recado_);
  achados_.clear();
  recado_.clear();
  return true;
}

unsigned long Correio::geracao() const {
  std::lock_guard<std::mutex> chave(tranca_);
  return posta_;
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
