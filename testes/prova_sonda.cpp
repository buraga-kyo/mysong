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
