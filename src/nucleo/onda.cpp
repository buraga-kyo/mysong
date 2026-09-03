// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA ONDA — src/nucleo/onda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o ffmpeg no fim, apartado, para que o
// olho veja a fronteira de um relance.
//
// DOMÍNIO ......... o caminho de uma faixa, ou as amostras já decodificadas.
// CONTRA-DOMÍNIO .. a envolvente em [0,1], ou a ausencia com a razão dita.
// INVARIANTE ...... funcção alguma d'aqui lança, e faixa sem onda não é falha:
//                   é a barra chata, que nunca ha de pender do ffmpeg.
// Q.E.D. .......... sendo o cache texto com versão, a bateria escreve-o á mão
//                   e afere a recusa sem correr programa algum.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/onda.hpp"

#include "nucleo/capa.hpp"  // somma_dos_octetos, raiz_do_cache: o mesmo cache

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>

namespace mysong::nucleo {

std::vector<std::string> linha_de_commando_da_onda(
    const std::filesystem::path& faixa) {
  // Cada bandeira cobre um caso, e não um receio. O `-v error` cala o banner
  // que o ffmpeg escreve sempre; o `-nostdin` impede que elle dispute com o
  // FTXUI o teclado do operador, que corre no mesmo terminal; e o `-vn` deita
  // fóra a CAPA embutida, que sem elle o ffmpeg trataria por corrente de
  // video e a onda sahiria da imagem em vez de sahir do som.
  return {"ffmpeg", "-v",  "error", "-nostdin", "-i",    faixa.string(),
          "-vn",    "-ac", "1",     "-ar",      "8000",  "-f",
          "s16le",  "pipe:1"};
}

namespace {

// O BALDE de um ponto, e é UMA funcção para os DOUS regimes. Havendo amostras
// que cheguem, o balde tem `quantas / pontos` d'ellas e o RESTO fica no
// ultimo, que assim a cauda da faixa não se perde. Havendo MENOS amostras que
// baldes, degenera n'uma amostra por proporção: o indice não decresce, d'onde
// o balde sem amostra propria repete a do vizinho de traz.
struct Balde {
  std::size_t principio = 0, fim = 0;
};
Balde balde_do_ponto(std::size_t quantas, std::size_t p, std::size_t pontos) {
  Balde balde;
  const std::size_t por_balde = quantas / pontos;
  if (por_balde > 0) {
    balde.principio = p * por_balde;
    balde.fim = p + 1 == pontos ? quantas : balde.principio + por_balde;
    return balde;
  }
  balde.principio = (p * quantas) / pontos;
  balde.fim = balde.principio + 1;
  return balde;
}
}  // namespace

Onda onda_das_amostras(const std::int16_t* amostras, std::size_t quantas,
                       std::size_t pontos) {
  Onda onda;
  if (amostras == nullptr || quantas == 0 || pontos == 0) return onda;
  onda.pontos.assign(pontos, 0.0f);
  float maior = 0.0f;
  for (std::size_t p = 0; p < pontos; ++p) {
    const Balde balde = balde_do_ponto(quantas, p, pontos);
    // A somma em double: um balde de faixa comprida traz dezenas de milhar
    // de quadrados, e em float os ultimos não pesariam por falta de mantissa.
    double somma = 0.0;
    for (std::size_t i = balde.principio; i < balde.fim; ++i) {
      const double amostra = static_cast<double>(amostras[i]) / 32768.0;
      somma += amostra * amostra;
    }
    const std::size_t conta = balde.fim - balde.principio;
    onda.pontos[p] = static_cast<float>(
        std::sqrt(somma / static_cast<double>(conta)));
    maior = std::max(maior, onda.pontos[p]);
  }
  // Máximo zero dá zeros, e não divisão por zero: faixa calada é onda rasa.
  if (maior > 0.0f)
    for (float& ponto : onda.pontos) ponto /= maior;
  return onda;
}

std::string chave_da_onda(const std::filesystem::path& faixa) {
  std::error_code erro;
  const std::filesystem::path absoluto = std::filesystem::absolute(faixa, erro);
  // Os campos costuram-se com o octeto NULLO, como os do letreiro: sem
  // costura, caminho «a» de tamanho doze e caminho «a1» de tamanho dous
  // dariam a mesma somma, e a tela mostraria a fórma de outra faixa.
  std::string tudo = erro ? faixa.string() : absoluto.string();
  const std::uintmax_t tamanho = std::filesystem::file_size(faixa, erro);
  tudo.push_back('\0');
  tudo += std::to_string(erro ? 0 : tamanho);
  const std::filesystem::file_time_type quando =
      std::filesystem::last_write_time(faixa, erro);
  tudo.push_back('\0');
  tudo += std::to_string(erro ? 0 : quando.time_since_epoch().count());
  return somma_dos_octetos(tudo);
}

std::filesystem::path caminho_da_onda_em_cache(std::string_view chave) {
  // Chave vazia não dá caminho: sahiria o arquivo escondido «.onda», e duas
  // faixas sem chave cahiriam n'elle uma por cima da outra.
  if (chave.empty()) return {};
  const std::filesystem::path raiz = raiz_do_cache();
  if (raiz.empty()) return {};
  // `ondas/` ao lado de `capas/` e de `letreiro/`, e não misturado com ellas:
  // o que se guarda aqui é fórma de faixa, e apagar uma pasta não ha de levar
  // a outra pelo caminho.
  return raiz / "ondas" / (std::string(chave) + ".onda");
}

namespace {

// A MARCA e a VERSÃO do formato. A marca diz de quem é o arquivo; a versão diz
// que geração de conta o escreveu: mudando a conta dos pontos ou a escala dos
// valores, sobe-se a versão e o cache de hontem sahe recusado.
inline constexpr std::string_view MARCA_DA_ONDA = "mysong-onda";
inline constexpr int VERSAO_DA_ONDA = 1;

// O DEGRAU de um ponto: um octeto, que é a escala em que a onda se guarda. A
// cella do terminal tem oito degraus, e guardar float de quatro octetos seria
// guardar precisão que a tela deita fóra na primeira pintura.
int degrau_do_ponto(float ponto) {
  const float dentro = ponto < 0.0f ? 0.0f : (ponto > 1.0f ? 1.0f : ponto);
  return static_cast<int>(std::lround(dentro * 255.0f));
}

}  // namespace

bool escreve_onda(const std::filesystem::path& onde, const Onda& onda) {
  if (onde.empty() || !onda.pronta()) return false;
  std::error_code erro;
  std::filesystem::create_directories(onde.parent_path(), erro);
  if (erro) return false;
  // Por TEMPORARIO e RENAME, pela razão exacta do cache da capa: quem acha o
  // arquivo toma-o por bom sem lhe conferir octeto, d'onde um arquivo cortado
  // ao meio (queda a meio da escripta, disco cheio, duas instancias sobre a
  // mesma faixa) envenenaria essa chave para sempre. O rename no mesmo systema
  // de arquivos é atomico, e resolve de graça a corrida entre duas instancias.
  const std::filesystem::path meio =
      onde.string() + "." + std::to_string(::getpid()) + ".parte";
  std::ofstream sahida(meio, std::ios::trunc);
  if (!sahida) return false;
  sahida << MARCA_DA_ONDA << ' ' << VERSAO_DA_ONDA << ' ' << onda.pontos.size()
         << '\n';
  for (std::size_t p = 0; p < onda.pontos.size(); ++p)
    sahida << (p == 0 ? "" : " ") << degrau_do_ponto(onda.pontos[p]);
  sahida << '\n';
  // O `close` ANTES do `good`: o ultimo despejo corre no fecho, e aferil-o
  // antes d'elle daria por bom o erro que mora justamente na cauda.
  sahida.close();
  if (sahida.good()) std::filesystem::rename(meio, onde, erro);
  if (!sahida.good() || erro) {
    std::filesystem::remove(meio, erro);
    return false;
  }
  return true;
}

bool le_onda(const std::filesystem::path& onde, Onda* onda) {
  if (onde.empty()) return false;
  std::ifstream entrada(onde);
  if (!entrada) return false;
  std::string marca;
  int versao = 0;
  std::size_t quantos = 0;
  if (!(entrada >> marca >> versao >> quantos)) return false;
  if (marca != MARCA_DA_ONDA || versao != VERSAO_DA_ONDA || quantos == 0)
    return false;
  Onda lida;
  // A reserva CINGE-SE, e a conta declarada não se toma por palavra: arquivo
  // do cache com «mysong-onda 1 999999999999» pediria de uma vez memoria que
  // esta machina não tem, e cache corrompido não ha de derrubar o tocador.
  lida.pontos.reserve(std::min<std::size_t>(quantos, PONTOS_DA_ONDA));
  for (std::size_t p = 0; p < quantos; ++p) {
    int degrau = 0;
    if (!(entrada >> degrau) || degrau < 0 || degrau > 255) return false;
    lida.pontos.push_back(static_cast<float>(degrau) / 255.0f);
  }
  // Valor A MAIS tambem recusa: o cabeçalho declara a conta, e arquivo que a
  // desminta é formato que esta Casa não conhece, e não sobra innocente.
  int sobra = 0;
  if (entrada >> sobra) return false;
  if (onda != nullptr) *onda = std::move(lida);
  return true;
}

// ── E AGORA O QUE TOCA O MUNDO.

namespace {

// ha_no_caminho — o programa existe no PATH e corre. Lavra-se aqui, e não se
// toma á sonda: alli a busca vive em namespace anonymo por ser da taboa dos
// requisitos, e abrir aquelle modulo por quinze linhas custaria mais.
bool ha_no_caminho(const char* nome) {
  const char* const caminho = std::getenv("PATH");
  if (caminho == nullptr) return false;
  std::string_view resto(caminho);
  while (!resto.empty()) {
    const std::size_t corte = resto.find(':');
    const std::string_view pasta = resto.substr(
        0, corte == std::string_view::npos ? resto.size() : corte);
    if (!pasta.empty()) {
      std::string tentativa(pasta);
      tentativa += '/';
      tentativa += nome;
      struct stat marca = {};
      if (::stat(tentativa.c_str(), &marca) == 0 && S_ISREG(marca.st_mode) &&
          ::access(tentativa.c_str(), X_OK) == 0)
        return true;
    }
    if (corte == std::string_view::npos) break;
    resto.remove_prefix(corte + 1);
  }
  return false;
}

// corre_o_ffmpeg — fork e exec SEM shell: o stdout n'um cano, e o stdin com o
// stderr no buraco. O `corre` da aquisição faz quasi isto, e não serve por uma
// cousa: alli o filho herda o stdin do pae, que n'esta Casa é o terminal que o
// FTXUI governa. O `-nostdin` da linha de commando já o defende, e o
// descriptor fechado defende-o OUTRA VEZ: a guarda que mora no argv morre no
// dia em que alguem lhe mexer, e esta não. Menos um quer dizer que nem se
// pôde erguer o processo.
int corre_o_ffmpeg(const std::vector<std::string>& argumentos,
                   std::string* colhido) {
  int cano[2] = {-1, -1};
  if (::pipe(cano) != 0) return -1;
  const ::pid_t filho = ::fork();
  if (filho < 0) { ::close(cano[0]); ::close(cano[1]); return -1; }
  if (filho == 0) {
    ::close(cano[0]);
    ::dup2(cano[1], STDOUT_FILENO);
    // O stderr vae ao buraco: esta Casa corre debaixo de uma tela do FTXUI,
    // e uma linha de aviso no meio do quadro estraga o quadro.
    const int buraco = ::open("/dev/null", O_RDWR);
    if (buraco >= 0) {
      ::dup2(buraco, STDIN_FILENO);
      ::dup2(buraco, STDERR_FILENO);
      ::close(buraco);
    }
    ::close(cano[1]);
    std::vector<char*> argv;
    argv.reserve(argumentos.size() + 1);
    for (const std::string& um : argumentos)
      argv.push_back(const_cast<char*>(um.c_str()));
    argv.push_back(nullptr);
    ::execvp(argv[0], argv.data());
    ::_exit(127);  // o 127 do shell para «commando não achado»
  }
  ::close(cano[1]);
  // O pedaço é grande: a faixa de tres minutos dá quasi tres milhões de
  // octetos, e o balde de quatro mil pedia mil e tantas voltas de leitura.
  std::vector<char> pedaco(64 * 1024);
  ::ssize_t lidos = 0;
  while ((lidos = ::read(cano[0], pedaco.data(), pedaco.size())) > 0)
    colhido->append(pedaco.data(), static_cast<std::size_t>(lidos));
  ::close(cano[0]);
  int estado = 0;
  if (::waitpid(filho, &estado, 0) < 0) return -1;
  return WIFEXITED(estado) ? WEXITSTATUS(estado) : -1;
}

}  // namespace

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
