// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA LIXEIRA — testes/prova_lixeira.cpp
// ══════════════════════════════════════════════════════════════════════════
// Corre inteira em directorio temporario: a raiz da lixeira entra por
// parâmetro, donde prova alguma manda cousa á lixeira de quem nos usa. O que
// ella afere é o PAR da especificação freedesktop: arquivo em `files` e
// bilhete em `info`, com o mesmo nome, e o bilhete a dizer d'onde a faixa veio.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include "nucleo/lixeira.hpp"

namespace nu = mysong::nucleo;

namespace {

// A COVA da prova: um acervo de rascunho, e uma raiz de dados ao lado d'elle,
// que é onde a lixeira nasce. Nem o acervo do operador nem a lixeira d'elle se
// tocam, que é o que a raiz por parâmetro veio dar.
class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-lixeira-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_ / "acervo");
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  std::filesystem::path lixeira() const { return caminho_ / "dados" / "Trash"; }
  // Cada arquivo leva conteudo proprio, que é como se sabe depois qual d'elles
  // foi parar onde: dous arquivos vazios não se distinguem em `files`.
  std::filesystem::path poe(const std::string& nome, const std::string& dentro) {
    const std::filesystem::path qual = caminho_ / "acervo" / nome;
    std::ofstream(qual, std::ios::binary) << dentro;
    return qual;
  }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

std::string texto_de(const std::filesystem::path& qual) {
  std::ifstream fonte(qual, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(fonte), {});
}

}  // namespace

TEST_CASE("mandar á lixeira põe o arquivo em files e o bilhete em info") {
  Cova cova;
  const std::filesystem::path faixa = cova.poe("Á vista.mp3", "som");
  const nu::DaLixeira desfecho = nu::manda_a_lixeira(faixa, cova.lixeira());
  CHECK(desfecho.feita);
  CHECK(desfecho.razao.empty());
  CHECK(desfecho.nome == "Á vista.mp3");
  CHECK_FALSE(std::filesystem::exists(faixa));
  CHECK(texto_de(cova.lixeira() / "files" / desfecho.nome) == "som");
  // O bilhete leva o nome do arquivo mais a extensão da norma, e diz d'onde a
  // faixa veio: sem essa linha, a lixeira mostra a faixa e não a restaura.
  const std::string bilhete =
      texto_de(cova.lixeira() / "info" / "Á vista.mp3.trashinfo");
  CHECK(bilhete.rfind("[Trash Info]\n", 0) == 0);
  CHECK(bilhete.find("\nPath=" + nu::escapa_o_caminho(faixa.string()) + "\n") !=
        std::string::npos);
  CHECK(bilhete.find("\nDeletionDate=20") != std::string::npos);
}

TEST_CASE("o caminho do bilhete escapa o espaço e o acento, e guarda a barra") {
  // O acervo d'elle é todo de espaço e acento: sem o escape, o bilhete sahia
  // quebrado na primeira faixa que se apagasse.
  CHECK(nu::escapa_o_caminho("/tmp/a b/Água.mp3") == "/tmp/a%20b/%C3%81gua.mp3");
  // A barra CONSERVA-SE, que o campo guarda caminho e não nome de arquivo.
  CHECK(nu::escapa_o_caminho("/casa/musica") == "/casa/musica");
  // O til, o ponto, o traço e o sublinhado são livres pela norma.
  CHECK(nu::escapa_o_caminho("a-b_c.d~e") == "a-b_c.d~e");
  CHECK(nu::escapa_o_caminho("50%+1") == "50%25%2B1");
}

TEST_CASE("a data da exclusão sahe em ISO 8601 na hora local") {
  // Pelo mktime, e não por um carimbo escripto á mão: assim a prova diz a
  // mesma cousa em qualquer fuso, que é onde uma comparação crua falharia.
  std::tm partido = {};
  partido.tm_year = 126;  // 2026
  partido.tm_mon = 8;     // setembro
  partido.tm_mday = 3;
  partido.tm_hour = 4;
  partido.tm_min = 5;
  partido.tm_sec = 6;
  partido.tm_isdst = -1;
  CHECK(nu::data_da_exclusao(std::mktime(&partido)) == "2026-09-03T04:05:06");
}

TEST_CASE("nome já tomado na lixeira ganha suffixo, e o bilhete ganha o mesmo") {
  Cova cova;
  nu::manda_a_lixeira(cova.poe("roda.mp3", "primeira"), cova.lixeira());
  const nu::DaLixeira segunda =
      nu::manda_a_lixeira(cova.poe("roda.mp3", "segunda"), cova.lixeira());
  CHECK(segunda.feita);
  CHECK(segunda.nome == "roda.mp3.2");
  // A primeira NÃO se perde: é o que a collisão por suffixo veio guardar.
  CHECK(texto_de(cova.lixeira() / "files" / "roda.mp3") == "primeira");
  CHECK(texto_de(cova.lixeira() / "files" / "roda.mp3.2") == "segunda");
  // O suffixo vae aos DOUS. Separados, a lixeira mostraria a segunda faixa com
  // o bilhete da primeira, e restaurá-la escreveria por cima do que ha.
  CHECK(std::filesystem::exists(cova.lixeira() / "info" /
                                "roda.mp3.2.trashinfo"));
}

TEST_CASE("o .lrc ao lado da faixa vae junto, e por par proprio") {
  Cova cova;
  const std::filesystem::path faixa = cova.poe("chuva.mp3", "som");
  const std::filesystem::path lrc = cova.poe("chuva.lrc", "[00:01.00] agua");
  const nu::DaLixeira desfecho = nu::manda_a_lixeira(faixa, cova.lixeira());
  CHECK(desfecho.levou_a_letra);
  CHECK_FALSE(std::filesystem::exists(lrc));
  CHECK(texto_de(cova.lixeira() / "files" / "chuva.lrc") == "[00:01.00] agua");
  // Par PROPRIO, e não pendurado no da faixa: assim cada um se restaura por si.
  CHECK(std::filesystem::exists(cova.lixeira() / "info" /
                                "chuva.lrc.trashinfo"));
}

TEST_CASE("a lixeira recusa com razão, e o que ella recusa fica no logar") {
  Cova cova;
  const nu::DaLixeira ausente =
      nu::manda_a_lixeira(cova.lixeira() / "nunca houve.mp3", cova.lixeira());
  CHECK_FALSE(ausente.feita);
  CHECK_FALSE(ausente.razao.empty());
  const std::filesystem::path faixa = cova.poe("fica.mp3", "som");
  const nu::DaLixeira sem_raiz = nu::manda_a_lixeira(faixa, {});
  CHECK_FALSE(sem_raiz.feita);
  CHECK_FALSE(sem_raiz.razao.empty());
  // Recusa não apaga: a faixa que não se pôde mandar ha de continuar a tocar.
  CHECK(std::filesystem::exists(faixa));
}
