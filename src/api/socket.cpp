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

#include <algorithm>
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

// A COLHEITA. Duas cousas que o mundo faz e que um dublê ingenuo não faria, e que
// são o caso NORMAL de um socket de FLUXO e não a excepção: a linha chega PARTIDA
// em varias leituras, e VARIAS linhas chegam numa leitura só. Donde se acumula por
// cliente até o \n, e se drena em LAÇO, e não uma linha e pronto.
void Servidor::colhe(Cliente& cliente) {
  if (cliente.fd < 0) return;
  char balde[4096];
  // O MEIO FECHAMENTO é o que o «nc» faz, e o que qualquer cliente de canalisação
  // faz: ao ver o fim do seu stdin elle fecha a ESCRIPTA e SEGUE a ler. Encerrar
  // aqui sem responder deixaria o aceite d'esta issue a falhar por defeito nosso, e
  // a culpa cahiria na ferramenta. Donde se marca o fim, se drena o que chegou, se
  // responde, e só depois se fecha.
  bool fim_da_entrada = false;
  for (;;) {
    const ::ssize_t lidos = ::recv(cliente.fd, balde, sizeof(balde), 0);
    if (lidos == 0) { fim_da_entrada = true; break; }
    if (lidos < 0) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK) break;
      encerra(cliente);
      return;
    }
    cliente.entrada.append(balde, static_cast<std::size_t>(lidos));
    // O teto conta a linha SEM o \n: acumulado que já tenha \n é mensagem pronta,
    // e não cliente mudo a crescer memoria.
    if (cliente.entrada.size() > kTetoDaLinha &&
        cliente.entrada.find('\n') == std::string::npos) {
      cliente.sahida +=
          "{\"ok\":false,\"erro\":\"linha_longa\",\"razao\":\"a linha passou de 64 "
          "KiB sem terminar em \\n\"}\n";
      escoa(cliente);
      encerra(cliente);
      return;
    }
  }
  for (std::size_t corte; (corte = cliente.entrada.find('\n')) != std::string::npos;) {
    const std::string linha = cliente.entrada.substr(0, corte);
    cliente.entrada.erase(0, corte + 1);
    const std::string resposta = responde(*tocador_, linha);
    // Resposta vazia é a linha em branco, e a ella nada se manda: nem uma linha
    // vazia, que o cliente leria como mensagem.
    if (resposta.empty()) continue;
    cliente.sahida += resposta;
    cliente.sahida += '\n';
  }
  escoa(cliente);
  // Fecha-se sómente depois de a resposta ter sahido por inteiro. Ficando resto por
  // escrever, a batida seguinte o escoa; e se o outro lado tiver fechado de todo, é
  // o EPIPE do send que encerra, calado pelo MSG_NOSIGNAL.
  if (fim_da_entrada && cliente.sahida.empty()) encerra(cliente);
}

void Servidor::aceita() {
  for (;;) {
    const int novo = ::accept4(escuta_, nullptr, nullptr,
                               SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (novo < 0) return;  // EAGAIN é o caso normal: não ha mais ninguem á porta
    if (clientes_.size() >= kTetoDeClientes) {
      // LOTADO não se cala. Responde-se e fecha-se, para que o cliente saiba por
      // que, em vez de ver a porta fechar sem palavra e culpar a rede.
      static const char kLotado[] =
          "{\"ok\":false,\"erro\":\"lotado\",\"razao\":\"ha clientes demais ao mesmo "
          "tempo; tente outra vez\"}\n";
      ::send(novo, kLotado, sizeof(kLotado) - 1, MSG_NOSIGNAL);
      ::close(novo);
      continue;
    }
    Cliente cliente;
    cliente.fd = novo;
    clientes_.push_back(std::move(cliente));
  }
}

void Servidor::pulsa() {
  if (escuta_ < 0) return;
  // Aceita ANTES do poll, e não depois: quem chegou nesta batida é servido nesta
  // batida, e não na seguinte. E aceitar antes é o que deixa as referencias do
  // laço abaixo validas, que o push_back pode remanejar o vector.
  aceita();

  std::vector<::pollfd> olhos;
  olhos.reserve(clientes_.size());
  for (const Cliente& cliente : clientes_) {
    ::pollfd olho{};
    olho.fd = cliente.fd;
    olho.events = static_cast<short>(POLLIN | (cliente.sahida.empty() ? 0 : POLLOUT));
    olhos.push_back(olho);
  }
  // Espera ZERO: batida alguma se bloqueia, e cliente mudo não trava os outros nem
  // o tocador. É o que permitte bater isto na mesma linha que Tocador::pulsa().
  if (!olhos.empty() && ::poll(olhos.data(), olhos.size(), 0) < 0) return;

  for (std::size_t i = 0; i < olhos.size(); ++i) {
    if ((olhos[i].revents & POLLOUT) != 0) escoa(clientes_[i]);
    if ((olhos[i].revents & (POLLIN | POLLHUP | POLLERR)) != 0) colhe(clientes_[i]);
  }
  clientes_.erase(std::remove_if(clientes_.begin(), clientes_.end(),
                                 [](const Cliente& cliente) { return cliente.fd < 0; }),
                  clientes_.end());
}

}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
