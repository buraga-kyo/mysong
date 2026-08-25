// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO PROJECTOR — testes/prova_video.cpp
// ══════════════════════════════════════════════════════════════════════════
// Janella alguma se abre nesta bateria: o que se afere é o que se HA DE correr e o
// que se HA DE mandar. A janella de verdade prova-se á mão, com `pgrep` e `xprop`. E
// dous casos erguem processo, sem abrirem video: medem o processo, não o quadro.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>  // o codigo de erro do remove_all da cova
#include <vector>

#include "nucleo/video.hpp"

namespace nu = mysong::nucleo;

namespace {

class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-video-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  const std::filesystem::path& raiz() const { return caminho_; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

TEST_CASE("o juizo do video é pela extensão, e a lista é fechada") {
  CHECK(nu::tem_video("/a/b/filme.mkv"));
  CHECK(nu::tem_video("/a/b/filme.mp4"));
  CHECK(nu::tem_video("/a/b/filme.webm"));
  // A CAIXA não importa: disco tras nome escripto de qualquer modo.
  CHECK(nu::tem_video("/a/b/FILME.MKV"));
  CHECK(nu::tem_video("/a/b/Filme.Mp4"));
  // Audio puro NÃO abre janella. É esta a assertiva que impede o operador de ver
  // janella preta a tocar um mp3.
  CHECK_FALSE(nu::tem_video("/a/b/faixa.mp3"));
  CHECK_FALSE(nu::tem_video("/a/b/faixa.flac"));
  CHECK_FALSE(nu::tem_video("/a/b/faixa.opus"));
  CHECK_FALSE(nu::tem_video("/a/b/faixa.ogg"));
  // Sem extensão alguma tambem não: lista fechada quer dizer fechada.
  CHECK_FALSE(nu::tem_video("/a/b/faixa"));
  CHECK_FALSE(nu::tem_video(""));
}

TEST_CASE("o soquete leva o pid no nome, e mora na raiz que se disse") {
  const std::filesystem::path posto =
      nu::caminho_do_soquete("/run/user/1000", 4242);
  CHECK(posto.parent_path() == "/run/user/1000");
  // O PID no nome é o que impede duas corridas do mysong de disputarem o mesmo
  // soquete: a segunda ligaria á janella da primeira.
  CHECK(posto.filename() == "mysong-video-4242.sock");
  CHECK(nu::caminho_do_soquete("/run/user/1000", 1).filename() !=
        posto.filename());
}

TEST_CASE("os argumentos declaram a classe da janella nas duas fórmas") {
  const std::vector<std::string> ditos =
      nu::argumentos_do_projector("/acervo/A/B/01 - Tal.mkv", "/run/s.sock");
  const auto tem = [&ditos](const std::string& qual) {
    return std::find(ditos.begin(), ditos.end(), qual) != ditos.end();
  };
  // As DUAS fórmas: o X11 lê uma e o Wayland a outra, e quem corre não sabe qual
  // das duas o systema d'elle usa. É por este nome que o RADICAL-OS a governa.
  CHECK(tem("--x11-name=mysong-video"));
  CHECK(tem("--wayland-app-id=mysong-video"));
  CHECK(tem("--input-ipc-server=/run/s.sock"));
  // SEM terminal: elle é nosso, e a TUI está a pintar n'elle.
  CHECK(tem("--no-terminal"));
  CHECK(tem("--force-window=yes"));
  // O `--` fecha as opções, e a faixa vae DEPOIS d'elle: sem isso, faixa chamada
  // `--version` viraria opção do mpv.
  CHECK(ditos[ditos.size() - 2] == "--");
  CHECK(ditos.back() == "/acervo/A/B/01 - Tal.mkv");
  CHECK(ditos.front() == "mpv");
}

TEST_CASE("o escape do JSON cobre a aspa, a barra e o controle") {
  CHECK(nu::escapa_json("simples") == "simples");
  CHECK(nu::escapa_json("com \"aspa\"") == "com \\\"aspa\\\"");
  CHECK(nu::escapa_json("com \\ barra") == "com \\\\ barra");
  // Byte de controle vae na fórma longa: o JSON não o admitte crú, e nome de
  // arquivo com tabulação existe no disco.
  CHECK(nu::escapa_json("com\ttab") == "com\\u0009tab");
  CHECK(nu::escapa_json("com\nlinha") == "com\\u000alinha");
}

TEST_CASE("as ordens sahem em JSON de uma linha, com o valor na fórma certa") {
  // A bandeira sahe SEM aspas: o mpv recusa «"true"» por texto onde espera
  // booleano, e a fita ficava a tocar quando se pedia pausa.
  CHECK(nu::ordem_de_bandeira("pause", true) ==
        "{\"command\":[\"set_property\",\"pause\",true]}\n");
  CHECK(nu::ordem_de_bandeira("pause", false) ==
        "{\"command\":[\"set_property\",\"pause\",false]}\n");
  CHECK(nu::ordem_de_numero("volume", 30) ==
        "{\"command\":[\"set_property\",\"volume\",30.000]}\n");
  // A busca é ABSOLUTA: quem chama sabe onde quer estar, e relativa faria duas
  // ordens seguidas somarem-se de modo que a tela não previa.
  CHECK(nu::ordem_de_busca(12.5) == "{\"command\":[\"seek\",12.500,\"absolute\"]}\n");
  CHECK(nu::ordem_simples("quit") == "{\"command\":[\"quit\"]}\n");
  // Cada ordem acaba em UMA quebra de linha: o soquete do mpv lê por linha, e
  // duas ordens sem quebra entre ellas seriam uma linha que elle recusa.
  for (const std::string& ordem : {nu::ordem_simples("quit"),
                                   nu::ordem_de_bandeira("pause", true),
                                   nu::ordem_de_numero("volume", 1),
                                   nu::ordem_de_busca(0)}) {
    REQUIRE_FALSE(ordem.empty());
    CHECK(ordem.back() == '\n');
    CHECK(std::count(ordem.begin(), ordem.end(), '\n') == 1);
  }
}

TEST_CASE("todo desfecho da fita tem nome, e nenhum é o do vizinho") {
  const nu::Fita todos[] = {nu::Fita::Rodando, nu::Fita::SemMpv,
                            nu::Fita::SemVideo, nu::Fita::SemSoquete,
                            nu::Fita::NaoAbriu};
  std::vector<std::string> ditos;
  for (const nu::Fita fita : todos) {
    const std::string razao(nu::razao_da_fita(fita));
    CHECK_FALSE(razao.empty());
    CHECK(razao != "desfecho sem nome");
    ditos.push_back(razao);
  }
  std::sort(ditos.begin(), ditos.end());
  CHECK(std::unique(ditos.begin(), ditos.end()) == ditos.end());
}

TEST_CASE("audio puro não abre fita alguma, e o projector fica quieto") {
  Cova cova;
  nu::Projector projector(cova.raiz());
  CHECK(projector.abre("/a/b/faixa.mp3") == nu::Fita::SemVideo);
  CHECK_FALSE(projector.rodando());
  CHECK(projector.faixa().empty());
  // Ordem a projector quieto devolve falso, e não estoura.
  CHECK_FALSE(projector.pausar());
  CHECK_FALSE(projector.retomar());
  CHECK_FALSE(projector.buscar(10.0));
  CHECK_FALSE(projector.volume(50));
  // E soquete algum ficou no disco.
  std::error_code erro;
  CHECK(std::filesystem::is_empty(cova.raiz(), erro));
}

TEST_CASE("falta o mpv no caminho: a fita diz o nome d'essa falta") {
  Cova cova;
  // O caminho de busca fica com um só directorio, e elle está vazio: o `execvp`
  // não acha o mpv, e o filho sahe com o codigo que o pae distingue. É a unica
  // injecção de falha que esta bateria faz, e ella não abre janella alguma.
  const char* antes = ::getenv("PATH");
  const std::string guardado = antes == nullptr ? std::string() : antes;
  const std::filesystem::path vazio = cova.raiz() / "sem-nada";
  std::filesystem::create_directories(vazio);
  REQUIRE(::setenv("PATH", vazio.c_str(), 1) == 0);

  {
    nu::Projector projector(cova.raiz());
    const nu::Fita fita = projector.abre("/a/b/filme.mkv");
    CHECK(fita == nu::Fita::SemMpv);
    CHECK(nu::razao_da_fita(fita).find("mpv") != std::string_view::npos);
    CHECK_FALSE(projector.rodando());
  }

  if (guardado.empty()) ::unsetenv("PATH");
  else ::setenv("PATH", guardado.c_str(), 1);
  // O soquete que se ia usar não ficou no disco: fechar limpa-o.
  CHECK_FALSE(std::filesystem::exists(
      nu::caminho_do_soquete(cova.raiz(), static_cast<long>(::getpid()))));
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
