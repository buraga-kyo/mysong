// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO ESTALEIRO — testes/prova_estaleiro.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. A OBRA entra por parametro, e no logar
// d'ella põe-se aqui uma que se deixa SEGURAR: os casos param a obra a meio,
// olham o estaleiro, e sómente então a soltam. Donde o limite se afere por
// construcção, e não por relogio: prova do genero «esperei um segundo e não
// passou de dous» é prova que a machina carregada perde.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

#include "nucleo/estaleiro.hpp"

namespace nu = mysong::nucleo;

namespace {

// A CANCELLA. A obra de mentira para n'ella, e sómente passa quando o caso
// abrir. É o que substitue a espera por relogio.
class Cancella {
 public:
  void espera() {
    std::unique_lock<std::mutex> chave(tranca_);
    sino_.wait(chave, [this] { return aberta_; });
  }
  void abre() {
    {
      std::lock_guard<std::mutex> chave(tranca_);
      aberta_ = true;
    }
    sino_.notify_all();
  }
  // chegaram — espera que `quantos` obreiros tenham CHEGADO á cancella. Sem isto o
  // caso olharia o estaleiro antes de elle ter começado, e leria zero por engano.
  void chegaram(std::size_t quantos) {
    std::unique_lock<std::mutex> chave(tranca_);
    sino_.wait(chave, [this, quantos] { return chegados_ >= quantos; });
  }
  void chego() {
    {
      std::lock_guard<std::mutex> chave(tranca_);
      ++chegados_;
    }
    sino_.notify_all();
  }

 private:
  std::mutex tranca_;
  std::condition_variable sino_;
  bool aberta_ = false;
  std::size_t chegados_ = 0;
};

}  // namespace
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
