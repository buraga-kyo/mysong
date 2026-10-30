#pragma once
#include <string_view>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace mysong::nucleo {
using VolumesDasPartes = std::array<int, 4>;
using ArquivosDasPartes = std::array<std::string, 4>;
// Ordem comum ao processamento, ao áudio e à interface.
inline constexpr std::array<const char*, 4> NOMES_DAS_PARTES{"Voz", "Bateria", "Baixo", "Demais instrumentos"};
inline constexpr std::array<const char*, 4> ARQUIVOS_DAS_PARTES{"vocals.flac", "drums.flac", "bass.flac", "other.flac"};

enum class EstadoDaSeparacao { Parado, Preparando, Separando, Conferindo, Pronto, Cancelando, Cancelado, Falhou };
struct RetratoDaSeparacao {
  EstadoDaSeparacao estado = EstadoDaSeparacao::Parado;
  std::filesystem::path original;
  ArquivosDasPartes arquivos{};
  std::optional<int> porcentagem;
  std::string detalhe;
  std::size_t versao = 0;
  bool cache = false;
  std::chrono::steady_clock::time_point inicio{};
};
bool separacao_ocupada(EstadoDaSeparacao estado);
std::filesystem::path raiz_das_separacoes();
std::string programa_de_separacao();
std::vector<std::string> argumentos_da_separacao(const std::string& programa,
    const std::filesystem::path& audio, const std::filesystem::path& destino);
std::optional<int> progresso_da_separacao(std::string_view linha);
std::string grafo_da_mistura(const std::array<int, 4>& ids, const VolumesDasPartes& volumes);

class SeparadorDeAudio {
 public:
  using Observador = std::function<void(std::string_view)>;
  using Executor = std::function<int(const std::vector<std::string>&, const Observador&)>;
  explicit SeparadorDeAudio(std::filesystem::path raiz = raiz_das_separacoes(),
                           std::string programa = programa_de_separacao(), Executor executor = {});
  ~SeparadorDeAudio();
  bool inicia(const std::filesystem::path& audio);
  void cancela();
  void espera();
  RetratoDaSeparacao retrato() const;
 private:
  void trabalha(std::filesystem::path audio);
  void publica(EstadoDaSeparacao estado, std::string detalhe, std::optional<int> porcentagem = {});
  std::filesystem::path raiz_;
  std::string programa_;
  Executor executor_;
  mutable std::mutex tranca_;
  RetratoDaSeparacao atual_;
  std::shared_ptr<std::atomic_bool> cancelar_;
  std::thread trabalho_;
};
}  // namespace mysong::nucleo
