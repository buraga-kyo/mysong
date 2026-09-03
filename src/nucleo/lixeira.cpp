// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LIXEIRA, LAVRA — src/nucleo/lixeira.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabeçalho. As tres regras da especificação freedesktop vivem aqui, e
// sómente aqui: o nome do par, o percent-encoding do `Path=`, e a data local.
//
// DOMÍNIO ......... um caminho, e a raiz da lixeira.
// CONTRA-DOMÍNIO .. o par arquivo e bilhete, e o desfecho por escripto.
// INVARIANTE ...... funcção alguma d'este arquivo lança pela borda: falta de
//                   disco e falta de permissão sahem por `razao`.
// Q.E.D. .......... o caminho lê-se do ambiente n'um logar só, donde a lixeira
//                   que a tela usa e a que a sonda diria são a mesma.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/lixeira.hpp"

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <system_error>

namespace mysong::nucleo {

// A barra CONSERVA-SE, e sómente ella entre as reservadas: o campo guarda um
// caminho, e escapá-la faria o gerenciador de arquivos ler um nome de arquivo
// com barras dentro. Espaço e acento, que é o que o acervo d'elle tem aos
// montes, sahem em `%20` e em dous octetos de `%XX` cada.
std::string escapa_o_caminho(std::string_view cru) {
  static constexpr char kAlgarismos[] = "0123456789ABCDEF";
  std::string url;
  url.reserve(cru.size());
  for (const char bruto : cru) {
    const unsigned char letra = static_cast<unsigned char>(bruto);
    if ((letra >= 'A' && letra <= 'Z') || (letra >= 'a' && letra <= 'z') ||
        (letra >= '0' && letra <= '9') || letra == '-' || letra == '_' ||
        letra == '.' || letra == '~' || letra == '/') {
      url += bruto;
      continue;
    }
    url += '%';
    url += kAlgarismos[letra >> 4];
    url += kAlgarismos[letra & 0x0Fu];
  }
  return url;
}

// Sem fuso escripto, que é o que a especificação pede: a hora é a do relogio de
// quem apagou, e é assim que o gerenciador de arquivos a mostra.
std::string data_da_exclusao(std::time_t quando) {
  std::tm partido = {};
  ::localtime_r(&quando, &partido);
  char linha[32] = {0};
  std::strftime(linha, sizeof(linha), "%Y-%m-%dT%H:%M:%S", &partido);
  return std::string(linha);
}

// O mesmo desenho do caminho_do_indice da janella, e de proposito: uma Casa que
// leia `$XDG_DATA_HOME` de duas maneiras teria dous logares de dados no dia em
// que o operador a assentasse. Directorio algum se cria aqui: quem cria é quem
// manda á lixeira, que é quem sabe se ha alguma cousa a mandar.
std::filesystem::path caminho_da_lixeira() {
  const char* dados = std::getenv("XDG_DATA_HOME");
  if (dados != nullptr && dados[0] != '\0')
    return std::filesystem::path(dados) / "Trash";
  const char* casa = std::getenv("HOME");
  if (casa == nullptr || casa[0] == '\0') return {};
  return std::filesystem::path(casa) / ".local" / "share" / "Trash";
}

namespace {
// UM arquivo, o par inteiro: nome livre, bilhete, e sómente depois a mudança.
// Junto, e não em tres funcções, porque a mudança que falha ha de apagar o
// bilhete que já se escreveu, e para isso precisa de o ter na mão.
bool poe_o_par(const std::filesystem::path& files,
               const std::filesystem::path& info,
               const std::filesystem::path& qual, std::string* nome,
               bool* copiada) {
  std::error_code erro;
  const std::string cru = qual.filename().string();
  std::string livre = cru;
  for (int n = 2; std::filesystem::exists(files / livre, erro) && n < 10000; ++n)
    livre = cru + "." + std::to_string(n);
  const std::filesystem::path bilhete = info / (livre + ".trashinfo");
  std::ofstream papel(bilhete, std::ios::binary | std::ios::trunc);
  papel << "[Trash Info]\nPath=" << escapa_o_caminho(qual.string())
        << "\nDeletionDate=" << data_da_exclusao(std::time(nullptr)) << "\n";
  papel.close();
  if (!papel.good()) return false;
  std::filesystem::rename(qual, files / livre, erro);
  if (erro) {
    // Outro volume: a lixeira mora no `$HOME`, e faixa de disco externo não se
    // renomeia para lá. Sem a copia, essa faixa não se apagaria de todo.
    std::filesystem::copy_file(qual, files / livre, erro);
    if (erro) {
      std::filesystem::remove(bilhete, erro);
      return false;
    }
    std::filesystem::remove(qual, erro);
    *copiada = true;
  }
  *nome = livre;
  return true;
}
}  // namespace

DaLixeira manda_a_lixeira(const std::filesystem::path& caminho,
                          const std::filesystem::path& lixeira) {
  DaLixeira desfecho;
  std::error_code erro;
  // As quatro recusas têm NOME. «Não deu» faria o recado da tela mentir por
  // omissão, e o operador ficaria sem saber se a faixa foi ou se está no logar.
  if (lixeira.empty()) {
    desfecho.razao = "não ha lixeira: nem HOME nem XDG_DATA_HOME";
    return desfecho;
  }
  const std::filesystem::path qual = std::filesystem::absolute(caminho, erro);
  if (!std::filesystem::exists(qual, erro)) {
    desfecho.razao = "esse arquivo já não está no logar";
    return desfecho;
  }
  std::filesystem::create_directories(lixeira / "files", erro);
  if (!erro) std::filesystem::create_directories(lixeira / "info", erro);
  if (erro) {
    desfecho.razao = "a lixeira não se deixa abrir: " + erro.message();
    return desfecho;
  }
  desfecho.feita = poe_o_par(lixeira / "files", lixeira / "info", qual,
                             &desfecho.nome, &desfecho.copiada);
  if (!desfecho.feita) desfecho.razao = "não se pôde mudar para a lixeira";
  return desfecho;
}

// A do systema. É a que a tela chama, e a unica que lê o ambiente.
DaLixeira manda_a_lixeira(const std::filesystem::path& caminho) {
  return manda_a_lixeira(caminho, caminho_da_lixeira());
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
