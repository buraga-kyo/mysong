// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LOUSA — src/nucleo/lousa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o filho e o cano no fim.
//
// DOMÍNIO ......... o rectangulo em célullas, e o caminho de uma imagem.
// CONTRA-DOMÍNIO .. linhas de JSON, e um filho que morre com o tocador.
// INVARIANTE ...... funcção alguma d'aqui lança nem espera pelo filho.
// Q.E.D. .......... a composição do JSON não conhece o filho, d'onde a prova do
//                   protocolo corre sem X11 vivo.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/lousa.hpp"

#include "nucleo/aquisicao.hpp"  // corre(): o fork e o exec sem shell

#include <atomic>
#include <cerrno>
#include <cstdlib>

#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace mysong::nucleo {

namespace {

// O FILHO d'esta Casa, guardado FÓRA do objecto para que o `atexit` o alcance:
// o handler d'elle não toma argumento algum. Atomico porque a sahida pode vir
// de fio que não é o que ergueu a lousa. UM, e a regra é declarada: uma lousa
// por processo, que duas dariam duas janellas a disputar o mesmo canto.
std::atomic<int> o_filho{-1};

void mata_o_filho() {
  const int quem = o_filho.exchange(-1);
  if (quem > 0) ::kill(quem, SIGTERM);
}

// ergue — o fork com CANO na entrada do filho: devolve o pid, e menos um
// quando não vae. SOCKETPAR, e não `pipe`: escrevendo-se n'um cano cujo leitor
// morreu, o systema manda SIGPIPE e o padrão d'elle mata o processo; com o
// socket, o MSG_NOSIGNAL desliga isso SEM tocar no tratador global.
int ergue(int* cano) noexcept {
  int par[2] = {-1, -1};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, par) != 0) return -1;
  const ::pid_t filho = ::fork();
  if (filho < 0) { ::close(par[0]); ::close(par[1]); return -1; }
  if (filho == 0) {
    ::close(par[0]);  // senão a entrada d'elle nunca veria o fim do cano
    ::dup2(par[1], STDIN_FILENO);
    // Sahida e erro ao buraco: byte do filho no terminal estraga o quadro.
    const int buraco = ::open("/dev/null", O_WRONLY);
    if (buraco >= 0) {
      ::dup2(buraco, STDOUT_FILENO);
      ::dup2(buraco, STDERR_FILENO);
      ::close(buraco);
    }
    ::close(par[1]);
    // A MORTE PROMETTIDA: cahindo o tocador por signal, o systema manda SIGTERM
    // a este filho. E pergunta-se pelo pae, que elle pode ter morrido no meio.
    ::prctl(PR_SET_PDEATHSIG, SIGTERM);
    if (::getppid() == 1) ::_exit(0);
    const std::vector<std::string> ordem = argumentos_da_lousa();
    std::vector<char*> argv;
    for (const std::string& um : ordem)
      argv.push_back(const_cast<char*>(um.c_str()));
    argv.push_back(nullptr);
    ::execvp(argv[0], argv.data());
    ::_exit(127);  // o 127 do shell para «commando não achado»
  }
  ::close(par[1]);
  // NÃO BLOQUEANTE: cano cheio não ha de segurar o quadro do pintor.
  const int bandeiras = ::fcntl(par[0], F_GETFL, 0);
  if (bandeiras >= 0) ::fcntl(par[0], F_SETFL, bandeiras | O_NONBLOCK);
  *cano = par[0];
  return static_cast<int>(filho);
}

}  // namespace

Lousa::Lousa(ModoDaLousa modo) noexcept
    : parecer_(
          parecer_da_lousa(modo, ha_display(), !versao_da_lousa().empty())) {
  if (!parecer_.de_pe) return;
  filho_ = ergue(&cano_);
  if (filho_ < 0) {
    parecer_ = {false, "o ueberzugpp não se ergueu"};
    return;
  }
  vivo_ = true;
  o_filho.store(filho_);
  // UMA vez por processo: o atexit não desregista, e registar por objecto
  // encheria a taboa d'elle na bateria que erguesse muitas lousas.
  static const bool registado = std::atexit(mata_o_filho) == 0;
  (void)registado;
}

Lousa::~Lousa() noexcept {
  tira_tudo();
  // O fim do cano é o pedido de sahir em ordem; o SIGTERM vem depois, para o
  // caso de elle estar preso a redimensionar uma imagem grande.
  if (cano_ >= 0) ::close(cano_);
  if (filho_ > 0) {
    o_filho.store(-1);
    ::kill(filho_, SIGTERM);
    // A UNICA espera d'este modulo, e é da SAHIDA: sem ella o filho ficaria
    // zombie e a janella d'elle podia sobreviver ao ultimo quadro do tocador,
    // que é justamente o fantasma que a issue manda não deixar na tela.
    int estado = 0;
    while (::waitpid(filho_, &estado, 0) < 0 && errno == EINTR) {}
  }
}

bool Lousa::disponivel() const noexcept { return vivo_ && cano_ >= 0; }

bool Lousa::escreve(const std::string& ordem) noexcept {
  if (!disponivel()) return false;
  const ::ssize_t postos =
      ::send(cano_, ordem.data(), ordem.size(), MSG_NOSIGNAL);
  if (postos == static_cast<::ssize_t>(ordem.size())) return true;
  ++descartadas_;
  // Cano cheio é passageiro, e a ordem descarta-se INTEIRA: meia linha de JSON
  // seria peor que linha nenhuma, que o filho lê por linha e a seguinte
  // emendaria n'ella. Toda outra falha, e a escripta PARTIDA ao meio, matam a
  // lousa: d'ahi em diante o filho já não entende o que vem. E mata-se elle
  // junto, que deixál-o com a janella de pé poria imagem velha por cima dos
  // symbolos do chafa que o painel volta a pintar.
  if (postos < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return false;
  vivo_ = false;
  if (filho_ > 0) ::kill(filho_, SIGTERM);
  return false;
}

bool Lousa::poe(std::string_view identidade, const std::filesystem::path& imagem,
                int collunha, int linha, std::size_t largura,
                std::size_t altura) noexcept {
  if (!disponivel() || imagem.empty() || largura == 0 || altura == 0)
    return false;
  const std::string ordem =
      ordem_de_por(identidade, imagem, collunha, linha, largura, altura);
  // A MESMA ordem não se torna a mandar, e não é economia de bytes: o
  // Überzug++ redimensiona a imagem a cada `add`, e o pintor pediria vinte
  // redimensionamentos por segundo de uma capa que não mudou.
  const std::string chave(identidade);
  const auto assento = postas_.find(chave);
  if (assento != postas_.end() && assento->second == ordem) return true;
  if (!escreve(ordem)) return false;
  postas_[chave] = ordem;
  return true;
}

bool Lousa::tira(std::string_view identidade) noexcept {
  const std::string chave(identidade);
  // O que não está posto não se tira: mandar `remove` de uma identidade que
  // nunca se poz seria uma linha por quadro na faixa sem capa alguma.
  if (postas_.erase(chave) == 0) return true;
  return escreve(ordem_de_tirar(identidade));
}

void Lousa::tira_tudo() noexcept {
  // As chaves copiam-se ANTES: o tira() muta a taboa, e apagar dentro do laço
  // que a percorre invalidaria o proprio percurso.
  std::vector<std::string> quaes;
  for (const auto& posta : postas_) quaes.push_back(posta.first);
  for (const std::string& qual : quaes) tira(qual);
}

std::vector<std::string> argumentos_da_lousa() {
  return {"ueberzugpp", "layer", "--silent", "-o", "x11"};
}

std::string escapado_em_json(std::string_view texto) {
  std::string sahida;
  for (const char letra : texto) {
    const auto octeto = static_cast<unsigned char>(letra);
    if (letra == '"' || letra == '\\') {
      sahida += '\\';
      sahida += letra;
    } else if (octeto < 0x20) {
      // Os de controle vão TODOS na fórma longa: uma só cobre os trinta e
      // dous, e caminho com um d'elles dentro não merece taboa á parte.
      static const char kAlgarismos[] = "0123456789abcdef";
      sahida += "\\u00";
      sahida += kAlgarismos[octeto >> 4];
      sahida += kAlgarismos[octeto & 0x0F];
    } else {
      sahida += letra;
    }
  }
  return sahida;
}

std::string ordem_de_por(std::string_view identidade,
                         const std::filesystem::path& imagem, int collunha,
                         int linha, std::size_t largura, std::size_t altura) {
  return "{\"action\":\"add\",\"identifier\":\"" + escapado_em_json(identidade) +
         "\",\"x\":" + std::to_string(collunha) +
         ",\"y\":" + std::to_string(linha) +
         ",\"max_width\":" + std::to_string(largura) +
         ",\"max_height\":" + std::to_string(altura) + ",\"path\":\"" +
         escapado_em_json(imagem.string()) + "\"}\n";
}

std::string ordem_de_tirar(std::string_view identidade) {
  return "{\"action\":\"remove\",\"identifier\":\"" +
         escapado_em_json(identidade) + "\"}\n";
}

Parecer parecer_da_lousa(ModoDaLousa modo, bool ha_display, bool ha_programa) {
  if (modo == ModoDaLousa::Nao)
    return {false, "desligada pelo ajuste: lousa = nao"};
  // A falta do PROGRAMA vem antes da do DISPLAY, e não é ordem gratuita: quem
  // não o installou ha de ler o remedio, e não «sem DISPLAY», que o mandaria
  // caçar defeito no servidor graphico por causa de um apt que falta.
  if (!ha_programa) return {false, "falta o ueberzugpp; ficam os symbolos"};
  if (!ha_display && modo != ModoDaLousa::Sim)
    return {false, "sem DISPLAY; a janella d'ella é de X11"};
  return {true, "X11"};
}

bool ha_display() {
  const char* const tela = std::getenv("DISPLAY");
  return tela != nullptr && tela[0] != '\0';
}

std::string versao_da_lousa() {
  // UMA chamada responde ás duas perguntas do diagnostico: se o programa está,
  // e qual é. Perguntar ao PATH á mão daria a primeira e não a segunda, e o
  // --sonda ha de dizer a versão, que é o que muda o protocolo por baixo de nós.
  std::string colhido;
  if (corre({"ueberzugpp", "--version"}, &colhido) != 0) return {};
  const std::size_t fim = colhido.find('\n');
  if (fim != std::string::npos) colhido.resize(fim);
  while (!colhido.empty() && colhido.back() == '\r') colhido.pop_back();
  return colhido;
}

std::string texto_da_lousa(const Parecer& parecer, std::string_view versao) {
  std::string texto = "\n  lousa: ";
  if (!parecer.de_pe) return texto + parecer.razao + "\n";
  // A versão vae CRUA como o programa a deu («ueberzugpp 2.9.8»): recortar-lhe
  // o nome para o tornar a escrever seria duas verdades a divergirem no dia em
  // que elle mudar a linha.
  texto += versao.empty() ? "ueberzugpp" : std::string(versao);
  return texto + ", " + parecer.razao + "\n";
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
