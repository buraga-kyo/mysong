#pragma once

#include <chrono>
#include <mutex>
#include <vector>

namespace mysong::tui {

class DinamicaDoEspectro {
 public:
  void avanca(const std::vector<float>& bandas, bool mudo);
  std::vector<float> retrato() const;
  void zera();

 private:
  mutable std::mutex tranca_;
  std::vector<float> picos_;
  std::chrono::steady_clock::time_point instante_ =
      std::chrono::steady_clock::now();
};

}  // namespace mysong::tui
