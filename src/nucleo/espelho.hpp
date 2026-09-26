#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace mysong::nucleo {
struct ListaNoDisco {
  std::string nome;
  std::vector<std::string> faixas;
};
class EspelhoDePlaylists {
 public:
  explicit EspelhoDePlaylists(std::filesystem::path raiz);
  ~EspelhoDePlaylists();
  EspelhoDePlaylists(const EspelhoDePlaylists&) = delete;
  EspelhoDePlaylists& operator=(const EspelhoDePlaylists&) = delete;
  bool prepara(const std::vector<ListaNoDisco>& listas);
  bool publica();
  void confirma() noexcept;
 private:
  std::filesystem::path destino_, temporario_;
  bool publicado_ = false, havia_ = false, confirmado_ = false;
};
}  // namespace mysong::nucleo
