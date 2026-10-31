#include "nucleo/separacao.hpp"
#include "nucleo/aquisicao.hpp"
#include "nucleo/capa.hpp"
#include <taglib/fileref.h>
#include <taglib/audioproperties.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <unistd.h>

namespace mysong::nucleo {
namespace {
std::string chave_do_audio(const std::filesystem::path& audio) {
  return somma_dos_octetos(audio.string() + ":" + std::to_string(std::filesystem::file_size(audio)) +
      ":" + std::to_string(std::filesystem::last_write_time(audio).time_since_epoch().count()) + ":htdemucs-4.0.1-flac-v1");
}
ArquivosDasPartes arquivos_em(const std::filesystem::path& pasta) {
  ArquivosDasPartes arquivos;
  for (std::size_t i = 0; i < arquivos.size(); ++i) arquivos[i] = (pasta / ARQUIVOS_DAS_PARTES[i]).string();
  return arquivos;
}
bool cache_pronto(const std::filesystem::path& pasta) {
  std::ifstream selo(pasta / "pronto");
  std::string versao;
  if (!std::getline(selo, versao) || versao != "mysong-partes-v1") return false;
  for (const auto* nome : ARQUIVOS_DAS_PARTES) {
    std::uintmax_t esperado = 0;
    std::error_code erro;
    if (!(selo >> esperado) || esperado == 0 || !std::filesystem::is_regular_file(pasta / nome, erro) || erro ||
        std::filesystem::file_size(pasta / nome, erro) != esperado || erro) return false;
  }
  return true;
}
}

bool separacao_ocupada(EstadoDaSeparacao estado) {
  return estado == EstadoDaSeparacao::Preparando || estado == EstadoDaSeparacao::Separando ||
      estado == EstadoDaSeparacao::Conferindo || estado == EstadoDaSeparacao::Cancelando;
}
std::filesystem::path raiz_das_separacoes() {
  const char* dados = std::getenv("XDG_DATA_HOME");
  if (dados && *dados) return std::filesystem::path(dados) / "mysong" / "separacoes";
  const char* casa = std::getenv("HOME");
  return casa && *casa ? std::filesystem::path(casa) / ".local/share/mysong/separacoes" : std::filesystem::path{};
}
std::string programa_de_separacao() {
  const char* definido = std::getenv("MYSONG_SEPARADOR");
  if (definido && *definido && ::access(definido, X_OK) == 0) return definido;
  const auto local = raiz_das_separacoes().parent_path() / "separador/bin/demucs";
  if (::access(local.c_str(), X_OK) == 0) return local.string();
  const char* caminho = std::getenv("PATH");
  std::istringstream pastas(caminho ? caminho : "");
  std::string pasta;
  while (std::getline(pastas, pasta, ':')) {
    const auto candidato = std::filesystem::path(pasta) / "demucs";
    if (!pasta.empty() && ::access(candidato.c_str(), X_OK) == 0) return candidato.string();
  }
  return {};
}
std::vector<std::string> argumentos_da_separacao(const std::string& programa,
    const std::filesystem::path& audio, const std::filesystem::path& destino) {
  return {"nice", "-n", "10", "env", "OMP_NUM_THREADS=2", "MKL_NUM_THREADS=2", "PYTHONUNBUFFERED=1",
      programa, "-n", "htdemucs", "-d", "cpu", "--segment", "7", "--shifts", "1", "-j", "1",
      "--flac", "--filename", "{stem}.{ext}", "-o", destino.string(), "--", audio.string()};
}
std::optional<int> progresso_da_separacao(std::string_view linha) {
  const auto fim = linha.find('%');
  if (fim == std::string_view::npos) return {};
  auto inicio = fim;
  while (inicio > 0 && linha[inicio - 1] >= '0' && linha[inicio - 1] <= '9') --inicio;
  if (inicio == fim || fim - inicio > 3) return {};
  const int numero = std::stoi(std::string(linha.substr(inicio, fim - inicio)));
  return numero >= 0 && numero <= 100 ? std::optional<int>{numero} : std::nullopt;
}
MedidasDaSeparacao medidas_da_separacao(std::string_view linha) {
  MedidasDaSeparacao medidas;
  static const std::regex trecho(R"(\|\s*([0-9]+(?:\.[0-9]+)?)/([0-9]+(?:\.[0-9]+)?))");
  const std::string texto(linha);
  std::smatch campos;
  if (std::regex_search(texto, campos, trecho)) {
    try {
      const double feito = std::stod(campos[1]), total = std::stod(campos[2]);
      if (std::isfinite(feito) && std::isfinite(total) && total > 0 && feito <= total) {
        medidas.processado = feito; medidas.total = total;
      }
    } catch (...) {}
  }
  const auto abre = linha.rfind('['), seta = linha.find('<', abre);
  const auto fim = linha.find_first_of(",]", seta);
  if (abre == std::string_view::npos || seta == std::string_view::npos || fim == std::string_view::npos)
    return medidas;
  const auto tempo = linha.substr(seta + 1, fim - seta - 1);
  int segundos = 0, componente = 0, digitos = 0, partes = 0;
  for (char c : tempo) {
    if (c == ':' && digitos > 0 && partes < 2) {
      if ((partes > 0 && componente >= 60) || componente > 10000) return medidas;
      segundos = segundos * 60 + componente;
      componente = 0; digitos = 0; ++partes;
    } else if (c >= '0' && c <= '9' && digitos < 5) {
      componente = componente * 10 + c - '0'; ++digitos;
    } else return medidas;
  }
  if (digitos > 0 && partes > 0 && componente < 60)
    medidas.restante = segundos * 60 + componente;
  return medidas;
}

std::string grafo_da_mistura(const std::array<int, 4>& ids, const VolumesDasPartes& volumes) {
  std::string grafo;
  for (std::size_t i = 0; i < ids.size(); ++i) {
    if (ids[i] <= 0) return {};
    grafo += "[aid" + std::to_string(ids[i]) + "]volume=" +
        std::to_string(std::clamp(volumes[i], 0, 100) / 100.0) + "[parte" + std::to_string(i) + "];";
  }
  return grafo + "[parte0][parte1][parte2][parte3]amix=inputs=4:normalize=0,alimiter=level=0[ao]";
}

SeparadorDeAudio::SeparadorDeAudio(std::filesystem::path raiz, std::string programa, Executor executor)
    : raiz_(std::move(raiz)), programa_(std::move(programa)), executor_(std::move(executor)) {
  if (!executor_) executor_ = [](const auto& args, const auto& observa) { return corre(args, nullptr, observa, true); };
}
SeparadorDeAudio::~SeparadorDeAudio() { cancela(); espera(); }
void SeparadorDeAudio::espera() { if (trabalho_.joinable()) trabalho_.join(); }
RetratoDaSeparacao SeparadorDeAudio::retrato() const {
  std::lock_guard<std::mutex> chave(tranca_); return atual_;
}
void SeparadorDeAudio::publica(EstadoDaSeparacao estado, std::string detalhe, std::optional<int> percentual) {
  std::lock_guard<std::mutex> chave(tranca_);
  if (cancelar_ && cancelar_->load() && estado != EstadoDaSeparacao::Cancelado && estado != EstadoDaSeparacao::Falhou) return;
  atual_.estado = estado; atual_.detalhe = std::move(detalhe); atual_.porcentagem = percentual;
  atual_.ultima_atividade = std::chrono::steady_clock::now(); ++atual_.versao;
}
bool SeparadorDeAudio::inicia(const std::filesystem::path& audio) {
  if (separacao_ocupada(retrato().estado)) return false;
  espera();
  {
    std::lock_guard<std::mutex> chave(tranca_);
    const auto versao = atual_.versao + 1;
    atual_ = {}; atual_.versao = versao; atual_.original = audio;
    atual_.estado = EstadoDaSeparacao::Preparando; atual_.detalhe = "Conferindo a música e o cache";
    atual_.inicio = std::chrono::steady_clock::now();
    atual_.ultima_atividade = atual_.inicio;
    cancelar_ = std::make_shared<std::atomic_bool>(false);
  }
  try { trabalho_ = std::thread(&SeparadorDeAudio::trabalha, this, audio); }
  catch (...) { publica(EstadoDaSeparacao::Falhou, "Não foi possível iniciar a separação"); return false; }
  return true;
}
void SeparadorDeAudio::cancela() {
  std::lock_guard<std::mutex> chave(tranca_);
  if (!separacao_ocupada(atual_.estado) || !cancelar_) return;
  cancelar_->store(true); atual_.estado = EstadoDaSeparacao::Cancelando;
  atual_.detalhe = "Encerrando o processamento"; ++atual_.versao;
}
void SeparadorDeAudio::trabalha(std::filesystem::path audio) {
  std::filesystem::path temporario;
  std::string registro;
  const EscopoDaBaixa escopo(cancelar_);
  try {
    audio = std::filesystem::canonical(audio);
    if (!std::filesystem::is_regular_file(audio) || raiz_.empty())
      throw std::runtime_error("Selecione um arquivo de áudio local");
    const auto chave = chave_do_audio(audio);
    const auto destino = raiz_ / chave;
    if (!cache_pronto(destino)) {
      if (programa_.empty()) throw std::runtime_error("Motor de separação indisponível. Instale com installacao/instala-separador.sh");
      std::filesystem::create_directories(raiz_);
      TagLib::FileRef original(audio.c_str());
      if (original.isNull() || !original.audioProperties()) throw std::runtime_error("Não foi possível ler o áudio da música");
      const auto duracao = original.audioProperties()->lengthInMilliseconds();
      const auto espaco = std::filesystem::space(raiz_);
      const auto reserva = static_cast<std::uintmax_t>(std::max(1, duracao)) * 706 + 128 * 1024 * 1024;
      if (espaco.available < reserva) throw std::runtime_error("Espaço livre insuficiente para separar esta música");
      temporario = raiz_ / (chave + ".parte-" + std::to_string(::getpid()) + "-" +
          std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
      std::filesystem::create_directories(temporario);
      bool separando = false;
      bool suspenso = false;
      const auto observa = [&](std::string_view linha) {
        if (linha == "MYSONG_PROCESSO_SUSPENSO") suspenso = true;
        if (registro.size() < 65536) { registro += std::string(linha); registro += '\n'; }
        if (linha.find("Separating track") != std::string_view::npos) {
          separando = true; publica(EstadoDaSeparacao::Preparando, "Lendo e decodificando o áudio da música");
        }
        if (const auto percentual = progresso_da_separacao(linha)) {
          if (separando) {
            std::lock_guard<std::mutex> guarda(tranca_);
            atual_.medidas = medidas_da_separacao(linha);
          }
          publica(separando ? EstadoDaSeparacao::Separando : EstadoDaSeparacao::Preparando,
              separando ? (*percentual == 100 ? "Salvando as partes em FLAC" : "Separando as partes da música")
                        : "Obtendo os recursos da separação", percentual);
        }
      };
      publica(EstadoDaSeparacao::Preparando, "Preparando o motor; a primeira utilização pode baixar recursos");
      const auto fim = executor_(argumentos_da_separacao(programa_, audio, temporario), observa);
      if (baixa_interrompida()) throw std::runtime_error("cancelado");
      if (fim != 0) {
        std::filesystem::create_directories(raiz_ / "registros");
        const auto caminho = raiz_ / "registros" / (chave + ".log");
        std::ofstream(caminho) << registro;
        throw std::runtime_error(suspenso ? "O sistema suspendeu o processo. A tarefa foi encerrada; tente novamente." :
            "Não foi possível separar a música. Detalhes em " + caminho.string());
      }
      publica(EstadoDaSeparacao::Conferindo, "Conferindo os quatro arquivos e a duração do áudio");
      const auto saida = temporario / "htdemucs";
      std::ofstream selo(saida / "pronto");
      selo << "mysong-partes-v1\n";
      for (const auto* nome : ARQUIVOS_DAS_PARTES) {
        if (baixa_interrompida()) throw std::runtime_error("cancelado");
        const auto arquivo = saida / nome;
        TagLib::FileRef parte(arquivo.c_str());
        if (!std::filesystem::is_regular_file(arquivo) || parte.isNull() || !parte.audioProperties() ||
            parte.audioProperties()->lengthInMilliseconds() <= 0 ||
            std::abs(parte.audioProperties()->lengthInMilliseconds() - duracao) > 1000)
          throw std::runtime_error("As partes geradas estão incompletas; o original foi preservado");
        selo << std::filesystem::file_size(arquivo) << '\n';
      }
      selo.close();
      if (!selo.good() || chave_do_audio(audio) != chave) throw std::runtime_error("O áudio mudou ou não foi possível salvar o cache");
      if (baixa_interrompida()) throw std::runtime_error("cancelado");
      if (!cache_pronto(destino)) {
        if (std::filesystem::exists(destino)) std::filesystem::remove_all(destino);
        std::filesystem::rename(saida, destino);
      }
    } else {
      std::lock_guard<std::mutex> guarda(tranca_); atual_.cache = true;
    }
    if (!temporario.empty()) std::filesystem::remove_all(temporario);
    if (baixa_interrompida()) throw std::runtime_error("cancelado");
    {
      std::lock_guard<std::mutex> guarda(tranca_);
      atual_.original = audio;
      if (cancelar_->load()) {
        atual_.estado = EstadoDaSeparacao::Cancelado;
        atual_.detalhe = "Separação cancelada; o arquivo original foi preservado";
      } else {
        atual_.arquivos = arquivos_em(destino);
        atual_.estado = EstadoDaSeparacao::Pronto;
        atual_.detalhe = "Partes prontas. Escolha o que deseja ouvir.";
      }
      atual_.porcentagem.reset();
      ++atual_.versao;
    }
  } catch (const std::exception& erro) {
    std::error_code ignorado;
    if (!temporario.empty()) std::filesystem::remove_all(temporario, ignorado);
    publica(baixa_interrompida() ? EstadoDaSeparacao::Cancelado : EstadoDaSeparacao::Falhou,
        baixa_interrompida() ? "Separação cancelada; o arquivo original foi preservado" : erro.what());
  } catch (...) {
    std::error_code ignorado;
    if (!temporario.empty()) std::filesystem::remove_all(temporario, ignorado);
    publica(EstadoDaSeparacao::Falhou, "A separação falhou; o arquivo original foi preservado");
  }
}
}  // namespace mysong::nucleo
