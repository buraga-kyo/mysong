#include "nucleo/espelho.hpp"
#include <fstream>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace mysong::nucleo {
namespace {
bool troca_pastas(const std::filesystem::path& primeira,
                  const std::filesystem::path& segunda) {
  return ::syscall(SYS_renameat2, AT_FDCWD, primeira.c_str(), AT_FDCWD,
                   segunda.c_str(), RENAME_EXCHANGE) == 0;
}

bool pasta_controlada(const std::filesystem::path& pasta) {
  if (std::filesystem::is_symlink(pasta) || !std::filesystem::is_directory(pasta))
    return false;
  std::ifstream marca(pasta / ".mysong");
  std::string assinatura;
  std::getline(marca, assinatura);
  if (assinatura != "Playlists do MySong") return false;
  for (const auto& entrada : std::filesystem::recursive_directory_iterator(pasta)) {
    if (entrada.is_symlink() || entrada.is_directory()) continue;
    if (entrada.path() != pasta / ".mysong") return false;
  }
  return true;
}

bool nome_seguro(const std::string& nome) {
  return !nome.empty() && nome != "." && nome != ".." && nome != ".mysong" &&
         nome.find('/') == std::string::npos && nome.find('\0') == std::string::npos;
}
}  // namespace

EspelhoDePlaylists::EspelhoDePlaylists(std::filesystem::path raiz)
    : destino_(std::move(raiz) / "Playlists") {}

bool EspelhoDePlaylists::prepara(const std::vector<ListaNoDisco>& listas) {
  try {
    std::filesystem::create_directories(destino_.parent_path());
    havia_ = std::filesystem::exists(std::filesystem::symlink_status(destino_));
    if (havia_ && !pasta_controlada(destino_)) return false;
    const std::string molde = (destino_.parent_path() / ".playlists-XXXXXX").string();
    std::vector<char> nome(molde.begin(), molde.end());
    nome.push_back('\0');
    if (::mkdtemp(nome.data()) == nullptr) return false;
    temporario_ = nome.data();
    std::ofstream marca(temporario_ / ".mysong");
    marca << "Playlists do MySong\n";
    marca.close();
    if (!marca) return false;
    for (const auto& lista : listas) {
      if (!nome_seguro(lista.nome)) return false;
      const auto pasta = temporario_ / lista.nome;
      std::filesystem::create_directory(pasta);
      std::size_t ordem = 0;
      for (const auto& faixa : lista.faixas) {
        const auto alvo = std::filesystem::absolute(faixa);
        const auto folha = std::to_string(++ordem) + " - " + alvo.filename().string();
        std::filesystem::create_symlink(alvo, pasta / folha);
      }
    }
    return true;
  } catch (const std::filesystem::filesystem_error&) { return false; }
}

bool EspelhoDePlaylists::publica() {
  if (temporario_.empty()) return false;
  if (havia_) publicado_ = troca_pastas(temporario_, destino_);
  else {
    publicado_ = ::syscall(SYS_renameat2, AT_FDCWD, temporario_.c_str(), AT_FDCWD,
                            destino_.c_str(), RENAME_NOREPLACE) == 0;
  }
  return publicado_;
}

void EspelhoDePlaylists::confirma() noexcept { confirmado_ = true; }

EspelhoDePlaylists::~EspelhoDePlaylists() {
  if (publicado_ && !confirmado_) {
    // Se a reversão falhar, preservamos as duas árvores para recuperação.
    if (havia_ && !troca_pastas(temporario_, destino_)) return;
    if (!havia_) {
      std::error_code erro;
      std::filesystem::rename(destino_, temporario_, erro);
      if (erro) return;
    }
  }
  std::error_code erro;
  if (!temporario_.empty()) std::filesystem::remove_all(temporario_, erro);
}
}  // namespace mysong::nucleo
