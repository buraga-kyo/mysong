// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CORREIO — src/tui/correio.hpp
// ══════════════════════════════════════════════════════════════════════════
// O RECADO que um fio de fundo deixa á tela. Um só cabe de cada vez: recado novo
// substitue o velho, que ninguem quer ver a busca de antes chegar depois da de agora.
// A tela do FTXUI corre n'um fio, e a busca na rede n'outro; o fio da busca NÃO toca
// a tela nem o navegador, e deixa aqui o que achou.
//
// DOMÍNIO ......... o que o fio da rede achou, e o recado que quer dar.
// CONTRA-DOMÍNIO .. o mesmo, entregue UMA vez ao fio da tela.
// INVARIANTE ...... a colheita CONSOME: colhido duas vezes, o segundo devolve
//                   falso. Sem isso, a tela poria a lista outra vez a cada quadro.
// Q.E.D. .......... a GERAÇÃO é publica e cresce a cada recado posto, donde o fio
//                   do relogio a lê para saber que ha repintura a pedir, sem ter
//                   de colher o recado que não é d'elle.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "tui/navegador.hpp"

namespace mysong::tui {

// GABARITO no que carrega, e não uma classe por genero de recado. A issue #13
// trouxe o segundo fio de rede, e o que elle acha não é Linha: uma classe por genero
// seria a mesma conta das duas contagens escripta duas vezes, e a que se corrigisse
// deixava a outra a errar.
template <class Carga>
class CorreioDe {
 public:
  // poe — deixa o recado, substituindo o que houvesse.
  void poe(std::vector<Carga> achados, std::string recado) {
    std::lock_guard<std::mutex> chave(tranca_);
    achados_ = std::move(achados);
    recado_ = std::move(recado);
    ++posta_;
  }

  // colhe — VERDADEIRO uma vez por recado posto. Colhido, o recado sahe d'aqui.
  bool colhe(std::vector<Carga>* achados, std::string* recado) {
    std::lock_guard<std::mutex> chave(tranca_);
    if (colhida_ == posta_) return false;
    colhida_ = posta_;
    // MOVE-SE, e não se copia: a lista pode ter cincoenta achados, e o fio da tela é
    // o unico que a vae ler.
    if (achados != nullptr) *achados = std::move(achados_);
    if (recado != nullptr) *recado = std::move(recado_);
    achados_.clear();
    recado_.clear();
    return true;
  }

  // geracao — quantos recados se pôz desde sempre. Cresce, e nunca decresce: é o que
  // o fio do relogio compara para saber que ha repintura a pedir.
  unsigned long geracao() const {
    std::lock_guard<std::mutex> chave(tranca_);
    return posta_;
  }

 private:
  mutable std::mutex tranca_;
  std::vector<Carga> achados_;
  std::string recado_;
  unsigned long posta_ = 0;
  unsigned long colhida_ = 0;
};

// O CORREIO dos achados da rede, que é o da issue #12. O nome fica, e o corpo é o
// gabarito: quem o usa não muda uma linha.
using Correio = CorreioDe<Linha>;

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
