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
#include <cstring>
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
  std::string dentro(const std::string& nome) const { return caminho_ + "/" + nome; }

 private:
  std::string caminho_;
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
}  // namespace

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
