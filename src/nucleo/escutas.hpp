#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "nucleo/biblioteca.hpp"
#include "nucleo/tocador.hpp"

struct sqlite3;

namespace mysong::nucleo {

struct ItemDeEscuta {
  std::string nome, artista;
  std::int64_t milissegundos = 0;
  std::int64_t reproducoes = 0;
};
struct ResumoDasEscutas {
  std::int64_t milissegundos = 0, reproducoes = 0, musicas = 0, artistas = 0;
  std::vector<ItemDeEscuta> mais_musicas, mais_artistas;
  ItemDeEscuta atual;
  std::string erro;
};

// Banco próprio, preservado quando o índice da biblioteca é reconstruído.
// O relógio monotônico mede a reprodução; a posição só valida o avanço real.
class EstatisticasDeEscuta {
 public:
  using Relogio = std::chrono::steady_clock;
  using Calendario = std::chrono::system_clock;
  using Metadados = std::function<Faixa(const std::string&)>;
  explicit EstatisticasDeEscuta(std::filesystem::path banco, Metadados metadados = {});
  ~EstatisticasDeEscuta();
  EstatisticasDeEscuta(const EstatisticasDeEscuta&) = delete;
  EstatisticasDeEscuta& operator=(const EstatisticasDeEscuta&) = delete;

  void observa(const Retracto& instante, Relogio::time_point agora = Relogio::now(),
               Calendario::time_point data = Calendario::now());
  bool salva();
  ResumoDasEscutas resumo(int dias = 0, const std::string& faixa = {},
                         Calendario::time_point data = Calendario::now());

 private:
  struct Registro {
    std::string ciclo, caminho, titulo, artista;
    std::int64_t dia = 0, milissegundos = 0, reproducoes = 0;
  };
  bool grava();
  void acrescenta(std::int64_t ms, Calendario::time_point data);
  mutable std::mutex tranca_;
  sqlite3* banco_ = nullptr;
  std::string erro_;
  Metadados metadados_;
  std::map<std::pair<std::string, std::int64_t>, Registro> pendentes_;
  std::map<std::int64_t, Registro> dias_correntes_;
  bool amostrado_ = false, contada_ = false;
  Retracto anterior_;
  Relogio::time_point instante_anterior_{}, ultima_gravacao_{};
  double espera_ = 0, fracao_ms_ = 0;
  std::int64_t ouvido_ = 0;
  Faixa faixa_;
  std::string ciclo_;
};

}  // namespace mysong::nucleo
