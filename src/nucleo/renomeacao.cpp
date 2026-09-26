#include "nucleo/renomeacao.hpp"
#include "nucleo/aquisicao.hpp"
#include "nucleo/ajustes.hpp"
#include "nucleo/varredura.hpp"
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace mysong::nucleo {
namespace {
bool move_sem_sobrescrever(const std::string& anterior, const std::string& novo) {
  return anterior == novo || ::syscall(SYS_renameat2, AT_FDCWD, anterior.c_str(),
                                       AT_FDCWD, novo.c_str(), RENAME_NOREPLACE) == 0;
}
}  // namespace

Renomeacao renomeia_arquivo(const std::string& caminho, const std::string& titulo,
                            Biblioteca& biblioteca, Roleiro& listas) {
  Renomeacao resultado;
  Faixa anterior;
  resultado.titulo = saneia_utf8(aparar(titulo));
  if (resultado.titulo.empty() || !biblioteca.acha_por_caminho(caminho, anterior)) {
    resultado.razao = "título vazio ou faixa ausente do índice";
    return resultado;
  }
  const std::filesystem::path original(caminho);
  resultado.caminho = (original.parent_path() /
      (saneia_nome(resultado.titulo) + original.extension().string())).string();
  if (!move_sem_sobrescrever(caminho, resultado.caminho)) {
