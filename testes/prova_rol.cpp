// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO ROL — testes/prova_rol.cpp
// ══════════════════════════════════════════════════════════════════════════
// Corre inteira em directorio temporario: o caminho do banco entra por parâmetro,
// donde prova alguma pode tocar as listas de quem nos usa. O que ella afere é a
// ORDEM, e o defeito que a peça pode ter é ordem com buraco: retirar o do meio e
// deixar zero e dous.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "nucleo/rol.hpp"

namespace nu = mysong::nucleo;

namespace {

class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-rol-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  std::filesystem::path banco() const { return caminho_ / "rol.sqlite3"; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
