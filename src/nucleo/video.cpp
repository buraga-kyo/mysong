// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROJECTOR — src/nucleo/video.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Não ha shell em logar algum: o `execvp` recebe o vector tal e
// qual, donde faixa chamada `; rm -rf ~` é nome de arquivo e não commando.
//
// DOMÍNIO ......... a faixa, e as ordens do operador.
// CONTRA-DOMÍNIO .. o processo da janella, e as linhas que lhe vão pelo soquete.
// INVARIANTE ...... fita alguma sobrevive ao projector: o destructor fecha. E
//                   fechar ESPERA o filho, donde processo algum fica orfão.
//                   Todo punho publico toma a TRANCA: este objecto é chamado do fio
//                   da tela e do fio do relogio, e `waitpid` chamado dos dous ao
//                   mesmo tempo daria um a colher o filho e outro a não o achar.
// Q.E.D. .......... abrir a segunda fita fecha a primeira, donde nunca ha duas
//                   janellas, e o audio nunca dobra por duas fitas.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/video.hpp"

#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <utility>

namespace mysong::nucleo {
namespace {

// As extensões que costumam trazer video. Lista FECHADA: extensão que não está
// aqui não abre janella, e é melhor recusar de mais que abrir janella vazia.
constexpr const char* kExtensoesComVideo[] = {
    ".mkv", ".mp4", ".webm", ".avi", ".mov", ".m4v", ".ogv", ".flv", ".wmv",
    ".mpg", ".mpeg", ".ts",
};

// O PRAZO da espera pelo soquete, e o passo d'ella. O mpv cria o soquete depois
// de subir, e subir leva tempo de disco: tentar uma vez e desistir daria «sem
// soquete» na machina carregada, que é falha inventada.
constexpr int kTentativasDoSoquete = 60;
constexpr int kMilesimosPorTentativa = 25;

std::string minuscula(std::string_view crua) {
  std::string baixa;
  baixa.reserve(crua.size());
  for (const unsigned char letra : crua)
    baixa += static_cast<char>(letra >= 'A' && letra <= 'Z' ? letra + 32
                                                            : letra);
  return baixa;
}

// abre_soquete — o punho ligado ao soquete do mpv, e menos um não havendo. Tenta
// mais de uma vez, com pausa: o soquete nasce depois do processo.
int abre_soquete(const std::filesystem::path& soquete) {
  for (int tentativa = 0; tentativa < kTentativasDoSoquete; ++tentativa) {
    const int punho = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (punho < 0) return -1;
    sockaddr_un posto{};
    posto.sun_family = AF_UNIX;
    const std::string caminho = soquete.string();
    // O caminho do soquete tem tecto de comprimento no proprio kernel. Cortá-lo
    // daria punho que liga ao logar errado; melhor recusar de vez.
    if (caminho.size() + 1 > sizeof(posto.sun_path)) {
      ::close(punho);
      return -1;
    }
    std::memcpy(posto.sun_path, caminho.c_str(), caminho.size() + 1);
    if (::connect(punho, reinterpret_cast<sockaddr*>(&posto), sizeof(posto)) ==
        0)
      return punho;
    ::close(punho);
    std::this_thread::sleep_for(
        std::chrono::milliseconds(kMilesimosPorTentativa));
  }
  return -1;
}

}  // namespace

bool extensao_com_video(std::string_view extensao) {
  const std::string baixa = minuscula(extensao);
  if (baixa.empty()) return false;
  return std::find(std::begin(kExtensoesComVideo), std::end(kExtensoesComVideo),
                   baixa) != std::end(kExtensoesComVideo);
}

bool tem_video(const std::filesystem::path& faixa) {
  return extensao_com_video(faixa.extension().string());
}

std::filesystem::path caminho_do_soquete(const std::filesystem::path& raiz,
                                         long pid) {
  return raiz / ("mysong-video-" + std::to_string(pid) + ".sock");
}

std::vector<std::string> argumentos_do_projector(
    const std::filesystem::path& faixa, const std::filesystem::path& soquete) {
  return {
      "mpv",
      // A CLASSE, nas duas fórmas: o X11 lê uma e o Wayland lê a outra, e quem
      // corre não sabe qual das duas o systema d'elle usa. Postas as duas, a
      // regra do RADICAL-OS acha a janella em qualquer d'ellas.
      std::string("--x11-name=") + kClasseDoVideo,
      std::string("--wayland-app-id=") + kClasseDoVideo,
      "--title=mysong \xe2\x96\xb8 " + faixa.filename().string(),
      "--input-ipc-server=" + soquete.string(),
      // SEM terminal: elle é nosso, e a TUI está a pintar n'elle. Sem esta
      // bandeira o mpv escreve a barra de estado por cima do quadro.
      "--no-terminal",
      // A janella nasce ainda que a faixa demore a abrir: sem isto, o operador
      // carrega na tecla e não vê nada acontecer.
      "--force-window=yes",
      // Sahe quando a faixa acaba, em vez de ficar de pé vazia.
      "--keep-open=no",
      "--",
      faixa.string(),
  };
}

std::string escapa_json(std::string_view crua) {
  std::string limpa;
  limpa.reserve(crua.size() + 8);
  for (const char letra : crua) {
    if (letra == '"' || letra == '\\') {
      limpa += '\\';
      limpa += letra;
    } else if (static_cast<unsigned char>(letra) < 0x20) {
      // Controle vae na fórma longa: o JSON não admitte byte de controle crú, e
      // nome de arquivo com tabulação existe.
      char molde[8] = {0};
      std::snprintf(molde, sizeof(molde), "\\u%04x",
                    static_cast<unsigned>(static_cast<unsigned char>(letra)));
      limpa += molde;
    } else {
      limpa += letra;
    }
  }
  return limpa;
}

std::string ordem_simples(std::string_view verbo) {
  return "{\"command\":[\"" + escapa_json(verbo) + "\"]}\n";
}

std::string ordem_de_bandeira(std::string_view propriedade, bool ligada) {
  return "{\"command\":[\"set_property\",\"" + escapa_json(propriedade) +
         "\"," + (ligada ? "true" : "false") + "]}\n";
}

std::string ordem_de_numero(std::string_view propriedade, double valor) {
  char molde[32] = {0};
  std::snprintf(molde, sizeof(molde), "%.3f", valor);
  return "{\"command\":[\"set_property\",\"" + escapa_json(propriedade) +
         "\"," + molde + "]}\n";
}

std::string ordem_de_busca(double segundos) {
  char molde[32] = {0};
  std::snprintf(molde, sizeof(molde), "%.3f", segundos);
  // ABSOLUTO, e não relativo: quem chama sabe onde quer estar, e relativo faria
  // duas ordens seguidas somarem-se de modo que a tela não previa.
  return std::string("{\"command\":[\"seek\",") + molde + ",\"absolute\"]}\n";
}

std::string ordem_de_busca_relativa(double deslocamento) {
  char molde[32] = {0};
  std::snprintf(molde, sizeof(molde), "%.3f", deslocamento);
  return std::string("{\"command\":[\"seek\",") + molde + ",\"relative\"]}\n";
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
