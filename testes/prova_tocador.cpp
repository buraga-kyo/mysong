// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DO TOCADOR — testes/prova_tocador.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o tocador com um motor DUBLÊ: uma carne de mentira que obedece á
// mesma interface e não abre mpv, nem placa de som, nem arquivo. É o que a
// issue pede com essas palavras, e o que faz esta prova rodar em machina surda.
//
// DOMÍNIO ......... um Tocador armado sobre o dublê, e ordens de operador.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... o dublê REGISTRA o que lhe mandaram, e não só devolve; é
//                   d'ahi que a prova assere a CHAMADA, e não o resultado.
//                   Tocador que devolvesse verdadeiro sem mandar nada ao motor
//                   passaria por uma prova de resultado, e cae n'esta.
// Q.E.D. .......... prova-se aqui a mechanica: fila, transições e pregão. O
//                   contracto com a libmpv NÃO se prova aqui, e nenhum caso
//                   d'este arquivo o finge: para elle ha o binario que soa.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "nucleo/tocador.hpp"
namespace {

using mysong::nucleo::Estado;
// O DUBLÊ. Registra o que lhe mandaram, para que a prova o interrogue.
class MotorDuble final : public mysong::nucleo::Motor {
 public:
  std::vector<std::string> tocados;
  bool recusa_tocar = false;

  bool tocar(const std::string& caminho) override {
    if (recusa_tocar) return false;
    tocados.push_back(caminho);
    posicao_ = 0.0;
    estado_ = Estado::Tocando;
    return true;
  }

 private:
  double posicao_ = 0.0;
  Estado estado_ = Estado::Parado;
};

}  // namespace

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
