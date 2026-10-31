#pragma once
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

namespace mysong::nucleo {
struct CenaDoTerminal {
  bool visivel = true;
  bool ativo = true;
  std::string geometria;
  std::size_t geracao = 0;
};
CenaDoTerminal interpreta_cena_do_tmux(std::string_view resposta, const CenaDoTerminal& anterior = {});
class SessaoDoTerminal {
 public:
  using Consulta = std::function<std::string(const std::string&)>;
  explicit SessaoDoTerminal(std::string painel, Consulta consulta = {});
  ~SessaoDoTerminal();
  void inicia();
  void pulsa();
  CenaDoTerminal retrato() const;
 private:
  std::shared_ptr<std::atomic_bool> parar_ = std::make_shared<std::atomic_bool>(false);
  std::thread trabalho_;
  std::mutex espera_;
  std::condition_variable sino_;
  std::string painel_;
  Consulta consulta_;
  mutable std::mutex tranca_;
  CenaDoTerminal cena_;
  std::chrono::steady_clock::time_point proxima_{};
};
}
