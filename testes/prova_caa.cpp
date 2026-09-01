// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO COVER ART ARCHIVE — testes/prova_caa.cpp
// ══════════════════════════════════════════════════════════════════════════
// SEM REDE, do principio ao fim: a Consulta entra de mentira, os corpos são
// recortes escriptos á mão sobre a fórma viva, e as faixas são fixtures
// lavradas em directorio temporario. A UNICA prova viva da issue corre fóra
// da bateria, pelo espia_capa, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <string>

#include "nucleo/caa.hpp"

namespace nu = mysong::nucleo;

namespace {

// A COVA de cada caso, no molde da prova do rol: prova alguma toca banco alheio.
class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-caa-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro; std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  std::filesystem::path banco() const { return caminho_ / "capas.sqlite3"; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

// Nome SEM virgula nem ponto-e-virgula: a virgula quebra o -tc do doctest, e o
// ponto-e-virgula é separador de LISTA do CMake, que partiria a entrada do
// ctest em duas fantasmas que filtram nada e passam vazias. Foi medido aqui.
TEST_CASE("a URL da capa sahe pela release escapada e vazia sem MBID") {
  CHECK(nu::url_da_capa("bc9051ea-9d77-3ae3-8bd6-45960a8c0e4f") ==
        "https://coverartarchive.org/release/"
        "bc9051ea-9d77-3ae3-8bd6-45960a8c0e4f/front-500");
  // MBID não se interpola cru: byte fóra da taboa sahe por cento e hexa. Não
  // é caso do vivo, e é justamente por isso que a guarda se prova aqui.
  CHECK(nu::url_da_capa("a b") ==
        "https://coverartarchive.org/release/a%20b/front-500");
  CHECK(nu::url_da_capa("").empty());
}

TEST_CASE("a taboa dos desfechos do CAA reparte o Falhou pelo estado") {
  using D = nu::DesfechoDaCapa;
  // O que o MB já nomeou passa tal e qual.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Achado, 200) == D::Achada);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Recuo, 503) == D::Recuo);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Recuo, 429) == D::Recuo);
  // O 404 é o CAA a dizer «capa não ha»: definitivo, lembra-se.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 404) == D::SemCapa);
  // Rede muda (estado zero) e 5xx sem recuo respondem amanhã: não se lembram.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 0) == D::Transitoria);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 500) == D::Transitoria);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 403) == D::Transitoria);
}

TEST_CASE("a memoria das procuradas escreve reabre e relê") {
  const Cova cova;
  {
    nu::MemoriaDeCapas memoria(cova.banco());
    REQUIRE(memoria.aberta());
    CHECK(memoria.versao() == nu::kVersaoDaMemoria);
    CHECK(memoria.lembra("/a/faixa.mp3", nu::Procurada::SemCapa));
    CHECK(memoria.lembra("/b/outra.mp3", nu::Procurada::Duvidosa));
    CHECK_FALSE(memoria.lembra("", nu::Procurada::SemCapa));  // sem identidade
  }
  // REABERTA de um punho novo, como a corrida de amanhã: o que poupa a rede
  // é o dado que persiste, e não o cache de um objecto vivo.
  nu::MemoriaDeCapas relida(cova.banco());
  REQUIRE(relida.aberta());
  CHECK(relida.ja_procurada("/a/faixa.mp3"));
  CHECK(relida.ja_procurada("/b/outra.mp3"));
  CHECK_FALSE(relida.ja_procurada("/c/terceira.mp3"));
  CHECK(relida.lembra("/a/faixa.mp3", nu::Procurada::Duvidosa));
  CHECK(relida.quantas() == 2);
}

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
