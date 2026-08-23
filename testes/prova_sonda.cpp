// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA SONDA — testes/prova_sonda.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a sonda dos requisitos pelos DOUS caminhos: com tudo presente e com o
// que se queira ausente. Nenhum caso consulta o systema: nem fontconfig, nem
// dlopen, nem PATH, nem variavel de ambiente. E é d'isto que a prova vale.
//
// A RAZÃO, que se registra para que ninguem a desfaça por commodidade: nesta
// machina os quatro requisitos estão presentes. Prova que chamasse o inquerito
// do systema sahiria verde aqui e MUDA sobre o caminho da recusa, que é o
// caminho que a tarefa inteira existe para garantir. Provar o que já funcciona
// não é prova; é cerimonia.
//
// DOMÍNIO ......... inqueritos de DUBLÊ, armados neste arquivo, que respondem
//                   ausente sómente ás chaves que o caso nomeia.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... nenhum caso toca o systema nem o ambiente; donde o
//                   resultado é o mesmo na machina do auctor e na crua.
// Q.E.D. .......... o inquerito sendo parametro, o impedimento é observavel
//                   onde nada falta; logo a recusa de abrir é asserção provada,
//                   e não promessa de quem escreveu o cabeçalho.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <initializer_list>
#include <string_view>
#include <vector>

#include "nucleo/sonda.hpp"

namespace nu = mysong::nucleo;

namespace {

// inquerito_faltando — arma um inquerito de dublê que responde AUSENTE sómente
// aos requisitos cujas chaves se nomeiam, e presente a todos os demais. A
// traducção de alvo para chave faz-se pela taboa, tal como o binario a faz.
nu::Inquerito inquerito_faltando(std::initializer_list<std::string_view> chaves) {
  const std::vector<std::string_view> ausentes(chaves.begin(), chaves.end());
  const auto responde = [ausentes](nu::Especie especie, std::string_view alvo) {
    for (const nu::Requisito& requisito : nu::requisitos())
      if (requisito.especie == especie && requisito.alvo == alvo)
        for (const std::string_view chave : ausentes)
          if (chave == requisito.chave) return false;
    return true;
  };
  nu::Inquerito inquerito;
  inquerito.familia_de_fonte = [responde](std::string_view alvo) {
    return responde(nu::Especie::FamiliaDeFonte, alvo);
  };
  inquerito.bibliotheca = [responde](std::string_view alvo) {
    return responde(nu::Especie::Bibliotheca, alvo);
  };
  inquerito.executavel = [responde](std::string_view alvo) {
    return responde(nu::Especie::Executavel, alvo);
  };
  return inquerito;
}

}  // namespace


TEST_CASE("com tudo presente, nada falta e nada impede") {
  const nu::Relatorio relatorio = nu::sondar(inquerito_faltando({}));
  CHECK(relatorio.estados.size() == nu::requisitos().size());
  CHECK_FALSE(relatorio.ha_falta());
  CHECK_FALSE(relatorio.ha_impedimento());
  CHECK(relatorio.faltas().empty());
  for (const nu::Estado& estado : relatorio.estados) CHECK(estado.presente);
}

TEST_CASE("a taboa traz os quatro requisitos, de chave unica e remedio dado") {
  const std::vector<nu::Requisito>& taboa = nu::requisitos();
  REQUIRE(taboa.size() == 4);
  for (std::size_t aqui = 0; aqui < taboa.size(); ++aqui) {
    CHECK_FALSE(taboa[aqui].chave.empty());
    CHECK_FALSE(taboa[aqui].nome.empty());
    CHECK_FALSE(taboa[aqui].alvo.empty());
    CHECK_FALSE(taboa[aqui].remedio.empty());
    for (std::size_t adeante = aqui + 1; adeante < taboa.size(); ++adeante)
      CHECK(taboa[aqui].chave != taboa[adeante].chave);
  }
}

// A guarda da consulta VAZIA: inquerito mal montado ha de accusar falta, e
// nunca dar por bom aquillo que não sabe. É a escolha segura das duas.
TEST_CASE("inquerito sem consulta alguma accusa falta, e não dá por bom") {
  const nu::Relatorio relatorio = nu::sondar(nu::Inquerito{});
  CHECK(relatorio.ha_impedimento());
  CHECK(relatorio.faltas().size() == nu::requisitos().size());
}

TEST_CASE("faltando a fonte, ha impedimento e o remedio vem nomeado") {
  const nu::Relatorio relatorio = nu::sondar(inquerito_faltando({"fonte"}));
  const std::vector<nu::Estado> faltas = relatorio.faltas();
  REQUIRE(faltas.size() == 1);
  CHECK(faltas.front().requisito.chave == "fonte");
  CHECK(faltas.front().requisito.gravidade == nu::Gravidade::Impedimento);
  CHECK_FALSE(faltas.front().requisito.remedio.empty());
  CHECK(relatorio.ha_impedimento());
  CHECK(relatorio.ha_falta());
}

// A GRAVIDADE da libmpv é asserção, e não detalhe: sem machina de som não ha
// tocador algum, sómente moldura. Rebaixada a aviso, este caso morre, e é por
// elle que a prova por mutação passa.
TEST_CASE("a libmpv é impedimento, e nunca aviso") {
  const nu::Relatorio relatorio = nu::sondar(inquerito_faltando({"libmpv"}));
  const std::vector<nu::Estado> faltas = relatorio.faltas();
  REQUIRE(faltas.size() == 1);
  CHECK(faltas.front().requisito.chave == "libmpv");
  CHECK(faltas.front().requisito.gravidade == nu::Gravidade::Impedimento);
  CHECK(relatorio.ha_impedimento());
}

// Os avisos não trancam a porta, e é o que aqui se afirma: ha falta, e NÃO ha
// impedimento. O programa que se recusasse a abrir por falta de chafa estaria
// a negar musica a quem sómente ficaria sem ver capa.
TEST_CASE("faltando o yt-dlp, ha aviso, e a porta não se tranca") {
  const nu::Relatorio relatorio = nu::sondar(inquerito_faltando({"yt-dlp"}));
  const std::vector<nu::Estado> faltas = relatorio.faltas();
  REQUIRE(faltas.size() == 1);
  CHECK(faltas.front().requisito.chave == "yt-dlp");
  CHECK(faltas.front().requisito.gravidade == nu::Gravidade::Aviso);
  CHECK(relatorio.ha_falta());
  CHECK_FALSE(relatorio.ha_impedimento());
}

TEST_CASE("faltando o chafa, ha aviso, e a porta não se tranca") {
  const nu::Relatorio relatorio = nu::sondar(inquerito_faltando({"chafa"}));
  const std::vector<nu::Estado> faltas = relatorio.faltas();
  REQUIRE(faltas.size() == 1);
  CHECK(faltas.front().requisito.chave == "chafa");
  CHECK(faltas.front().requisito.gravidade == nu::Gravidade::Aviso);
  CHECK_FALSE(relatorio.ha_impedimento());
}

// Faltando tudo, a tela ha de mostrar tudo, e na ordem em que se lê: primeiro
// o que impede, depois o que sómente avisa. Quem está impedido de abrir ha de
// ler primeiro aquillo que o impede.
TEST_CASE("faltando os quatro, os impedimentos vêm adeante dos avisos") {
  const nu::Relatorio relatorio =
      nu::sondar(inquerito_faltando({"chafa", "libmpv", "fonte", "yt-dlp"}));
  const std::vector<nu::Estado> faltas = relatorio.faltas();
  REQUIRE(faltas.size() == 4);
  CHECK(faltas[0].requisito.gravidade == nu::Gravidade::Impedimento);
  CHECK(faltas[1].requisito.gravidade == nu::Gravidade::Impedimento);
  CHECK(faltas[2].requisito.gravidade == nu::Gravidade::Aviso);
  CHECK(faltas[3].requisito.gravidade == nu::Gravidade::Aviso);
  CHECK(relatorio.ha_impedimento());
}

// A ordem é ESTAVEL, e não mera consequencia do dia: duas colheitas do mesmo
// relatorio devolvem a mesma enfiada de chaves, byte por byte.
TEST_CASE("a ordem das faltas repete-se identica em duas colheitas") {
  const nu::Relatorio relatorio =
      nu::sondar(inquerito_faltando({"yt-dlp", "fonte", "chafa"}));
  const std::vector<nu::Estado> primeira = relatorio.faltas();
  const std::vector<nu::Estado> segunda = relatorio.faltas();
  REQUIRE(primeira.size() == segunda.size());
  for (std::size_t passo = 0; passo < primeira.size(); ++passo)
    CHECK(primeira[passo].requisito.chave == segunda[passo].requisito.chave);
  CHECK(primeira.front().requisito.chave == "fonte");
}

// Impedimento acompanhado de aviso AINDA impede: um só basta para trancar.
TEST_CASE("o impedimento tranca a porta ainda que venha com avisos") {
  const nu::Relatorio relatorio =
      nu::sondar(inquerito_faltando({"libmpv", "chafa"}));
  CHECK(relatorio.faltas().size() == 2);
  CHECK(relatorio.ha_impedimento());
}
