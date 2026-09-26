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
