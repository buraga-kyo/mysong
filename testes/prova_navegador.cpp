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

// O CAMINHO DO ACEITE, de ponta a ponta: de Artistas a um artista, d'elle a um
// album, e d'alli a uma faixa que se manda tocar. Cada degrau afere-se contra a
// taboa escripta no arnês.
TEST_CASE("de artistas a uma faixa, o caminho inteiro do aceite") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);

  CHECK(navegador.secao() == tui::Secao::Artistas);
  CHECK(textos(navegador) == std::vector<std::string>{"Ada Lovelace", "Bach"});
  CHECK(navegador.trilha().empty());
  CHECK(navegador.eleito() == 0u);
  // Artista não tem caminho: pedi-lo aqui devolve vazio de proposito.
  CHECK(navegador.caminho_eleito().empty());

  CHECK_FALSE(navegador.entra());  // entrou n'um artista, e não n'uma faixa
  CHECK(navegador.secao() == tui::Secao::Albuns);
  CHECK(navegador.trilha() == std::vector<std::string>{"Ada Lovelace"});
  CHECK(textos(navegador) == std::vector<std::string>{"Máquina", "Notas"});

  CHECK_FALSE(navegador.entra());  // entrou n'um album
  CHECK(navegador.secao() == tui::Secao::Faixas);
  CHECK(navegador.trilha() ==
        std::vector<std::string>{"Ada Lovelace", "Máquina"});
  // Ordem de NUMERO: gravou-se Tear com tres e Nota G com sete.
  CHECK(textos(navegador) == std::vector<std::string>{"Tear", "Nota G"});
  CHECK(navegador.vista()[0].numero == 3);
  CHECK(navegador.vista()[0].duracao == 103);
  CHECK(navegador.vista()[0].autor == "Ada Lovelace");

  CHECK(navegador.entra());  // AGORA é faixa: quem chama manda tocar
  CHECK(navegador.caminho_eleito() ==
        "/acervo/Ada Lovelace/Máquina/Tear.mp3");
  // E entrar n'uma faixa não muda a secção: continua-se onde se estava.
  CHECK(navegador.secao() == tui::Secao::Faixas);
}

// A LISTA NÃO DÁ A VOLTA. Descer no ultimo fica no ultimo, e subir no primeiro
// fica no primeiro: dar a volta n'uma lista de mil artistas faria o operador
// perder o logar sem saber como.
TEST_CASE("a lista não dá a volta nas duas pontas") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  REQUIRE(navegador.vista().size() == 2u);

  CHECK(navegador.eleito() == 0u);
  navegador.sobe();
  CHECK(navegador.eleito() == 0u);  // no primeiro, subir não passa
  navegador.desce();
  CHECK(navegador.eleito() == 1u);
  navegador.desce();
  CHECK(navegador.eleito() == 1u);  // no ultimo, descer não passa
  navegador.ao_principio();
  CHECK(navegador.eleito() == 0u);
  navegador.ao_fim();
  CHECK(navegador.eleito() == 1u);
}

// Voltar sobe UM degrau, e no alto não faz nada.
TEST_CASE("voltar sobe um degrau, e no alto devolve falso") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  navegador.entra();  // Albuns de Ada Lovelace
  navegador.entra();  // Faixas de Máquina
  REQUIRE(navegador.secao() == tui::Secao::Faixas);

  CHECK(navegador.volta());
  CHECK(navegador.secao() == tui::Secao::Albuns);
  CHECK(navegador.trilha() == std::vector<std::string>{"Ada Lovelace"});
  CHECK(navegador.volta());
  CHECK(navegador.secao() == tui::Secao::Artistas);
  CHECK(navegador.trilha().empty());
  CHECK_FALSE(navegador.volta());  // no alto, nada
  CHECK(navegador.secao() == tui::Secao::Artistas);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
