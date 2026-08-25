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

namespace {

nu::Faixa faz(const std::string& artista, const std::string& album,
              const std::string& titulo, int numero) {
  nu::Faixa faixa;
  faixa.caminho = "/acervo/" + artista + "/" + album + "/" + titulo + ".mp3";
  faixa.raiz = "/acervo";
  faixa.artista = artista;
  faixa.album = album;
  faixa.titulo = titulo;
  faixa.numero = numero;
  faixa.duracao = 100 + numero;
  return faixa;
}

// O ACERVO da prova, escripto Á MÃO aqui em cima para que os casos aferem a ORDEM
// contra uma taboa que se lê, e não contra o que a obra devolveu.
//   Ada Lovelace / Máquina  : 3 Tear, 7 Nota G
//   Ada Lovelace / Notas    : 1 Traducção
//   Bach         / Cravo     : 2 Fuga
bool enche(const std::filesystem::path& banco) {
  nu::Escriba escriba(banco);
  if (!escriba.aberto()) return false;
  return escriba.grava(faz("Ada Lovelace", "Máquina", "Tear", 3)) &&
         escriba.grava(faz("Ada Lovelace", "Máquina", "Nota G", 7)) &&
         escriba.grava(faz("Ada Lovelace", "Notas", "Traducção", 1)) &&
         escriba.grava(faz("Bach", "Cravo", "Fuga", 2)) &&
         escriba.conclui();
}

std::vector<std::string> textos(const tui::Navegador& navegador) {
  std::vector<std::string> fóra;
  for (const tui::Linha& linha : navegador.vista()) fóra.push_back(linha.texto);
  return fóra;
}

}  // namespace

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
