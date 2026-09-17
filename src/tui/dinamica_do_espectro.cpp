#include "tui/dinamica_do_espectro.hpp"

#include "tui/espectro.hpp"

namespace mysong::tui {

void DinamicaDoEspectro::avanca(const std::vector<float>& bandas, bool mudo) {
  const auto agora = std::chrono::steady_clock::now();
  const double segundos =
      std::chrono::duration<double>(agora - instante_).count();
  instante_ = agora;
  std::lock_guard<std::mutex> guarda(tranca_);
  if (mudo) picos_.clear();
  avanca_picos(picos_, bandas, segundos);
}

std::vector<float> DinamicaDoEspectro::retrato() const {
  std::lock_guard<std::mutex> guarda(tranca_);
  return picos_;
}

void DinamicaDoEspectro::zera() {
  std::lock_guard<std::mutex> guarda(tranca_);
  picos_.clear();
  instante_ = std::chrono::steady_clock::now();
}

}  // namespace mysong::tui
