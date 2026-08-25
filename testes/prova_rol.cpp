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

TEST_CASE("o saneamento do nome apara os brancos, e nome vazio não vale") {
  CHECK(nu::saneia_nome_de_rol("  Da manhã  ") == "Da manhã");
  CHECK(nu::saneia_nome_de_rol("\t\n ").empty());
  CHECK(nu::saneia_nome_de_rol("").empty());
  // O corte pelo comprimento recúa até ao byte lider: cortar UTF-8 a meio deixaria
  // byte solto, e o nome sahiria com um losango no fim.
  const std::string comprido(119, 'x');
  const std::string cortado = nu::saneia_nome_de_rol(comprido + "á");
  CHECK(cortado.size() == 119);
  CHECK(cortado == comprido);
}

TEST_CASE("o banco nasce na primeira abertura, com a versão assentada") {
  Cova cova;
  CHECK_FALSE(std::filesystem::exists(cova.banco()));
  nu::Roleiro roleiro(cova.banco());
  REQUIRE(roleiro.aberto());
  CHECK(roleiro.versao() == nu::kVersaoDoRol);
  CHECK(roleiro.rois().empty());
  // Abrir DUAS vezes não duplica a versão: o esquema é IF NOT EXISTS, e a versão
  // sómente se assenta havendo zero linhas.
  nu::Roleiro outro(cova.banco());
  CHECK(outro.versao() == nu::kVersaoDoRol);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
