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
#include <vector>

#include "tui/navegador.hpp"

namespace mysong::tui {

class Correio {
 public:
  // poe — deixa o recado, substituindo o que houvesse.
  void poe(std::vector<Linha> achados, std::string recado);

  // colhe — VERDADEIRO uma vez por recado posto. Colhido, o recado sahe d'aqui.
  bool colhe(std::vector<Linha>* achados, std::string* recado);

  // geracao — quantos recados se pôz desde sempre. Cresce, e nunca decresce: é o que
  // o fio do relogio compara para saber que ha repintura a pedir.
  unsigned long geracao() const;

 private:
  mutable std::mutex tranca_;
  std::vector<Linha> achados_;
  std::string recado_;
  unsigned long posta_ = 0;
  unsigned long colhida_ = 0;
};

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
