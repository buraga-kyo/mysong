// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DA API, BANDA VIVA — testes/prova_api_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// Aqui mora SÓ o que não se pode provar em machina surda: socket AF_UNIX de
// verdade, aberto no systema de arquivos, com cliente de verdade do outro lado. É
// pouco de proposito — os treze verbos e todo o enquadramento provam-se em
// prova_api.cpp —, e o que fica são as cinco cousas que só o transporte pode
// errar.
//
// DOMÍNIO ......... descriptors e arquivos no systema, que é estado GLOBAL.
// CONTRA-DOMÍNIO .. veredicto do doctest, e nenhum resto em disco.
// INVARIANTE ...... cada caso abre o seu socket num directorio temporario PROPRIO,
//                   feito por mkdtemp e desfeito por destructor. É o que impede
//                   dous casos de colidirem no mesmo caminho, e o que impede um
//                   caso de envenenar o seguinte com socket orphao. Sem isto, a
//                   ordem em que o doctest corre os casos passaria a importar, e
//                   bateria cuja ordem importa é bateria que falha em outra
//                   machina.
// Q.E.D. .......... o caminho do socket é INJECTAVEL na fabrica, e é essa unica
//                   decisão de desenho que torna esta bateria possivel: sem ella
//                   todo caso teria de disputar $XDG_RUNTIME_DIR/mysong.sock, e
//                   com o mysong do operador, se elle o tivesse aberto.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "api/socket.hpp"

namespace {

using mysong::api::Servidor;
using mysong::nucleo::Estado;

// O DIRECTORIO PROPRIO de cada caso. Nasce por mkdtemp, e morre no destructor com
// tudo o que houver dentro. É a metade da limpeza que a bateria deve, e é o que
// impede um caso de envenenar o seguinte.
class DirectorioTemporario {
 public:
  DirectorioTemporario() {
    char molde[] = "/tmp/mysong-prova-XXXXXX";
    const char* feito = ::mkdtemp(molde);
    if (feito != nullptr) caminho_ = feito;
  }
  ~DirectorioTemporario() {
    if (caminho_.empty()) return;
    // Arvore rasa por desenho: não ha recursão a escrever, e não ha directorio
    // dentro d'este senão o que esta bateria puser, que é socket e mais nada.
    ::DIR* porta = ::opendir(caminho_.c_str());
    if (porta != nullptr) {
      while (const ::dirent* entrada = ::readdir(porta)) {
        const std::string nome = entrada->d_name;
        if (nome == "." || nome == "..") continue;
        ::unlink((caminho_ + "/" + nome).c_str());
      }
      ::closedir(porta);
    }
    ::rmdir(caminho_.c_str());
  }
  DirectorioTemporario(const DirectorioTemporario&) = delete;
  DirectorioTemporario& operator=(const DirectorioTemporario&) = delete;

  bool valido() const { return !caminho_.empty(); }
  const std::string& raiz() const { return caminho_; }
  std::string dentro(const std::string& nome) const { return caminho_ + "/" + nome; }

 private:
  std::string caminho_;
};
// A GUARDA do ambiente. O caminho de fabrica sahe de $XDG_RUNTIME_DIR, e prova
// que muta variavel de ambiente ha de a repor, senão o caso seguinte herda o que
// este poz. Repõe tambem o caso de a variavel não existir antes, que apagar o que
// não havia é cousa diversa de repor o que havia.
class Ambiente {
 public:
  Ambiente(std::string nome, const std::string& valor) : nome_(std::move(nome)) {
    const char* antigo = ::getenv(nome_.c_str());
    havia_ = antigo != nullptr;
    if (havia_) antigo_ = antigo;
    ::setenv(nome_.c_str(), valor.c_str(), 1);
  }
  ~Ambiente() {
    if (havia_) ::setenv(nome_.c_str(), antigo_.c_str(), 1);
    else ::unsetenv(nome_.c_str());
  }
  Ambiente(const Ambiente&) = delete;
  Ambiente& operator=(const Ambiente&) = delete;

 private:
  std::string nome_;
  std::string antigo_;
  bool havia_ = false;
};
// UM CLIENTE DE VERDADE, e não dublê: socket AF_UNIX, connect, send e recv. É por
// elle que se prova o que só o transporte pode errar.
class Cliente {
 public:
  explicit Cliente(const std::string& caminho) {
    fd_ = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd_ < 0) return;
    ::sockaddr_un endereco{};
    endereco.sun_family = AF_UNIX;
    std::memcpy(endereco.sun_path, caminho.c_str(), caminho.size());
    ligado_ = ::connect(fd_, reinterpret_cast<const ::sockaddr*>(&endereco),
                        sizeof(endereco)) == 0;
  }
  ~Cliente() { fecha(); }
  Cliente(const Cliente&) = delete;
  Cliente& operator=(const Cliente&) = delete;

  bool ligado() const { return ligado_; }
  void manda(const std::string& bytes) {
    ::send(fd_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
  }
  void fecha() {
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
  }

  // Bate o servidor e colhe, até haver as linhas que se esperam ou até se esgotar a
  // paciencia. Bater ENTRE as tentativas é o que faz d'esta prova prova: o servidor
  // não tem linha de execução propria, e é a bateria que lhe dá as batidas.
  std::vector<std::string> colhe(Servidor& servidor, std::size_t quantas) {
    std::vector<std::string> linhas;
    for (int volta = 0; volta < 200 && linhas.size() < quantas; ++volta) {
      servidor.pulsa();
      char balde[4096];
      const ::ssize_t lidos = ::recv(fd_, balde, sizeof(balde), 0);
      if (lidos > 0) acumulado_.append(balde, static_cast<std::size_t>(lidos));
      for (std::size_t corte; (corte = acumulado_.find('\n')) != std::string::npos;) {
        linhas.push_back(acumulado_.substr(0, corte));
        acumulado_.erase(0, corte + 1);
      }
    }
    return linhas;
  }

 private:
  int fd_ = -1;
  bool ligado_ = false;
  std::string acumulado_;
};
// Um motor mudo: aqui não se prova mechanica de nucleo alguma, e por isso o dublê
// é o menor que sirva. O que se prova é o TRANSPORTE.
class MotorMudo final : public mysong::nucleo::Motor {
 public:
  bool tocar(const std::string&) override { estado_ = Estado::Tocando; return true; }
  bool pausar() override { estado_ = Estado::Pausado; return true; }
  bool retomar() override { estado_ = Estado::Tocando; return true; }
  bool buscar(double) override { return true; }
  bool volume(int) override { return true; }
  double posicao() const override { return 0.0; }
  double duracao() const override { return 42.0; }
  Estado estado() const override { return estado_; }
  void bombear() override {}

 private:
  Estado estado_ = Estado::Parado;
};
}  // namespace

TEST_CASE("o socket abre em 0600, responde, e a linha chega PARTIDA byte a byte") {
  const DirectorioTemporario casa;
  REQUIRE(casa.valido());
  const std::string caminho = casa.dentro("mysong.sock");

  MotorMudo motor;
  mysong::nucleo::Tocador tocador(motor);
  std::string razao;
  auto servidor = Servidor::abrir(tocador, caminho, &razao);
  REQUIRE_MESSAGE(servidor.has_value(), razao);

  struct ::stat marca {};
  REQUIRE(::stat(caminho.c_str(), &marca) == 0);
  CHECK(S_ISSOCK(marca.st_mode));
  CHECK((marca.st_mode & 0777) == 0600);  // a proteccao inteira d'esta superficie

  Cliente cliente(caminho);
  REQUIRE(cliente.ligado());
  // BYTE A BYTE: o socket é de FLUXO e não de mensagem, e é este o caso normal que
  // um dublê ingenuo nunca produziria. Servidor que presumisse mensagem-por-leitura
  // morre aqui, e morre só aqui.
  const std::string pedido = "{\"verbo\":\"estado\"}\n";
  for (const char letra : pedido) {
    cliente.manda(std::string(1, letra));
    servidor->pulsa();
  }
  const std::vector<std::string> linhas = cliente.colhe(*servidor, 1);
  REQUIRE(linhas.size() == 1);
  CHECK(linhas[0].find("\"ok\":true") != std::string::npos);
  CHECK(linhas[0].find("\"duracao\":42.000") != std::string::npos);
}
TEST_CASE("duas mensagens numa leitura so produzem DUAS linhas, na ordem") {
  const DirectorioTemporario casa;
  REQUIRE(casa.valido());
  MotorMudo motor;
  mysong::nucleo::Tocador tocador(motor);
  std::string razao;
  auto servidor = Servidor::abrir(tocador, casa.dentro("mysong.sock"), &razao);
  REQUIRE_MESSAGE(servidor.has_value(), razao);

  Cliente cliente(casa.dentro("mysong.sock"));
  REQUIRE(cliente.ligado());
  // Num send SÓ. Servidor que respondesse uma linha por leitura calaria a segunda
  // mensagem, e o cliente ficaria a esperar resposta que nunca vem.
  cliente.manda("{\"verbo\":\"versao\"}\n{\"verbo\":\"estado\"}\n");
  const std::vector<std::string> linhas = cliente.colhe(*servidor, 2);
  REQUIRE(linhas.size() == 2);
  CHECK(linhas[0].find("\"protocolo\":1") != std::string::npos);
  CHECK(linhas[1].find("\"volume\":100") != std::string::npos);
  // E a linha em branco no meio não produz resposta alguma, nem linha vazia.
  cliente.manda("\n   \n{\"verbo\":\"versao\"}\n");
  const std::vector<std::string> depois = cliente.colhe(*servidor, 1);
  REQUIRE(depois.size() == 1);
  CHECK(depois[0].find("\"protocolo\":1") != std::string::npos);
}

// O caso que só existe por causa do MSG_NOSIGNAL. Sem elle, o send em descriptor cujo
// par se fechou ergue SIGPIPE, e é o PROCESSO DA PROVA que morre: o caso não passa
// por acidente, e o defeito não se esconde num CHECK que nunca correu.
TEST_CASE("cliente que fecha de todo nao derruba o servidor, e o seguinte e servido") {
  const DirectorioTemporario casa;
  REQUIRE(casa.valido());
  const std::string caminho = casa.dentro("mysong.sock");
  MotorMudo motor;
  mysong::nucleo::Tocador tocador(motor);
  std::string razao;
  auto servidor = Servidor::abrir(tocador, caminho, &razao);
  REQUIRE_MESSAGE(servidor.has_value(), razao);

  {
    Cliente apressado(caminho);
    REQUIRE(apressado.ligado());
    apressado.manda("{\"verbo\":\"estado\"}\n");
    apressado.fecha();  // fecha ANTES de o servidor ter batido
  }
  for (int volta = 0; volta < 5; ++volta) servidor->pulsa();

  Cliente paciente(caminho);
  REQUIRE(paciente.ligado());
  paciente.manda("{\"verbo\":\"versao\"}\n");
  const std::vector<std::string> linhas = paciente.colhe(*servidor, 1);
  REQUIRE(linhas.size() == 1);
  CHECK(linhas[0].find("\"ok\":true") != std::string::npos);
}
// A differença entre ROBUSTEZ e ROUBO, em duas metades que se provam separadas.
TEST_CASE("o orphao reclama-se, e o socket VIVO respeita-se e nao se desliga") {
  const DirectorioTemporario casa;
  REQUIRE(casa.valido());
  const std::string caminho = casa.dentro("mysong.sock");
  MotorMudo motor;
  mysong::nucleo::Tocador tocador(motor);

  SUBCASE("orphao: ha arquivo e ninguem escuta, donde se reclama") {
    // Um socket ligado e ABANDONADO: o arquivo fica, e ninguem escuta. É
    // exactamente o resto que um processo morto de morte matada deixa.
    const int abandonado = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    REQUIRE(abandonado >= 0);
    ::sockaddr_un endereco{};
    endereco.sun_family = AF_UNIX;
    std::memcpy(endereco.sun_path, caminho.c_str(), caminho.size());
    REQUIRE(::bind(abandonado, reinterpret_cast<const ::sockaddr*>(&endereco),
                   sizeof(endereco)) == 0);
    ::close(abandonado);  // fecha SEM escutar: o arquivo fica orphao

    std::string razao;
    auto servidor = Servidor::abrir(tocador, caminho, &razao);
    CHECK_MESSAGE(servidor.has_value(), razao);
  }

  SUBCASE("vivo: ha quem escute, donde se recusa e o arquivo alheio fica") {
    std::string razao_do_primeiro;
    auto primeiro = Servidor::abrir(tocador, caminho, &razao_do_primeiro);
    REQUIRE_MESSAGE(primeiro.has_value(), razao_do_primeiro);

    std::string razao;
    auto segundo = Servidor::abrir(tocador, caminho, &razao);
    CHECK_FALSE(segundo.has_value());
    CHECK(razao.find("outra instancia") != std::string::npos);
    // E o arquivo do primeiro FICA: roubar o caminho faria as ordens do operador
    // passarem á instancia que elle abriu por ultimo, e a que elle ouve ficaria
    // surda sem aviso.
    struct ::stat marca {};
    CHECK(::stat(caminho.c_str(), &marca) == 0);
    // E o primeiro segue a servir de facto, e não sómente a possuir o arquivo.
    Cliente cliente(caminho);
    REQUIRE(cliente.ligado());
    cliente.manda("{\"verbo\":\"versao\"}\n");
    CHECK(cliente.colhe(*primeiro, 1).size() == 1);
  }
}
TEST_CASE("a limpeza corre por QUALQUER sahida, e a excepcao inclusa") {
  const DirectorioTemporario casa;
  REQUIRE(casa.valido());
  const std::string caminho = casa.dentro("mysong.sock");
  MotorMudo motor;
  mysong::nucleo::Tocador tocador(motor);

  // A pilha desenrola-se por excepção erguida DEPOIS de o socket abrir. Quem
  // desliga o arquivo é o destructor, e nenhum trecho de limpeza que se tenha de
  // lembrar: e é justamente por aqui que o trecho esquecido apparecia.
  try {
    auto servidor = Servidor::abrir(tocador, caminho, nullptr);
    REQUIRE(servidor.has_value());
    throw std::runtime_error("interrupcao de proposito");
  } catch (const std::runtime_error&) {
  }
  struct ::stat marca {};
  CHECK(::stat(caminho.c_str(), &marca) != 0);  // o arquivo já não existe

  // E o caso SEGUINTE abre no MESMO caminho sem tropeçar no que o anterior deixou.
  // Sem esta segunda metade, um destructor que não limpasse passaria sempre que o
  // doctest corresse este caso por ultimo.
  std::string razao;
  auto outra_vez = Servidor::abrir(tocador, caminho, &razao);
  CHECK_MESSAGE(outra_vez.has_value(), razao);
}

TEST_CASE("o caminho recusa-se por nome quando falta o XDG ou quando nao cabe") {
  MotorMudo motor;
  mysong::nucleo::Tocador tocador(motor);

  // Caminho vazio é o que caminho_padrao_do_socket devolve sem $XDG_RUNTIME_DIR.
  std::string razao;
  CHECK_FALSE(Servidor::abrir(tocador, "", &razao).has_value());
  CHECK(razao.find("XDG_RUNTIME_DIR") != std::string::npos);
  // E recuo algum a /tmp: recuar deixaria qualquer usuario da machina governar o
  // tocador alheio, ou criar o arquivo primeiro e passar a receber as ordens.
  CHECK(razao.find("/tmp") != std::string::npos);  // dito na razão, e não usado

  // O limite de sun_path recusa-se com o comprimento E o limite ditos, e nunca se
  // trunca em silêncio. Nesta machina o caminho real tem trinta bytes, donde este
  // caso nunca se daria por acidente: força-se de proposito.
  const std::string comprido = "/tmp/" + std::string(200, 'x') + ".sock";
  razao.clear();
  CHECK_FALSE(Servidor::abrir(tocador, comprido, &razao).has_value());
  CHECK(razao.find("108") != std::string::npos);
  CHECK(razao.find(std::to_string(comprido.size())) != std::string::npos);
}
// O CAMINHO DE FABRICA, ponta a ponta: é o que a issue #69 pede por escripto, e é
// por onde o operador entra de verdade. Sem este caso a bateria provaria o
// transporte n'um caminho injectado, e nunca aquelle que a janella usa.
TEST_CASE("o socket sobe no caminho de fabrica e some quando o programa fecha") {
  const DirectorioTemporario casa;
  REQUIRE(casa.valido());
  const Ambiente posto("XDG_RUNTIME_DIR", casa.raiz());
  const std::string caminho = mysong::api::caminho_padrao_do_socket();
  REQUIRE(caminho == casa.dentro("mysong.sock"));

  MotorMudo motor;
  mysong::nucleo::Tocador tocador(motor);
  {
    std::string razao;
    auto servidor = Servidor::abrir(tocador, caminho, &razao);
    REQUIRE_MESSAGE(servidor.has_value(), razao);
    Cliente cliente(caminho);
    REQUIRE(cliente.ligado());
    cliente.manda("{\"verbo\":\"versao\"}\n");
    const std::vector<std::string> linhas = cliente.colhe(*servidor, 1);
    REQUIRE(linhas.size() == 1);
    CHECK(linhas[0].find("\"ok\":true") != std::string::npos);
    CHECK(linhas[0].find("\"obra\":\"mysong\"") != std::string::npos);
  }
  // Fechado o programa, o arquivo sahe do disco: é a segunda metade do aceite, e
  // é o que impede o orphao de envenenar a corrida seguinte.
  struct ::stat marca {};
  CHECK(::stat(caminho.c_str(), &marca) != 0);
}

// O DITO do diagnostico nos TRES estados que o operador pode encontrar. Sem este
// caso, a linha do --sonda seria a unica parte d'esta obra que só o olho afere.
TEST_CASE("o dito do socket diz o caminho e quem escute n'elle") {
  MotorMudo motor;
  mysong::nucleo::Tocador tocador(motor);

  SUBCASE("sem a variavel: diz que caminho nao ha e nomeia a variavel") {
    // Vazia e ausente entram pelo mesmo galho da fabrica, e a guarda repõe.
    const Ambiente posto("XDG_RUNTIME_DIR", "");
    const std::string dito = mysong::api::texto_do_socket();
    CHECK(dito.find("XDG_RUNTIME_DIR") != std::string::npos);
    CHECK(dito.find("nao ha") != std::string::npos);
  }

  SUBCASE("com a variavel e sem servidor: diz o caminho e que ninguem escuta") {
    const DirectorioTemporario casa;
    REQUIRE(casa.valido());
    const Ambiente posto("XDG_RUNTIME_DIR", casa.raiz());
    const std::string dito = mysong::api::texto_do_socket();
    CHECK(dito.find(casa.dentro("mysong.sock")) != std::string::npos);
    CHECK(dito.find("ninguem escuta") != std::string::npos);
  }

  SUBCASE("com o servidor de pe: diz que ha quem escute") {
    const DirectorioTemporario casa;
    REQUIRE(casa.valido());
    const Ambiente posto("XDG_RUNTIME_DIR", casa.raiz());
    std::string razao;
    auto servidor = Servidor::abrir(tocador, casa.dentro("mysong.sock"), &razao);
    REQUIRE_MESSAGE(servidor.has_value(), razao);
    CHECK(mysong::api::texto_do_socket().find("ha quem escute") !=
          std::string::npos);
  }
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
