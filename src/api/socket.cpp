// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO SOCKET, LAVRA — src/api/socket.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabecalho. É a UNICA unidade da superfície que nomeia AF_UNIX, poll e
// descriptor; o protocolo, que é o cerebro, nada d'isso conhece, e é d'essa
// repartição que vem a bateria em machina surda.
//
// DOMÍNIO ......... descriptors, e o que o kernel disser d'elles.
// CONTRA-DOMÍNIO .. linhas que entram, linhas que sahem, e um arquivo de socket
//                   que existe enquanto o Servidor existe, e não mais.
// INVARIANTE ...... o errno guarda-se em variavel local ANTES de qualquer close
//                   ou umask, porque fechar descriptor pode sobrescrever o errno
//                   que se queria relatar, e razão composta do errno de outra
//                   chamada é razão que manda depurar no logar errado.
// Q.E.D. .......... nenhuma chamada d'esta unidade bloqueia. O poll espera ZERO, o
//                   socket nasce O_NONBLOCK, e o accept vem por accept4 com a
//                   mesma marca; logo uma batida custa o que o kernel tiver
//                   pronto, e cliente mudo não trava nem o tocador nem os outros.
// ══════════════════════════════════════════════════════════════════════════
#include "api/socket.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "api/protocolo.hpp"

namespace mysong::api {

std::string caminho_padrao_do_socket() {
  const char* raiz = std::getenv("XDG_RUNTIME_DIR");
  if (raiz == nullptr || *raiz == '\0') return {};
  return std::string(raiz) + "/mysong.sock";
}

namespace {

// O limite de sun_path, lido da propria estructura e não chumbado: são 108 bytes
// nesta plataforma, e o terminador é um d'elles.
constexpr std::size_t kSunPath = sizeof(::sockaddr_un::sun_path);

// CABE? Truncar em silêncio faria o socket nascer em caminho que não é o que se
// pediu, e o cliente iria bater á porta errada sem que ninguem lhe dissesse.
bool cabe_no_sun_path(const std::string& caminho, std::string* razao) {
  if (caminho.size() + 1 <= kSunPath) return true;
  if (razao != nullptr)
    *razao = "o caminho do socket tem " + std::to_string(caminho.size()) +
             " bytes, e o limite de sun_path e " + std::to_string(kSunPath) +
             " contado o terminador, donde cabem " + std::to_string(kSunPath - 1) +
             ": " + caminho;
  return false;
}

// Assenta o endereço. Presume que já se verificou que cabe.
void assenta_endereco(::sockaddr_un* endereco, const std::string& caminho) {
  *endereco = ::sockaddr_un{};
  endereco->sun_family = AF_UNIX;
  std::memcpy(endereco->sun_path, caminho.c_str(), caminho.size());
}

// HA QUEM ESCUTE? O socket é a propria prova de vida: tenta-se connectar, e quem
// responde está vivo. PID algum se consulta e arquivo de tranca algum se escreve,
// que ambos mentem quando o processo morre de morte matada. Na duvida (nem se pudo
// abrir a sonda) responde-se SIM, que é o lado seguro: não se desliga o alheio.
bool ha_quem_escute(const std::string& caminho) {
  const int sonda = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (sonda < 0) return true;
  ::sockaddr_un endereco{};
  assenta_endereco(&endereco, caminho);
  const int veredicto = ::connect(
      sonda, reinterpret_cast<const ::sockaddr*>(&endereco), sizeof(endereco));
  const int guardado = errno;
  ::close(sonda);
  errno = guardado;
  return veredicto == 0;
}

}  // namespace

std::optional<Servidor> Servidor::abrir(nucleo::Tocador& tocador,
                                        const std::string& caminho,
                                        std::string* razao) {
  const auto recusa = [razao](std::string dito) {
    if (razao != nullptr) *razao = std::move(dito);
    return std::optional<Servidor>{};
  };

  if (caminho.empty())
    return recusa(
        "caminho de socket vazio: XDG_RUNTIME_DIR nao esta definido, e esta Casa "
        "NAO recua a /tmp, que e escripta de todos");
  if (!cabe_no_sun_path(caminho, razao)) return std::optional<Servidor>{};

  // Ha arquivo no caminho? Orphao de processo morto reclama-se; socket VIVO
  // respeita-se, e o alheio nao se desliga. E a differenca entre robustez e roubo.
  struct ::stat marca {};
  if (::stat(caminho.c_str(), &marca) == 0) {
    if (ha_quem_escute(caminho))
      return recusa("outra instancia do mysong ja serve em " + caminho +
                    "; esta NAO lhe rouba o socket");
    if (::unlink(caminho.c_str()) != 0)
      return recusa("ha socket orphao em " + caminho +
                    " e nao se pudo desligar: " + std::strerror(errno));
  }

  const int escuta = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (escuta < 0)
    return recusa(std::string("nao se pudo abrir o socket: ") + std::strerror(errno));

  ::sockaddr_un endereco{};
  assenta_endereco(&endereco, caminho);
  // O umask é estado GLOBAL do processo: assenta-se, cria-se, e restaura-se no
  // mesmo escopo, para que arquivo algum que o programa crie depois herde isto. O
  // 0177 deixa o socket em 0600, que com o $XDG_RUNTIME_DIR a 0700 é a protecção
  // inteira d'esta superfície: não ha senha, e a porta é a do systema de arquivos.
  const ::mode_t antiga = ::umask(0177);
  const int ligou = ::bind(
      escuta, reinterpret_cast<const ::sockaddr*>(&endereco), sizeof(endereco));
  const int erro_do_bind = errno;
  ::umask(antiga);
  if (ligou != 0) {
    ::close(escuta);
    return recusa("nao se pudo ligar " + caminho + ": " +
                  std::strerror(erro_do_bind));
  }
  if (::listen(escuta, 8) != 0) {
    const int erro_da_escuta = errno;
    ::close(escuta);
    ::unlink(caminho.c_str());
    return recusa("nao se pudo escutar em " + caminho + ": " +
                  std::strerror(erro_da_escuta));
  }
  return Servidor(tocador, escuta, caminho);
}

Servidor::Servidor(nucleo::Tocador& tocador, int escuta, std::string caminho) noexcept
    : tocador_(&tocador), escuta_(escuta), caminho_(std::move(caminho)) {}

// O MOVE esvazia a fonte, e é por isso que elle é seguro: o destructor da fonte
// corre de todo modo, e sem esvaziar ella desligaria o arquivo que o destino
// acabou de herdar.
Servidor::Servidor(Servidor&& outro) noexcept
    : tocador_(outro.tocador_),
      escuta_(outro.escuta_),
      caminho_(std::move(outro.caminho_)),
      clientes_(std::move(outro.clientes_)) {
  outro.escuta_ = -1;
  outro.caminho_.clear();
  outro.clientes_.clear();
}

// A LIMPEZA, por qualquer caminho de sahida e excepção inclusa. Quem desliga o
// arquivo é este destructor, e não um trecho que cada retorno tenha de lembrar:
// trecho que se lembra é trecho que um dia se esquece, e socket orphao em disco
// envenena a instancia seguinte. Sendo destructor, a pilha que se desenrola por
// excepção o corre do mesmo modo.
Servidor::~Servidor() {
  for (Cliente& cliente : clientes_)
    if (cliente.fd >= 0) ::close(cliente.fd);
  clientes_.clear();
  if (escuta_ >= 0) ::close(escuta_);
  escuta_ = -1;
  if (!caminho_.empty()) ::unlink(caminho_.c_str());
  caminho_.clear();
}

const std::string& Servidor::caminho() const noexcept { return caminho_; }

void Servidor::encerra(Cliente& cliente) noexcept {
  if (cliente.fd >= 0) ::close(cliente.fd);
  // Marca-se, e NÃO se apaga do vector aqui: apagar invalidaria a referencia de
  // quem nos chamou, que ainda está a correr sobre ella. Quem recolhe é o pulsa().
  cliente.fd = -1;
  cliente.entrada.clear();
  cliente.sahida.clear();
}

// A ESCRIPTA. MSG_NOSIGNAL, e nunca SIGPIPE: cliente que fecha no meio de uma
// resposta é ROTINA, e rotina não derruba processo. Sem esta marca, um «nc» que
// sahisse antes de ler mataria o tocador inteiro.
void Servidor::escoa(Cliente& cliente) {
  while (cliente.fd >= 0 && !cliente.sahida.empty()) {
    const ::ssize_t postos = ::send(cliente.fd, cliente.sahida.data(),
                                    cliente.sahida.size(), MSG_NOSIGNAL);
    if (postos < 0) {
      if (errno == EINTR) continue;
      // Cheio por ora: o resto vae na batida seguinte, e não se perde.
      if (errno == EAGAIN || errno == EWOULDBLOCK) return;
      encerra(cliente);
      return;
    }
    // A escripta PARCIAL é o caso NORMAL de socket de fluxo, e não a excepção:
    // guarda-se o resto, e não se presume que uma resposta caiba numa chamada.
    cliente.sahida.erase(0, static_cast<std::size_t>(postos));
  }
}

}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
