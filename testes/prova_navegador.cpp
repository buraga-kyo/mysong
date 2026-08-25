// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO NAVEGADOR — testes/prova_navegador.cpp
// ══════════════════════════════════════════════════════════════════════════
// Percorre o caminho inteiro do aceite, de Artistas a uma faixa, sobre um índice
// que esta bateria escreve. Sem terminal, sem motor, sem som.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <filesystem>
#include <string>
#include <vector>

#include "nucleo/biblioteca.hpp"
#include "tui/navegador.hpp"

namespace nu = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-nav-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  std::filesystem::path banco() const { return caminho_ / "indice.sqlite3"; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
