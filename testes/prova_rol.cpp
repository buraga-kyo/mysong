// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO ROL — testes/prova_rol.cpp
// ══════════════════════════════════════════════════════════════════════════
// Corre inteira em directorio temporario: o caminho do banco entra por parâmetro,
// donde prova alguma pode tocar as listas de quem nos usa. O que ella afere é a
// ORDEM, e o defeito que a peça pode ter é ordem com buraco: retirar o do meio e
// deixar zero e dous.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "nucleo/rol.hpp"

namespace nu = mysong::nucleo;

namespace {

class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-rol-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  std::filesystem::path banco() const { return caminho_ / "rol.sqlite3"; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

TEST_CASE("o saneamento do nome apara os brancos, e nome vazio não vale") {
  CHECK(nu::saneia_nome_de_rol("  Da manhã  ") == "Da manhã");
  CHECK(nu::saneia_nome_de_rol("\t\n ").empty());
  CHECK(nu::saneia_nome_de_rol("").empty());
  // O corte pelo comprimento recúa até ao byte lider: cortar UTF-8 a meio deixaria
  // byte solto, e o nome sahiria com um losango no fim.
  const std::string comprido(119, 'x');
  const std::string cortado = nu::saneia_nome_de_rol(comprido + "á");
  CHECK(cortado.size() == 119);
  CHECK(cortado == comprido);
}

TEST_CASE("o banco nasce na primeira abertura, com a versão assentada") {
  Cova cova;
  CHECK_FALSE(std::filesystem::exists(cova.banco()));
  nu::Roleiro roleiro(cova.banco());
  REQUIRE(roleiro.aberto());
  CHECK(roleiro.versao() == nu::kVersaoDoRol);
  CHECK(roleiro.rois().empty());
  // Abrir DUAS vezes não duplica a versão: o esquema é IF NOT EXISTS, e a versão
  // sómente se assenta havendo zero linhas.
  nu::Roleiro outro(cova.banco());
  CHECK(outro.versao() == nu::kVersaoDoRol);
}

TEST_CASE("criar recusa nome vazio, e recusa nome repetido") {
  Cova cova;
  nu::Roleiro roleiro(cova.banco());
  const int uma = roleiro.cria("Da manhã");
  CHECK(uma > 0);
  CHECK(roleiro.cria("   ") == 0);
  // Duas listas do mesmo nome na tela não se distinguem, e por isso não se aceita.
  CHECK(roleiro.cria("Da manhã") == 0);
  // E o nome saneia-se ANTES de se comparar: «  Da manhã  » é a mesma.
  CHECK(roleiro.cria("  Da manhã  ") == 0);
  REQUIRE(roleiro.rois().size() == 1);
  CHECK(roleiro.rois()[0].nome == "Da manhã");
  CHECK(roleiro.rois()[0].quantos == 0);
}

TEST_CASE("juntar põe no FIM, e a lista sahe na ordem gravada") {
  Cova cova;
  nu::Roleiro roleiro(cova.banco());
  const int id = roleiro.cria("Da manhã");
  REQUIRE(roleiro.junta(id, "/a/1.mp3"));
  REQUIRE(roleiro.junta(id, "/a/2.mp3"));
  REQUIRE(roleiro.junta(id, "/a/3.mp3"));
  CHECK(roleiro.faixas(id) ==
        std::vector<std::string>{"/a/1.mp3", "/a/2.mp3", "/a/3.mp3"});
  CHECK(roleiro.rois()[0].quantos == 3);
  // A MESMA faixa duas vezes é direito de quem a quer duas vezes.
  REQUIRE(roleiro.junta(id, "/a/1.mp3"));
  CHECK(roleiro.faixas(id).size() == 4);
  // Lista que não existe não aceita faixa: quem o recusa é a chave estrangeira.
  CHECK_FALSE(roleiro.junta(id + 999, "/a/4.mp3"));
  CHECK(roleiro.faixas(id + 999).empty());
  CHECK_FALSE(roleiro.junta(id, ""));
}

TEST_CASE("retirar do MEIO fecha o buraco da ordem") {
  Cova cova;
  nu::Roleiro roleiro(cova.banco());
  const int id = roleiro.cria("Da manhã");
  for (const char* qual : {"/a/1.mp3", "/a/2.mp3", "/a/3.mp3"})
    REQUIRE(roleiro.junta(id, qual));

  REQUIRE(roleiro.retira(id, 1));  // o do meio
  CHECK(roleiro.faixas(id) == std::vector<std::string>{"/a/1.mp3", "/a/3.mp3"});
  // O BURACO fechou-se: juntar outra põe na ordem 2, e não na 3. Se o buraco
  // ficasse, mover para cima teria de adivinhar quem é o vizinho.
  REQUIRE(roleiro.junta(id, "/a/4.mp3"));
  REQUIRE(roleiro.troca(id, 1, 2));
  CHECK(roleiro.faixas(id) ==
        std::vector<std::string>{"/a/1.mp3", "/a/4.mp3", "/a/3.mp3"});
  // Ordem que não existe não se retira, e a lista fica como estava: a transacção
  // desfaz-se, e não deixa a renumeração meia.
  CHECK_FALSE(roleiro.retira(id, 9));
  CHECK(roleiro.faixas(id) ==
        std::vector<std::string>{"/a/1.mp3", "/a/4.mp3", "/a/3.mp3"});
}

TEST_CASE("trocar move para cima e para baixo, e recusa a ordem que não ha") {
  Cova cova;
  nu::Roleiro roleiro(cova.banco());
  const int id = roleiro.cria("Da manhã");
  for (const char* qual : {"/a/1.mp3", "/a/2.mp3", "/a/3.mp3"})
    REQUIRE(roleiro.junta(id, qual));

  REQUIRE(roleiro.troca(id, 0, 1));
  CHECK(roleiro.faixas(id) ==
        std::vector<std::string>{"/a/2.mp3", "/a/1.mp3", "/a/3.mp3"});
  // Trocar não vizinhas tambem vale: é a mesma operação.
  REQUIRE(roleiro.troca(id, 0, 2));
  CHECK(roleiro.faixas(id) ==
        std::vector<std::string>{"/a/3.mp3", "/a/1.mp3", "/a/2.mp3"});
  // Ordem que não existe não troca, e a lista fica como estava: sem a
  // transacção, a primeira das tres escriptas deixava a linha na sentinella e a
  // faixa desapparecia da lista.
  CHECK_FALSE(roleiro.troca(id, 0, 9));
  CHECK(roleiro.faixas(id) ==
        std::vector<std::string>{"/a/3.mp3", "/a/1.mp3", "/a/2.mp3"});
  CHECK_FALSE(roleiro.troca(id, 0, 0));  // consigo mesma não é troca
}

TEST_CASE("apagar leva os itens consigo, e a lista some") {
  Cova cova;
  nu::Roleiro roleiro(cova.banco());
  const int id = roleiro.cria("Da manhã");
  const int outra = roleiro.cria("Da noite");
  REQUIRE(roleiro.junta(id, "/a/1.mp3"));
  REQUIRE(roleiro.junta(outra, "/a/2.mp3"));

  REQUIRE(roleiro.apaga(id));
  REQUIRE(roleiro.rois().size() == 1);
  CHECK(roleiro.rois()[0].nome == "Da noite");
  CHECK(roleiro.faixas(id).empty());
  // A OUTRA lista não se tocou: apagar uma não leva a vizinha.
  CHECK(roleiro.faixas(outra) == std::vector<std::string>{"/a/2.mp3"});
  CHECK_FALSE(roleiro.apaga(id));  // apagada duas vezes, a segunda é falsa
}

TEST_CASE("renomear conserva os itens, e recusa o nome vazio") {
  Cova cova;
  nu::Roleiro roleiro(cova.banco());
  const int id = roleiro.cria("Da manhã");
  REQUIRE(roleiro.junta(id, "/a/1.mp3"));
  REQUIRE(roleiro.renomeia(id, "  Da tarde  "));
  REQUIRE(roleiro.rois().size() == 1);
  CHECK(roleiro.rois()[0].nome == "Da tarde");
  CHECK(roleiro.faixas(id) == std::vector<std::string>{"/a/1.mp3"});
  CHECK_FALSE(roleiro.renomeia(id, "   "));
  CHECK_FALSE(roleiro.renomeia(id + 999, "Alheia"));
  CHECK(roleiro.rois()[0].nome == "Da tarde");
}

TEST_CASE("a lista sobrevive a fechar e reabrir o banco") {
  Cova cova;
  int id = 0;
  {
    nu::Roleiro roleiro(cova.banco());
    id = roleiro.cria("Da manhã");
    for (const char* qual : {"/a/1.mp3", "/a/2.mp3", "/a/3.mp3"})
      REQUIRE(roleiro.junta(id, qual));
    REQUIRE(roleiro.troca(id, 0, 2));
  }
  // O aceite da tarefa em uma linha: fechar e reabrir, e a ordem é a que se
  // deixou. O banco fecha-se pelo destructor, e não por punho que se chame.
  nu::Roleiro outra_vez(cova.banco());
  REQUIRE(outra_vez.rois().size() == 1);
  CHECK(outra_vez.rois()[0].quantos == 3);
  CHECK(outra_vez.faixas(id) ==
        std::vector<std::string>{"/a/3.mp3", "/a/2.mp3", "/a/1.mp3"});
}

TEST_CASE("nome com aspas é nome, e não pedaço de SQL") {
  Cova cova;
  nu::Roleiro roleiro(cova.banco());
  // Se a cadeia fosse por concatenação em vez de amarração, isto abriria cadeia no
  // meio do SQL e a lista não nasceria.
  const int id = roleiro.cria("Ária \"Ré\"; DROP TABLE rol;--");
  REQUIRE(id > 0);
  REQUIRE(roleiro.junta(id, "/a/1.mp3"));
  REQUIRE(roleiro.rois().size() == 1);
  CHECK(roleiro.rois()[0].nome == "Ária \"Ré\"; DROP TABLE rol;--");
  CHECK(roleiro.faixas(id).size() == 1);
}

TEST_CASE("a faixa apagada sae de todas as listas, e a ordem fecha o buraco") {
  Cova cova;
  nu::Roleiro roleiro(cova.banco());
  const int manha = roleiro.cria("Da manhã");
  const int noite = roleiro.cria("Da noite");
  REQUIRE(roleiro.junta(manha, "/a/uma.mp3"));
  REQUIRE(roleiro.junta(manha, "/a/vae.mp3"));
  REQUIRE(roleiro.junta(manha, "/a/outra.mp3"));
  REQUIRE(roleiro.junta(noite, "/a/vae.mp3"));
  // Duas vezes na MESMA lista: é o caso que a ordem descendente resolve, e o
  // que uma retirada de baixo para cima tiraria do logar errado.
  REQUIRE(roleiro.junta(noite, "/a/vae.mp3"));
  CHECK(roleiro.retira_de_todos("/a/vae.mp3") == 3);
  CHECK(roleiro.faixas(manha) ==
        std::vector<std::string>{"/a/uma.mp3", "/a/outra.mp3"});
  CHECK(roleiro.faixas(noite).empty());
  // Retirar o que já não está é zero, e não avaria.
  CHECK(roleiro.retira_de_todos("/a/vae.mp3") == 0);
  // A ordem ficou CONTIGUA: sem isso, subir e descer passavam a adivinhar quem
  // é o visinho. Trocar as duas que ficaram prova-o sem se ler a columna.
  CHECK(roleiro.troca(manha, 0, 1));
  CHECK(roleiro.faixas(manha) ==
        std::vector<std::string>{"/a/outra.mp3", "/a/uma.mp3"});
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
