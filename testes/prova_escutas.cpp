#include <doctest/doctest.h>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <thread>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include "nucleo/escutas.hpp"
#include "tui/escutas.hpp"

namespace nu = mysong::nucleo;
namespace {
struct Pasta {
  std::filesystem::path raiz;
  Pasta() {
    char nome[] = "/tmp/mysong-escutas-XXXXXX";
    const char* caminho = ::mkdtemp(nome);
    if (caminho) raiz = caminho;
  }
  ~Pasta() { std::error_code erro; if (!raiz.empty()) std::filesystem::remove_all(raiz, erro); }
  std::filesystem::path banco() const { return raiz / "escutas.sqlite3"; }
};
nu::Faixa metadados(const std::string& caminho) {
  nu::Faixa faixa;
  faixa.caminho = caminho;
  faixa.titulo = caminho == "/a.mp3" ? "Canção A" : "Canção B";
  faixa.artista = caminho == "/a.mp3" ? "Artista A" : "Artista B";
  return faixa;
}
struct Relogio {
  nu::EstatisticasDeEscuta::Relogio::time_point agora{};
  nu::EstatisticasDeEscuta::Calendario::time_point data =
      nu::EstatisticasDeEscuta::Calendario::from_time_t(1793401200);
  void anda(int ms) { agora += std::chrono::milliseconds(ms); data += std::chrono::milliseconds(ms); }
};
nu::Retracto tocando(const std::string& caminho = "/a.mp3", std::uint64_t ciclo = 1) {
  nu::Retracto r;
  r.faixa = std::make_shared<const std::string>(caminho);
  r.estado = nu::Estado::Tocando; r.duracao = 200; r.reproducao = ciclo;
  return r;
}
void ouve(nu::EstatisticasDeEscuta& historico, nu::Retracto& r, Relogio& relogio, int segundos) {
  for (int i = 0; i < segundos; ++i) {
    relogio.anda(1000); r.posicao += 1;
    historico.observa(r, relogio.agora, relogio.data);
  }
}
}

TEST_CASE("escutas persistem sem duplicar ao salvar e ordenam musicas e artistas por tempo") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  Relogio relogio;
  {
    nu::EstatisticasDeEscuta historico(pasta.banco(), metadados);
    auto r = tocando();
    historico.observa(r, relogio.agora, relogio.data);
    ouve(historico, r, relogio, 40);
    r = tocando("/b.mp3", 2);
    historico.observa(r, relogio.agora, relogio.data);
    ouve(historico, r, relogio, 65);
    for (int i = 0; i < 5; ++i) CHECK(historico.salva());
    const auto resumo = historico.resumo(0, "/b.mp3", relogio.data);
    CHECK(resumo.erro.empty()); CHECK(resumo.milissegundos == 105000);
    CHECK(resumo.reproducoes == 2); CHECK(resumo.musicas == 2); CHECK(resumo.artistas == 2);
    REQUIRE(resumo.mais_artistas.size() == 2);
    CHECK(resumo.mais_artistas[0].nome == "Artista B");
    CHECK(resumo.mais_musicas[0].nome == "Canção B");
    CHECK(resumo.atual.milissegundos == 65000);
  }
  nu::EstatisticasDeEscuta reaberto(pasta.banco(), metadados);
  CHECK(reaberto.resumo().milissegundos == 105000);
  CHECK(reaberto.resumo().reproducoes == 2);
}

TEST_CASE("pausas mudo saltos e suspensao nao inflam o tempo ouvido") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  nu::EstatisticasDeEscuta historico(pasta.banco(), metadados);
  Relogio relogio; auto r = tocando();
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 5);
  r.estado = nu::Estado::Pausado;
  relogio.anda(1000); historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 10);
  r.estado = nu::Estado::Tocando;
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 5);
  relogio.anda(1000); r.posicao += 100;
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 5);
  relogio.anda(1000); r.posicao = 0;
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 5);
  r.mudo = true; historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 10);
  r.mudo = false; historico.observa(r, relogio.agora, relogio.data);
  relogio.anda(300000); r.posicao += 300;
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 5);
  CHECK(historico.resumo().milissegundos == 25000);
  CHECK(historico.resumo().reproducoes == 0);
}

TEST_CASE("atualizacao lenta da posicao nao perde metade da escuta nem soma buffering") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  nu::EstatisticasDeEscuta historico(pasta.banco(), metadados);
  Relogio relogio; auto r = tocando();
  historico.observa(r, relogio.agora, relogio.data);
  for (int i = 0; i < 200; ++i) {
    relogio.anda(50);
    if (i % 2 == 1) r.posicao += 0.1;
    historico.observa(r, relogio.agora, relogio.data);
  }
  CHECK(historico.resumo().milissegundos == 10000);
  for (int i = 0; i < 10; ++i) { relogio.anda(1000); historico.observa(r, relogio.agora, relogio.data); }
  CHECK(historico.resumo().milissegundos == 10000);
  relogio.anda(100); r.posicao += .1;
  historico.observa(r, relogio.agora, relogio.data);
  CHECK(historico.resumo().milissegundos == 10100);
}

TEST_CASE("repetir a mesma musica conta nova reproducao sem duplicar ao pausar") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  nu::EstatisticasDeEscuta historico(pasta.banco(), metadados);
  Relogio relogio; auto r = tocando(); r.duracao = 10;
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 5);
  CHECK(historico.resumo().reproducoes == 1);
  r.estado = nu::Estado::Pausado; historico.observa(r, relogio.agora, relogio.data);
  relogio.anda(1000); r.estado = nu::Estado::Tocando;
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 5);
  CHECK(historico.resumo().reproducoes == 1);
  r.reproducao = 2; r.posicao = 0;
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 5);
  CHECK(historico.resumo().reproducoes == 2);
  CHECK(historico.resumo().milissegundos == 15000);
}

TEST_CASE("historico separa periodos sem depender do indice da biblioteca") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  nu::EstatisticasDeEscuta historico(pasta.banco(), metadados);
  Relogio relogio; auto r = tocando();
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 40);
  relogio.data += std::chrono::hours(24 * 10);
  r = tocando("/b.mp3", 2);
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 50);
  CHECK(historico.resumo(7, {}, relogio.data).milissegundos == 50000);
  CHECK(historico.resumo(30, {}, relogio.data).milissegundos == 90000);
  CHECK(historico.resumo(0, {}, relogio.data).milissegundos == 90000);
}

TEST_CASE("painel de escutas mostra ranking musica atual e atalhos sem transbordar") {
  nu::ResumoDasEscutas r;
  r.milissegundos = 125000; r.reproducoes = 2; r.musicas = 1; r.artistas = 1;
  r.atual = {"Canção", "Artista", 125000, 2};
  r.mais_musicas.push_back(r.atual);
  r.mais_artistas.push_back({"Artista", "Artista", 125000, 2});
  for (const int largura : {40, 80, 143}) {
    for (const int altura : {12, 30, 67}) {
      auto tela = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura), ftxui::Dimension::Fixed(altura));
      ftxui::Render(tela, mysong::tui::painel_de_escutas(r, 7, largura, altura));
      std::string texto;
      for (int y = 0; y < altura; ++y)
        for (int x = 0; x < largura; ++x) texto += tela.PixelAt(x, y).character;
      CHECK(texto.find("SUAS ESCUTAS") != std::string::npos);
      CHECK(texto.find("e/Esc") != std::string::npos);
    }
  }
}

TEST_CASE("artista agrega faixas diferentes e metadados nao sao relidos a cada quadro") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  int leituras = 0;
  nu::EstatisticasDeEscuta historico(pasta.banco(), [&](const std::string& caminho) {
    ++leituras;
    auto faixa = metadados(caminho); faixa.artista = "Artista d'Água"; return faixa;
  });
  Relogio relogio; auto r = tocando();
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 35);
  r = tocando("/b.mp3", 2);
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 40);
  const auto resumo = historico.resumo();
  CHECK(leituras == 2);
  CHECK(resumo.artistas == 1); CHECK(resumo.musicas == 2);
  REQUIRE(resumo.mais_artistas.size() == 1);
  CHECK(resumo.mais_artistas[0].nome == "Artista d'Água");
  CHECK(resumo.mais_artistas[0].milissegundos == 75000);
  CHECK(resumo.mais_artistas[0].reproducoes == 2);
}

TEST_CASE("escuta atravessando meia noite reparte tempo entre os dias") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  nu::EstatisticasDeEscuta historico(pasta.banco(), metadados);
  std::tm calendario{};
  calendario.tm_year = 126; calendario.tm_mon = 0; calendario.tm_mday = 2;
  calendario.tm_hour = 23; calendario.tm_min = 59; calendario.tm_sec = 59;
  calendario.tm_isdst = -1;
  Relogio relogio;
  relogio.data = nu::EstatisticasDeEscuta::Calendario::from_time_t(std::mktime(&calendario));
  auto r = tocando(); r.duracao = 2;
  historico.observa(r, relogio.agora, relogio.data);
  relogio.anda(2000); r.posicao = 2;
  historico.observa(r, relogio.agora, relogio.data);
  CHECK(historico.resumo(0, {}, relogio.data).milissegundos == 2000);
  CHECK(historico.resumo(1, {}, relogio.data).milissegundos == 1000);
  CHECK(historico.resumo(0, {}, relogio.data).reproducoes == 1);
}

TEST_CASE("gravacao automatica preserva progresso sem abrir a tela de estatisticas") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  nu::EstatisticasDeEscuta historico(pasta.banco(), metadados);
  Relogio relogio; auto r = tocando();
  historico.observa(r, relogio.agora, relogio.data);
  ouve(historico, r, relogio, 8);
  nu::EstatisticasDeEscuta leitor(pasta.banco(), metadados);
  CHECK(leitor.resumo().milissegundos == 5000);
  CHECK(historico.salva());
  CHECK(leitor.resumo().milissegundos == 8000);
}

TEST_CASE("consultas em paralelo nao duplicam os registros") {
  Pasta pasta; REQUIRE_FALSE(pasta.raiz.empty());
  nu::EstatisticasDeEscuta historico(pasta.banco(), metadados);
  std::thread tocador([&] {
    Relogio relogio; auto r = tocando();
    historico.observa(r, relogio.agora, relogio.data);
    ouve(historico, r, relogio, 60);
  });
  for (int i = 0; i < 10; ++i) {
    const auto r = historico.resumo();
    CHECK(r.erro.empty());
    CHECK(r.milissegundos <= 60000);
  }
  tocador.join();
  CHECK(historico.resumo().milissegundos == 60000);
  CHECK(historico.resumo().reproducoes == 1);
}
