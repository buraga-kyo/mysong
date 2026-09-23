#pragma once

#include <condition_variable>
#include <mutex>

namespace mysong::tui {

// Uma campainha conserva no maximo UM pedido pendente. Toques repetidos antes
// da colheita fundem-se; toque durante trabalho fica para a passagem seguinte.
class Campainha {
 public:
  explicit Campainha(bool tocada = false) noexcept : tocada_(tocada) {}
  Campainha(const Campainha&) = delete;
  Campainha& operator=(const Campainha&) = delete;

  void toca() noexcept {
    {
      std::lock_guard<std::mutex> guarda(tranca_);
      if (fechada_) return;
      tocada_ = true;
    }
    condicao_.notify_one();
  }

  bool espera() noexcept {
    std::unique_lock<std::mutex> guarda(tranca_);
    condicao_.wait(guarda, [&] { return tocada_ || fechada_; });
    if (fechada_) return false;
    tocada_ = false;
    return true;
  }

  void fecha() noexcept {
    {
      std::lock_guard<std::mutex> guarda(tranca_);
      fechada_ = true;
    }
    condicao_.notify_all();
  }

 private:
  std::mutex tranca_;
  std::condition_variable condicao_;
  bool tocada_ = false;
  bool fechada_ = false;
};

}  // namespace mysong::tui
