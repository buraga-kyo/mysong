#include "nucleo/renomeacao.hpp"
#include "nucleo/aquisicao.hpp"
#include "nucleo/ajustes.hpp"
#include "nucleo/letra.hpp"
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
  const std::filesystem::path letra_antiga = caminho_do_lrc(original);
  const std::filesystem::path letra_nova = caminho_do_lrc(resultado.caminho);
  if (!move_sem_sobrescrever(caminho, resultado.caminho)) {
    resultado.razao = "destino existente ou arquivo sem permissão para renomear";
    return resultado;
  }
  std::error_code erro_da_letra;
  const bool letra_existe = std::filesystem::exists(letra_antiga, erro_da_letra);
  const bool letra_movida = letra_existe && letra_antiga != letra_nova &&
                            move_sem_sobrescrever(letra_antiga.string(),
                                                  letra_nova.string());
  if (erro_da_letra || (letra_existe && !letra_movida && letra_antiga != letra_nova)) {
    move_sem_sobrescrever(resultado.caminho, caminho);
    resultado.razao = "letra ausente ou destino .lrc existente";
    return resultado;
  }
  const auto etiqueta = renomeia_titulo(resultado.caminho, resultado.titulo);
  if (!etiqueta.feito) {
    resultado.razao = etiqueta.razao;
    if (letra_movida) move_sem_sobrescrever(letra_nova.string(), letra_antiga.string());
    if (!move_sem_sobrescrever(resultado.caminho, caminho))
      resultado.razao += "; arquivo permanece em " + resultado.caminho;
    return resultado;
  }
  const bool listas_prontas = listas.muda_caminho(caminho, resultado.caminho);
  if (listas_prontas && biblioteca.renomeia(caminho, resultado.caminho, resultado.titulo)) {
    resultado.feita = true;
    return resultado;
  }
  resultado.razao = "não foi possível atualizar índice ou playlists";
  const bool listas_repostas = !listas_prontas || listas.muda_caminho(resultado.caminho, caminho);
  const bool titulo_reposto = renomeia_titulo(resultado.caminho, anterior.titulo).feito;
  const bool arquivo_reposto = move_sem_sobrescrever(resultado.caminho, caminho);
  if (letra_movida &&
      !move_sem_sobrescrever(letra_nova.string(), letra_antiga.string()))
    resultado.razao += "; letra permanece em " + letra_nova.string();
  if (!listas_repostas || !titulo_reposto || !arquivo_reposto)
    resultado.razao += "; recuperação incompleta, confira " + resultado.caminho;
  return resultado;
}
}  // namespace mysong::nucleo
