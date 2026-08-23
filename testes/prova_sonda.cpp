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

#include <initializer_list>
#include <string_view>
#include <vector>

#include "nucleo/sonda.hpp"

namespace nu = mysong::nucleo;

